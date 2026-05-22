/*
==============================================================================

GUN COMMANDER

==============================================================================
*/

#include "g_local.h"
#include "../../quake2/monsterframes/m_gunner.h"

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
#define GUNCMDR_INVASION_RUN_SCALE  1.15f

#define GUNCMDR_SCALE(self)              (((self)->s.scale > 0) ? (self)->s.scale : 1.0f)

#define GUNCMDR_MINS_X_SCALED(self)      (-16.0f * GUNCMDR_SCALE(self))
#define GUNCMDR_MINS_Y_SCALED(self)      (-16.0f * GUNCMDR_SCALE(self))
#define GUNCMDR_MINS_Z_SCALED(self)      (-24.0f * GUNCMDR_SCALE(self))
#define GUNCMDR_MAXS_X_SCALED(self)      (16.0f * GUNCMDR_SCALE(self))
#define GUNCMDR_MAXS_Y_SCALED(self)      (16.0f * GUNCMDR_SCALE(self))
#define GUNCMDR_STAND_MAXZ_SCALED(self)  (36.0f * GUNCMDR_SCALE(self))
#define GUNCMDR_DUCK_MAXZ_SCALED(self)   (4.0f  * GUNCMDR_SCALE(self))
#define GUNCMDR_DEAD_MAXZ_SCALED(self)   (-8.0f * GUNCMDR_SCALE(self))
#define GUNCMDR_SHRINK_MAXZ_SCALED(self) (-4.0f * GUNCMDR_SCALE(self))

static void guncmdr_stand(edict_t *self);
static void guncmdr_run(edict_t *self);
static void guncmdr_attack(edict_t *self);
static void guncmdr_fire_chain(edict_t *self);
static void guncmdr_refire_chain(edict_t *self);
static void guncmdr_grenade_finished(edict_t *self);
static void guncmdr_grenade_mortar_resume(edict_t *self);
static void guncmdr_resume_back_attack(edict_t *self);
static void guncmdr_duck_down(edict_t *self);
static void guncmdr_duck_up(edict_t *self);
static mmove_t guncmdr_move_duckstep_dodge;
extern mmove_t guncmdr_move_fidget;
extern mmove_t guncmdr_move_death1;
extern mmove_t guncmdr_move_death2;
extern mmove_t guncmdr_move_death6;

static void guncmdr_ai_dodge_slide(edict_t *self, float dist);
extern mmove_t guncmdr_move_dodge_slide;

static void guncmdr_ai_run(edict_t *self, float dist)
{
	drone_ai_run(self, invasion->value ? dist * GUNCMDR_INVASION_RUN_SCALE : dist);
}

static void guncmdr_set_stand_bbox(edict_t *self)
{
	VectorSet(self->mins,
		GUNCMDR_MINS_X_SCALED(self),
		GUNCMDR_MINS_Y_SCALED(self),
		GUNCMDR_MINS_Z_SCALED(self));
	VectorSet(self->maxs,
		GUNCMDR_MAXS_X_SCALED(self),
		GUNCMDR_MAXS_Y_SCALED(self),
		GUNCMDR_STAND_MAXZ_SCALED(self));
}

static void guncmdr_idle_sound(edict_t *self)
{
	if (random() < 0.5)
		gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
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
	guncmdr_ai_run, 15, NULL,
	guncmdr_ai_run, 16, NULL,
	guncmdr_ai_run, 20, NULL,
	guncmdr_ai_run, 18, NULL,
	guncmdr_ai_run, 24, NULL,
	guncmdr_ai_run, 13.5, NULL
};
mmove_t guncmdr_move_run = { FRAME_c_run101, FRAME_c_run106, guncmdr_frames_run, NULL };

static void guncmdr_run(edict_t *self)
{
	guncmdr_duck_up(self);

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

	if (self->s.frame >= FRAME_c_attack401 && self->s.frame <= FRAME_c_attack505)
		flash_number = MZ2_GUNNER_MACHINEGUN_2;
	else if (self->s.frame >= FRAME_c_run201 && self->s.frame <= FRAME_c_run206)
		flash_number = MZ2_GUNNER_MACHINEGUN_1 + (self->s.frame - FRAME_c_run201);
	else if (self->s.frame >= FRAME_c_duckstep01 && self->s.frame <= FRAME_c_duckstep06)
		flash_number = MZ2_GUNCMDR_CHAINGUN_2;
	else
		flash_number = MZ2_GUNNER_MACHINEGUN_1 + ((self->s.frame - FRAME_c_attack107) % 6);

	damage = M_SHOTGUN_DMG_BASE + M_SHOTGUN_DMG_ADDON * drone_damagelevel(self);
	if (M_SHOTGUN_DMG_MAX && damage > M_SHOTGUN_DMG_MAX)
		damage = M_SHOTGUN_DMG_MAX;

	MonsterAim(self, M_PROJECTILE_ACC, 800, false, flash_number, forward, start);
	if (!M_MonsterHasClearShotFrom(self, start))
		return;

	damage = vrx_increase_monster_damage_by_talent(self->activator, damage);
	fire_flechette(self, start, forward, damage, 800, damage);

	gi.WriteByte(svc_muzzleflash2);
	gi.WriteShort(self - g_edicts);
	gi.WriteByte(flash_number);
	gi.multicast(start, MULTICAST_PVS);
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
	guncmdr_ai_run, 15, GunnerCmdrFire,
	guncmdr_ai_run, 16, GunnerCmdrFire,
	guncmdr_ai_run, 20, GunnerCmdrFire,
	guncmdr_ai_run, 18, GunnerCmdrFire,
	guncmdr_ai_run, 24, GunnerCmdrFire,
	guncmdr_ai_run, 13.5, GunnerCmdrFire
};
mmove_t guncmdr_move_fire_chain_run = { FRAME_c_run201, FRAME_c_run206, guncmdr_frames_fire_chain_run, guncmdr_refire_chain };

mframe_t guncmdr_frames_fire_chain_dodge_right[] =
{
	guncmdr_ai_dodge_slide, 10.2, GunnerCmdrFire,
	guncmdr_ai_dodge_slide, 18.0, GunnerCmdrFire,
	guncmdr_ai_dodge_slide, 7.0, GunnerCmdrFire,
	guncmdr_ai_dodge_slide, 7.2, GunnerCmdrFire,
	guncmdr_ai_dodge_slide, -2.0, GunnerCmdrFire
};
mmove_t guncmdr_move_fire_chain_dodge_right = { FRAME_c_attack401, FRAME_c_attack405, guncmdr_frames_fire_chain_dodge_right, guncmdr_refire_chain };

mframe_t guncmdr_frames_fire_chain_dodge_left[] =
{
	guncmdr_ai_dodge_slide, 10.2, GunnerCmdrFire,
	guncmdr_ai_dodge_slide, 18.0, GunnerCmdrFire,
	guncmdr_ai_dodge_slide, 7.0, GunnerCmdrFire,
	guncmdr_ai_dodge_slide, 7.2, GunnerCmdrFire,
	guncmdr_ai_dodge_slide, -2.0, GunnerCmdrFire
};
mmove_t guncmdr_move_fire_chain_dodge_left = { FRAME_c_attack501, FRAME_c_attack505, guncmdr_frames_fire_chain_dodge_left, guncmdr_refire_chain };

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
	if (!M_MonsterHasClearShotFrom(self, start))
		return;

	monster_fire_grenade(self, start, forward, damage, speed, flash_number);
}

static void guncmdr_ai_duckstep_slide(edict_t *self, float dist)
{
	if (!G_EntIsAlive(self->enemy) && G_EntIsAlive(self->monsterinfo.attacker))
		self->enemy = self->monsterinfo.attacker;
	if (!G_EntIsAlive(self->enemy))
		return;

	guncmdr_duck_down(self);
	drone_ai_dodge_slide(self, dist);
}

mframe_t guncmdr_frames_attack_mortar[] =
{
	ai_charge, 0, guncmdr_duck_down,
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
	ai_charge, 0, guncmdr_duck_up,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t guncmdr_move_attack_mortar = { FRAME_c_attack201, FRAME_c_attack221, guncmdr_frames_attack_mortar, guncmdr_grenade_finished };

static void guncmdr_grenade_mortar_resume(edict_t *self)
{
	self->monsterinfo.currentmove = &guncmdr_move_attack_mortar;
	self->s.frame = self->count;
}

mframe_t guncmdr_frames_attack_mortar_dodge[] =
{
	guncmdr_ai_duckstep_slide, 11, NULL,
	guncmdr_ai_duckstep_slide, 12, GunnerCmdrFire,
	guncmdr_ai_duckstep_slide, 16, NULL,
	guncmdr_ai_duckstep_slide, 16, GunnerCmdrFire,
	guncmdr_ai_duckstep_slide, 12, NULL,
	guncmdr_ai_duckstep_slide, 11, GunnerCmdrFire
};
mmove_t guncmdr_move_attack_mortar_dodge = { FRAME_c_duckstep01, FRAME_c_duckstep06, guncmdr_frames_attack_mortar_dodge, guncmdr_grenade_mortar_resume };

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

static void guncmdr_resume_back_attack(edict_t *self)
{
	self->monsterinfo.currentmove = &guncmdr_move_attack_grenade_back;
	self->s.frame = self->count;
}

mframe_t guncmdr_frames_attack_grenade_back_dodge_right[] =
{
	guncmdr_ai_dodge_slide, 10.2, NULL,
	guncmdr_ai_dodge_slide, 18.0, NULL,
	guncmdr_ai_dodge_slide, 7.0, NULL,
	guncmdr_ai_dodge_slide, 7.2, NULL,
	guncmdr_ai_dodge_slide, -2.0, NULL
};
mmove_t guncmdr_move_attack_grenade_back_dodge_right = { FRAME_c_attack601, FRAME_c_attack605, guncmdr_frames_attack_grenade_back_dodge_right, guncmdr_resume_back_attack };

mframe_t guncmdr_frames_attack_grenade_back_dodge_left[] =
{
	guncmdr_ai_dodge_slide, 10.2, NULL,
	guncmdr_ai_dodge_slide, 18.0, NULL,
	guncmdr_ai_dodge_slide, 7.0, NULL,
	guncmdr_ai_dodge_slide, 7.2, NULL,
	guncmdr_ai_dodge_slide, -2.0, NULL
};
mmove_t guncmdr_move_attack_grenade_back_dodge_left = { FRAME_c_attack701, FRAME_c_attack705, guncmdr_frames_attack_grenade_back_dodge_left, guncmdr_resume_back_attack };

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
	self->maxs[2] = GUNCMDR_DUCK_MAXZ_SCALED(self);
	self->takedamage = DAMAGE_YES;
	gi.linkentity(self);
}

static void guncmdr_duck_up(edict_t *self)
{
	vec3_t oldmaxs;
	trace_t tr;

	if (!(self->monsterinfo.aiflags & AI_DUCKED) &&
		self->maxs[2] == GUNCMDR_STAND_MAXZ_SCALED(self))
		return;

	VectorCopy(self->maxs, oldmaxs);
	self->maxs[2] = GUNCMDR_STAND_MAXZ_SCALED(self);

	tr = gi.trace(self->s.origin, self->mins, self->maxs, self->s.origin, self, MASK_MONSTERSOLID);
	if (tr.startsolid || tr.allsolid)
	{
		VectorCopy(oldmaxs, self->maxs);
		self->monsterinfo.aiflags |= AI_DUCKED;
		return;
	}

	self->monsterinfo.aiflags &= ~AI_DUCKED;
	self->takedamage = DAMAGE_AIM;
	gi.linkentity(self);
}

static void guncmdr_duck_hold(edict_t *self)
{
	if (self->monsterinfo.pausetime > level.time)
		self->monsterinfo.nextframe = self->s.frame;
}

mframe_t guncmdr_frames_duck_attack[] =
{
	ai_move, 3.6, NULL,
	ai_move, 5.6, guncmdr_duck_down,
	ai_move, 8.4, NULL,
	ai_move, 2.0, guncmdr_duck_hold,
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

static void guncmdr_finish_duckstep_dodge(edict_t *self)
{
	if (self->monsterinfo.nextattack > 0 && G_ValidTarget(self, self->enemy, true, true))
	{
		self->monsterinfo.nextattack--;
		self->monsterinfo.currentmove = &guncmdr_move_duckstep_dodge;
		return;
	}

	self->monsterinfo.nextattack = 0;
	guncmdr_duck_up(self);
	guncmdr_run(self);
}

mframe_t guncmdr_frames_duckstep_dodge[] =
{
	guncmdr_ai_duckstep_slide, 11, NULL,
	guncmdr_ai_duckstep_slide, 12, GunnerCmdrFire,
	guncmdr_ai_duckstep_slide, 16, NULL,
	guncmdr_ai_duckstep_slide, 16, GunnerCmdrFire,
	guncmdr_ai_duckstep_slide, 12, NULL,
	guncmdr_ai_duckstep_slide, 11, GunnerCmdrFire
};
static mmove_t guncmdr_move_duckstep_dodge = { FRAME_c_duckstep01, FRAME_c_duckstep06, guncmdr_frames_duckstep_dodge, guncmdr_finish_duckstep_dodge };

static void guncmdr_jump_now(edict_t *self)
{
	vec3_t forward;

	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorMA(self->velocity, 150, forward, self->velocity);
	self->velocity[2] += 350;
}

static void guncmdr_jump2_now(edict_t *self)
{
	vec3_t forward;

	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorMA(self->velocity, 150, forward, self->velocity);
	self->velocity[2] += 400;
}

static void guncmdr_jump_wait_land(edict_t *self)
{
	if (!self->groundentity && level.time < self->monsterinfo.pausetime)
		self->monsterinfo.nextframe = self->s.frame;
	else
		self->monsterinfo.nextframe = self->s.frame + 1;
}

static void guncmdr_jump_wait_land_ai(edict_t *self, float dist)
{
	ai_move(self, dist);
	guncmdr_jump_wait_land(self);
}

mframe_t guncmdr_frames_jump[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, guncmdr_jump_now,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	guncmdr_jump_wait_land_ai, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t guncmdr_move_jump = { FRAME_c_jump01, FRAME_c_jump10, guncmdr_frames_jump, guncmdr_run };

mframe_t guncmdr_frames_jump2[] =
{
	ai_move, -8, NULL,
	ai_move, -4, NULL,
	ai_move, -4, NULL,
	ai_move, 0, guncmdr_jump2_now,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	guncmdr_jump_wait_land_ai, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t guncmdr_move_jump2 = { FRAME_c_jump01, FRAME_c_jump10, guncmdr_frames_jump2, guncmdr_run };

static qboolean guncmdr_try_sidestep(edict_t *self);

static qboolean guncmdr_is_dodge_move(edict_t *self)
{
	return self->monsterinfo.currentmove == &guncmdr_move_fire_chain_dodge_left ||
		self->monsterinfo.currentmove == &guncmdr_move_fire_chain_dodge_right ||
		self->monsterinfo.currentmove == &guncmdr_move_attack_grenade_back_dodge_left ||
		self->monsterinfo.currentmove == &guncmdr_move_attack_grenade_back_dodge_right ||
		self->monsterinfo.currentmove == &guncmdr_move_attack_mortar_dodge ||
		self->monsterinfo.currentmove == &guncmdr_move_duckstep_dodge ||
		self->monsterinfo.currentmove == &guncmdr_move_dodge_slide ||
		self->monsterinfo.currentmove == &guncmdr_move_jump ||
		self->monsterinfo.currentmove == &guncmdr_move_jump2 ||
		self->monsterinfo.currentmove == &guncmdr_move_duck_attack;
}

static qboolean guncmdr_dodge_hit_low(edict_t *self, vec3_t dir)
{
	const float duck_height = self->absmax[2] - 33;

	return dir[2] > self->absmin[2] && dir[2] <= duck_height;
}

static void guncmdr_start_duck_dodge(edict_t *self)
{
	if (!self->groundentity)
		return;

	self->monsterinfo.nextattack = 0;
	self->monsterinfo.pausetime = level.time + 0.75f;
	self->monsterinfo.currentmove = &guncmdr_move_duck_attack;
	guncmdr_duck_down(self);
	self->monsterinfo.dodge_time = level.time + 1.8f;
}

static qboolean guncmdr_start_duckstep_dodge(edict_t *self)
{
	if (!self->groundentity)
		return false;

	self->monsterinfo.nextattack = random() < 0.55f ? 1 : 0;
	self->monsterinfo.currentmove = &guncmdr_move_duckstep_dodge;
	guncmdr_duck_down(self);
	self->monsterinfo.dodge_time = level.time + 1.6f;
	return true;
}

static qboolean guncmdr_start_mortar_dodge(edict_t *self)
{
	if (!self->groundentity || !G_ValidTarget(self, self->enemy, true, true))
		return false;

	if (entdist(self, self->enemy) < GUNCMDR_CHAINGUN_RUN_RANGE)
		return false;

	self->monsterinfo.nextattack = 0;
	self->monsterinfo.currentmove = &guncmdr_move_attack_mortar;
	guncmdr_duck_down(self);
	self->monsterinfo.dodge_time = level.time + 2.0f;
	return true;
}

static void guncmdr_dodge(edict_t *self, edict_t *attacker, vec3_t dir, int radius)
{
	if (level.time < self->monsterinfo.dodge_time)
		return;
	if (!attacker)
		return;
	if (OnSameTeam(self, attacker))
		return;

	self->monsterinfo.attacker = attacker;
	if (!G_EntIsAlive(self->enemy) && G_EntIsAlive(attacker))
		self->enemy = attacker;

	if (guncmdr_is_dodge_move(self))
	{
		if (self->monsterinfo.currentmove == &guncmdr_move_duck_attack &&
			(radius || guncmdr_dodge_hit_low(self, dir)))
		{
			drone_set_dodge_side(self, dir);
			guncmdr_start_duckstep_dodge(self);
		}
		return;
	}

	if (random() > 0.8)
		return;

	drone_set_dodge_side(self, dir);

	if (guncmdr_try_sidestep(self))
	{
		self->monsterinfo.dodge_time = level.time + 1.2f;
		return;
	}

	if (radius && self->groundentity)
	{
		if (random() < 0.35f && guncmdr_start_duckstep_dodge(self))
			return;
		if (random() < 0.35f && guncmdr_start_mortar_dodge(self))
			return;
		self->monsterinfo.pausetime = level.time + 2.0;
		self->monsterinfo.currentmove = &guncmdr_move_jump;
		self->monsterinfo.dodge_time = level.time + 3.0;
		return;
	}

	if (!radius && !guncmdr_dodge_hit_low(self, dir) && self->groundentity)
	{
		if (random() < 0.45f && guncmdr_start_duckstep_dodge(self))
			return;
		if (random() < 0.35f && guncmdr_start_mortar_dodge(self))
			return;
		guncmdr_start_duck_dodge(self);
		return;
	}

	if (guncmdr_start_duckstep_dodge(self))
		return;

	if (self->groundentity)
	{
		self->monsterinfo.currentmove = &guncmdr_move_dodge_slide;
		self->monsterinfo.dodge_time = level.time + 1.0;
		return;
	}
}

static qboolean guncmdr_can_proactive_dodge(edict_t *self, float dist)
{
	return !(self->monsterinfo.aiflags & AI_STAND_GROUND)
		&& self->groundentity
		&& dist > 160
		&& level.time >= self->monsterinfo.dodge_time;
}

static qboolean guncmdr_can_chain(edict_t *self)
{
	return M_MonsterHasClearShotFromFlash(self, MZ2_GUNNER_MACHINEGUN_1);
}

static qboolean guncmdr_can_dodge_chain(edict_t *self)
{
	return M_MonsterHasClearShotFromFlash(self, MZ2_GUNNER_MACHINEGUN_2);
}

static qboolean guncmdr_can_grenade(edict_t *self)
{
	return M_MonsterHasClearShotFromFlash(self, MZ2_GUNNER_GRENADE_1);
}

static void guncmdr_start_fire_chain_dodge(edict_t *self)
{
	guncmdr_duck_up(self);
	self->monsterinfo.lefty = !self->monsterinfo.lefty;

	if (self->monsterinfo.lefty)
		self->monsterinfo.currentmove = &guncmdr_move_fire_chain_dodge_left;
	else
		self->monsterinfo.currentmove = &guncmdr_move_fire_chain_dodge_right;

	self->monsterinfo.dodge_time = level.time + 1.0;
}

static void guncmdr_attack(edict_t *self)
{
	float dist;
	float zdiff;
	float r;
	qboolean can_chain;
	qboolean can_grenade;

	if (!G_ValidTarget(self, self->enemy, true, true))
		return;

	guncmdr_duck_up(self);

	dist = entdist(self, self->enemy);
	zdiff = fabs(self->s.origin[2] - self->enemy->s.origin[2]);
	r = random();
	can_chain = guncmdr_can_chain(self);
	can_grenade = guncmdr_can_grenade(self);

	if (dist <= MELEE_DISTANCE && self->monsterinfo.melee_finished < level.time)
		self->monsterinfo.currentmove = &guncmdr_move_attack_kick;
	else if (can_chain && guncmdr_can_proactive_dodge(self, dist) && r < 0.55)
		guncmdr_start_fire_chain_dodge(self);
	else if (can_grenade && (dist >= GUNCMDR_MORTAR_RANGE || zdiff > 96) && r < 0.75)
	{
		self->monsterinfo.currentmove = &guncmdr_move_attack_mortar;
		guncmdr_duck_down(self);
	}
	else if (can_grenade && self->groundentity && dist > 96 && dist < GUNCMDR_CHAINGUN_RUN_RANGE && r < 0.05)
	{
		self->monsterinfo.pausetime = level.time + 0.75f;
		self->monsterinfo.currentmove = &guncmdr_move_duck_attack;
		guncmdr_duck_down(self);
		self->monsterinfo.dodge_time = level.time + 1.8f;
	}
	else if (can_grenade && !(self->monsterinfo.aiflags & AI_STAND_GROUND) && dist > GUNCMDR_GRENADE_RANGE && r < 0.90)
		self->monsterinfo.currentmove = &guncmdr_move_attack_grenade_back;
	else if (can_chain)
		self->monsterinfo.currentmove = &guncmdr_move_attack_chain;
	else if (can_grenade)
	{
		self->monsterinfo.currentmove = &guncmdr_move_attack_grenade_back;
	}
	else
		return;

	M_DelayNextAttack(self, 0, true);
}

static void guncmdr_fire_chain(edict_t *self)
{
	if (!(self->monsterinfo.aiflags & AI_STAND_GROUND) && G_ValidTarget(self, self->enemy, true, true))
	{
		float dist = entdist(self, self->enemy);

		if (guncmdr_can_dodge_chain(self) && guncmdr_can_proactive_dodge(self, dist))
			guncmdr_start_fire_chain_dodge(self);
		else if (guncmdr_can_chain(self) && dist > GUNCMDR_CHAINGUN_RUN_RANGE)
			self->monsterinfo.currentmove = &guncmdr_move_fire_chain_run;
		else if (guncmdr_can_chain(self))
			self->monsterinfo.currentmove = &guncmdr_move_fire_chain;
		else
			self->monsterinfo.currentmove = &guncmdr_move_endfire_chain;
	}
	else if (guncmdr_can_chain(self))
		self->monsterinfo.currentmove = &guncmdr_move_fire_chain;
	else
		self->monsterinfo.currentmove = &guncmdr_move_endfire_chain;
}

static void guncmdr_refire_chain(edict_t *self)
{
	guncmdr_duck_up(self);

	if (G_ValidTarget(self, self->enemy, true, true) && visible(self, self->enemy) && random() <= 0.75)
	{
		float dist = entdist(self, self->enemy);

		if (guncmdr_can_dodge_chain(self) && guncmdr_can_proactive_dodge(self, dist) && random() < 0.65)
			guncmdr_start_fire_chain_dodge(self);
		else if (guncmdr_can_chain(self) && !(self->monsterinfo.aiflags & AI_STAND_GROUND) && dist > GUNCMDR_CHAINGUN_RUN_RANGE)
			self->monsterinfo.currentmove = &guncmdr_move_fire_chain_run;
		else if (guncmdr_can_chain(self))
			self->monsterinfo.currentmove = &guncmdr_move_fire_chain;
		else
			self->monsterinfo.currentmove = &guncmdr_move_endfire_chain;
	}
	else
		self->monsterinfo.currentmove = &guncmdr_move_endfire_chain;

	self->monsterinfo.attack_finished = level.time + 0.5;
}

static void guncmdr_grenade_finished(edict_t *self)
{
	guncmdr_duck_up(self);
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

static void guncmdr_pain5_to_death1(edict_t *self)
{
	if (self->health <= 0)
		self->monsterinfo.currentmove = &guncmdr_move_death1;
}

static void guncmdr_pain5_to_death2(edict_t *self)
{
	if (self->health <= 0 && random() < 0.5f)
		self->monsterinfo.currentmove = &guncmdr_move_death2;
}

mframe_t guncmdr_frames_pain4[] =
{
	ai_move, -17.1, NULL,
	ai_move, -3.2, NULL,
	ai_move, 0.9, NULL,
	ai_move, 3.6, NULL,
	ai_move, -2.6, NULL,
	ai_move, 1.0, NULL,
	ai_move, -5.1, NULL,
	ai_move, -6.7, NULL,
	ai_move, -8.8, NULL,
	ai_move, 0, NULL,

	ai_move, 0, NULL,
	ai_move, -2.1, NULL,
	ai_move, -2.3, NULL,
	ai_move, -2.5, NULL,
	ai_move, 0, NULL
};
mmove_t guncmdr_move_pain4 = { FRAME_c_pain401, FRAME_c_pain415, guncmdr_frames_pain4, guncmdr_run };

mframe_t guncmdr_frames_pain5[] =
{
	ai_move, -29, NULL,
	ai_move, -5, NULL,
	ai_move, -5, NULL,
	ai_move, -3, NULL,
	ai_move, 0, NULL,
	ai_move, 0, guncmdr_pain5_to_death2,
	ai_move, 9, NULL,
	ai_move, 3, NULL,
	ai_move, 0, guncmdr_pain5_to_death1,
	ai_move, 0, NULL,

	ai_move, 0, NULL,
	ai_move, -4.6, NULL,
	ai_move, -4.8, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 9.5, NULL,
	ai_move, 3.4, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,

	ai_move, -2.4, NULL,
	ai_move, -9.0, NULL,
	ai_move, -5.0, NULL,
	ai_move, -3.6, NULL
};
mmove_t guncmdr_move_pain5 = { FRAME_c_pain501, FRAME_c_pain524, guncmdr_frames_pain5, guncmdr_run };

static void guncmdr_pain6_to_death6(edict_t *self)
{
	if (self->health <= 0)
		self->monsterinfo.currentmove = &guncmdr_move_death6;
}

mframe_t guncmdr_frames_pain6[] =
{
	ai_move, 16, NULL,
	ai_move, 16, NULL,
	ai_move, 12, NULL,
	ai_move, 5.5, guncmdr_duck_down,
	ai_move, 3.0, NULL,
	ai_move, -4.7, NULL,
	ai_move, -6.0, guncmdr_pain6_to_death6,
	ai_move, 0, NULL,
	ai_move, 1.8, NULL,
	ai_move, 0.7, NULL,

	ai_move, 0, NULL,
	ai_move, -2.1, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,

	ai_move, 0, NULL,
	ai_move, -6.1, NULL,
	ai_move, 10.5, NULL,
	ai_move, 4.3, NULL,
	ai_move, 4.7, guncmdr_duck_up,
	ai_move, 1.4, NULL,
	ai_move, 0, NULL,
	ai_move, -3.2, NULL,
	ai_move, 2.3, NULL,
	ai_move, -4.4, NULL,

	ai_move, -4.4, NULL,
	ai_move, -2.4, NULL
};
mmove_t guncmdr_move_pain6 = { FRAME_c_pain601, FRAME_c_pain632, guncmdr_frames_pain6, guncmdr_run };

mframe_t guncmdr_frames_pain7[] =
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

	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t guncmdr_move_pain7 = { FRAME_c_pain701, FRAME_c_pain714, guncmdr_frames_pain7, guncmdr_run };

static void guncmdr_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	float r, dot = 1.0f;
	vec3_t forward, dir;

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 3;

	if (guncmdr_is_dodge_move(self))
		return;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3.0;
	gi.sound(self, CHAN_VOICE, (random() < 0.5) ? sound_pain : sound_pain2, 1, ATTN_NORM, 0);

	if (invasion->value == 2)
		return;

	guncmdr_duck_up(self);

	if (other)
	{
		AngleVectors(self->s.angles, forward, NULL, NULL);
		VectorSubtract(other->s.origin, self->s.origin, dir);
		dir[2] = 0;
		if (VectorNormalize(dir) != 0)
			dot = DotProduct(dir, forward);
	}

	if (damage < 35)
	{
		r = random();
		if (r < 0.25)
			self->monsterinfo.currentmove = &guncmdr_move_pain1;
		else if (r < 0.50)
			self->monsterinfo.currentmove = &guncmdr_move_pain2;
		else if (r < 0.75)
			self->monsterinfo.currentmove = &guncmdr_move_pain3;
		else
			self->monsterinfo.currentmove = &guncmdr_move_pain7;
	}
	else if (dot < -0.40f)
	{
		self->monsterinfo.currentmove = &guncmdr_move_pain6;
		guncmdr_duck_down(self);
		self->pain_debounce_time += 1.5;
	}
	else
	{
		if (random() < 0.5)
			self->monsterinfo.currentmove = &guncmdr_move_pain4;
		else
			self->monsterinfo.currentmove = &guncmdr_move_pain5;
		self->pain_debounce_time += 1.5;
	}
}

static void guncmdr_dead(edict_t *self)
{
	VectorSet(self->mins,
		GUNCMDR_MINS_X_SCALED(self),
		GUNCMDR_MINS_Y_SCALED(self),
		GUNCMDR_MINS_Z_SCALED(self));

	VectorSet(self->maxs,
		GUNCMDR_MAXS_X_SCALED(self),
		GUNCMDR_MAXS_Y_SCALED(self),
		GUNCMDR_DEAD_MAXZ_SCALED(self));

	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
	M_PrepBodyRemoval(self);
}

static void guncmdr_shrink(edict_t *self)
{
	self->maxs[2] = GUNCMDR_SHRINK_MAXZ_SCALED(self);
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}

static void guncmdr_ai_dodge_slide(edict_t *self, float dist)
{
	if (!G_EntIsAlive(self->enemy))
		return;

	drone_ai_dodge_slide(self, dist);
}

mframe_t guncmdr_frames_dodge_slide[] =
{
	guncmdr_ai_dodge_slide, 12, NULL,
	guncmdr_ai_dodge_slide, 10, NULL,
	guncmdr_ai_dodge_slide, 14, NULL,
	guncmdr_ai_dodge_slide, 13, NULL,
	guncmdr_ai_dodge_slide, 14, NULL,
	guncmdr_ai_dodge_slide, 10, NULL
};
mmove_t guncmdr_move_dodge_slide = { FRAME_c_run101, FRAME_c_run106, guncmdr_frames_dodge_slide, guncmdr_run };

static qboolean guncmdr_try_sidestep(edict_t *self)
{
	if (self->monsterinfo.currentmove == &guncmdr_move_fire_chain ||
		self->monsterinfo.currentmove == &guncmdr_move_fire_chain_run)
	{
		if (self->monsterinfo.lefty)
			self->monsterinfo.currentmove = &guncmdr_move_fire_chain_dodge_left;
		else
			self->monsterinfo.currentmove = &guncmdr_move_fire_chain_dodge_right;

		return true;
	}

	if (self->monsterinfo.currentmove == &guncmdr_move_attack_grenade_back)
	{
		self->count = self->s.frame;

		if (self->monsterinfo.lefty)
			self->monsterinfo.currentmove = &guncmdr_move_attack_grenade_back_dodge_left;
		else
			self->monsterinfo.currentmove = &guncmdr_move_attack_grenade_back_dodge_right;

		return true;
	}

	if (self->monsterinfo.currentmove == &guncmdr_move_attack_mortar)
	{
		self->count = self->s.frame;
		self->monsterinfo.currentmove = &guncmdr_move_attack_mortar_dodge;
		return true;
	}

	if (self->monsterinfo.currentmove == &guncmdr_move_run)
	{
		if (random() < 0.45f)
			return guncmdr_start_duckstep_dodge(self);

		self->monsterinfo.currentmove = &guncmdr_move_dodge_slide;
		return true;
	}

	return false;
}

static void guncmdr_footstep(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_thud, 1, ATTN_NORM, 0);
}

static void guncmdr_shrink_footstep(edict_t *self)
{
	guncmdr_footstep(self);
	guncmdr_shrink(self);
}

mframe_t guncmdr_frames_death1[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 4, NULL,
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
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t guncmdr_move_death1 = { FRAME_c_death101, FRAME_c_death118, guncmdr_frames_death1, guncmdr_dead };

mframe_t guncmdr_frames_death2[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t guncmdr_move_death2 = { FRAME_c_death201, FRAME_c_death204, guncmdr_frames_death2, guncmdr_dead };

mframe_t guncmdr_frames_death3[] =
{
	ai_move, 20, NULL,
	ai_move, 10, NULL,
	ai_move, 10, guncmdr_shrink_footstep,
	ai_move, 0, guncmdr_footstep,
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
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t guncmdr_move_death3 = { FRAME_c_death301, FRAME_c_death321, guncmdr_frames_death3, guncmdr_dead };

mframe_t guncmdr_frames_death4[] =
{
	ai_move, -20, NULL,
	ai_move, -16, NULL,
	ai_move, -26, guncmdr_shrink_footstep,
	ai_move, 0, guncmdr_footstep,
	ai_move, -12, NULL,
	ai_move, 16, NULL,
	ai_move, 9.2, NULL,
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
mmove_t guncmdr_move_death4 = { FRAME_c_death401, FRAME_c_death436, guncmdr_frames_death4, guncmdr_dead };

mframe_t guncmdr_frames_death5[] =
{
	ai_move, -14, NULL,
	ai_move, -2.7, NULL,
	ai_move, -2.5, NULL,
	ai_move, -4.6, guncmdr_footstep,
	ai_move, -4.0, guncmdr_footstep,
	ai_move, -1.5, NULL,
	ai_move, 2.3, NULL,
	ai_move, 2.5, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 3.5, NULL,
	ai_move, 12.9, guncmdr_footstep,
	ai_move, 3.8, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, -2.1, NULL,
	ai_move, -1.3, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 3.4, NULL,
	ai_move, 5.7, NULL,
	ai_move, 11.2, NULL,
	ai_move, 0, guncmdr_footstep
};
mmove_t guncmdr_move_death5 = { FRAME_c_death501, FRAME_c_death528, guncmdr_frames_death5, guncmdr_dead };

mframe_t guncmdr_frames_death6[] =
{
	ai_move, 0, guncmdr_shrink,
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
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t guncmdr_move_death6 = { FRAME_c_death601, FRAME_c_death614, guncmdr_frames_death6, guncmdr_dead };

mframe_t guncmdr_frames_death7[] =
{
	ai_move, 30, NULL,
	ai_move, 20, NULL,
	ai_move, 16, guncmdr_shrink_footstep,
	ai_move, 5, guncmdr_footstep,
	ai_move, -6, NULL,
	ai_move, -7, guncmdr_footstep,
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
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, guncmdr_footstep,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, guncmdr_footstep,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t guncmdr_move_death7 = { FRAME_c_death701, FRAME_c_death730, guncmdr_frames_death7, guncmdr_dead };

static void guncmdr_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int n;
	vec3_t forward, dir;
	float dot = 0;
	edict_t *head;

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

	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	vrx_update_drone_death_skin(self);

	if (self->monsterinfo.currentmove == &guncmdr_move_pain5 &&
		self->s.frame < FRAME_c_pain508)
	{
		if (self->activator && !self->activator->client)
			self->activator->num_monsters_real--;
		return;
	}
	if (self->monsterinfo.currentmove == &guncmdr_move_pain6 &&
		self->s.frame < FRAME_c_pain607)
	{
		if (self->activator && !self->activator->client)
			self->activator->num_monsters_real--;
		return;
	}

	if (inflictor)
	{
		AngleVectors(self->s.angles, forward, NULL, NULL);
		VectorSubtract(inflictor->s.origin, self->s.origin, dir);
		dir[2] = 0;
		VectorNormalize(dir);
		dot = DotProduct(dir, forward);
	}

	if (point[2] >= self->s.origin[2] + self->maxs[2] - 8 && self->velocity[2] < 65)
	{
		if (vrx_spawn_nonessential_ent(self->s.origin))
		{
			head = ThrowGibEx(self, "models/monsters/gunner/gibs/head.md2", damage, GIB_SKINNED | GIB_HEAD,
				self->s.scale ? self->s.scale : 1.0f);
			if (head)
				head->s.skinnum /= 2;
		}
		self->monsterinfo.currentmove = &guncmdr_move_death5;
	}
	else if (dot < -0.40)
	{
		n = GetRandom(0, self->monsterinfo.currentmove == &guncmdr_move_pain6 ? 2 : 3);
		if (n == 0)
			self->monsterinfo.currentmove = &guncmdr_move_death3;
		else if (n == 1)
			self->monsterinfo.currentmove = &guncmdr_move_death7;
		else if (n == 2)
			self->monsterinfo.currentmove = &guncmdr_move_death6;
		else
			self->monsterinfo.currentmove = &guncmdr_move_pain6;
	}
	else
	{
		n = GetRandom(0, self->monsterinfo.currentmove == &guncmdr_move_pain5 ? 1 : 2);
		if (n == 0)
			self->monsterinfo.currentmove = &guncmdr_move_death4;
		else
			self->monsterinfo.currentmove = &guncmdr_move_pain5;
	}

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
	gi.modelindex("models/proj/flechette/tris.md2");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/gunner/tris.md2");
	gi.modelindex("models/monsters/gunner/gibs/chest.md2");
	gi.modelindex("models/monsters/gunner/gibs/foot.md2");
	gi.modelindex("models/monsters/gunner/gibs/garm.md2");
	gi.modelindex("models/monsters/gunner/gibs/gun.md2");
	gi.modelindex("models/monsters/gunner/gibs/head.md2");

	self->s.scale = 1.25f;
	guncmdr_set_stand_bbox(self); // base bbox is -16 -16 -24, 16 16 36 at scale 1.0
	self->s.skinnum = 2;

	self->monsterinfo.control_cost = M_TANK_CONTROL_COST;
	self->monsterinfo.cost = M_TANK_COST;
	self->health = M_GUNCMDR_INITIAL_HEALTH + M_GUNCMDR_ADDON_HEALTH * self->monsterinfo.level;
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

	M_SetMonsterArmor(self, M_GUNCMDR_INITIAL_ARMOR + M_GUNCMDR_ADDON_ARMOR * self->monsterinfo.level);
	self->mtype = M_GUNCMDR;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &guncmdr_move_stand;
	self->monsterinfo.scale = MODEL_SCALE;
	self->nextthink = level.time + FRAMETIME;
}
