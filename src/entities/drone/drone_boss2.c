/*
==============================================================================

boss2

==============================================================================
*/

#include "g_local.h"
#include "../../quake2/monsterframes/m_boss2.h"

#define BOSS2_VARIANT_MG		0
#define BOSS2_VARIANT_HYPER		1
#define BOSS2_VARIANT_SMALL		2
#define BOSS2_ROCKET_SPEED		750
#define BOSS2_INVASION_SCALE		0.75f
#define BOSS2_INVASION_MOVE_SCALE	1.5f

static int sound_pain1;
static int sound_pain2;
static int sound_pain3;
static int sound_death;
static int sound_search1;

void drone_ai_stand(edict_t *self, float dist);
void drone_ai_walk(edict_t *self, float dist);
void drone_ai_run(edict_t *self, float dist);

static void boss2_run(edict_t *self);
static void boss2_attack_mg(edict_t *self);
static void boss2_reattack_mg(edict_t *self);

static qboolean boss2_is_small(const edict_t *self)
{
	return self->style == BOSS2_VARIANT_SMALL;
}

static qboolean boss2_is_hyper(const edict_t *self)
{
	return self->style == BOSS2_VARIANT_HYPER || boss2_is_small(self);
}

static float boss2_move_scale(const edict_t *self)
{
	return (invasion->value && !boss2_is_small(self)) ? BOSS2_INVASION_MOVE_SCALE : 1.0f;
}

static void boss2_ai_walk(edict_t *self, float dist)
{
	drone_ai_walk(self, dist * boss2_move_scale(self));
}

static void boss2_ai_run(edict_t *self, float dist)
{
	drone_ai_run(self, dist * boss2_move_scale(self));
}

static float boss2_voice_attenuation(const edict_t *self)
{
	return boss2_is_small(self) ? ATTN_NORM : ATTN_NONE;
}

static void boss2_project_flash(edict_t *self, int flash, vec3_t start)
{
	vec3_t forward, right;
	vec3_t offset;

	AngleVectors(self->s.angles, forward, right, NULL);
	VectorCopy(monster_flash_offset[flash], offset);
	if (self->s.scale && self->s.scale != 1.0f)
		VectorScale(offset, self->s.scale, offset);
	G_ProjectSource(self->s.origin, offset, forward, right, start);
}

static void boss2_predict_aim(edict_t *self, vec3_t start, int speed, float lead, vec3_t dir)
{
	vec3_t target;
	float dist;
	float time;

	if (!G_ValidTarget(self, self->enemy, false, true))
	{
		AngleVectors(self->s.angles, dir, NULL, NULL);
		return;
	}

	G_EntMidPoint(self->enemy, target);
	if (speed > 0)
	{
		VectorSubtract(target, start, dir);
		dist = VectorLength(dir);
		time = dist / speed + lead;
		if (time < 0)
			time = 0;
		VectorMA(target, time, self->enemy->velocity, target);
	}

	VectorSubtract(target, start, dir);
	if (!VectorNormalize(dir))
		AngleVectors(self->s.angles, dir, NULL, NULL);
}

static void boss2_search(edict_t *self)
{
	if (random() < 0.5f)
		gi.sound(self, CHAN_VOICE, sound_search1, 1, boss2_voice_attenuation(self), 0);
}

static void boss2_explode(edict_t *self)
{
	vec3_t org;

	VectorCopy(self->s.origin, org);
	org[0] += crandom() * self->maxs[0];
	org[1] += crandom() * self->maxs[1];
	org[2] += crandom() * self->maxs[2];

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1_BIG);
	gi.WritePosition(org);
	gi.multicast(self->s.origin, MULTICAST_PVS);
}

static void boss2_dead(edict_t *self)
{
	int n;

	for (n = 0; n < 3; n++)
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", 500, GIB_ORGANIC);
	for (n = 0; n < 5; n++)
		ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2", 500, GIB_METALLIC);

	ThrowGib(self, "models/monsters/boss2/gibs/chest.md2", 500, GIB_METALLIC);
	ThrowGib(self, "models/monsters/boss2/gibs/chaingun.md2", 500, GIB_METALLIC);
	ThrowGib(self, "models/monsters/boss2/gibs/cpu.md2", 500, GIB_METALLIC);
	ThrowGib(self, "models/monsters/boss2/gibs/engine.md2", 500, GIB_METALLIC);
	ThrowGib(self, "models/monsters/boss2/gibs/rocket.md2", 500, GIB_METALLIC);
	ThrowGib(self, "models/monsters/boss2/gibs/spine.md2", 500, GIB_METALLIC);
	ThrowGib(self, "models/monsters/boss2/gibs/wing.md2", 500, GIB_METALLIC);
	ThrowGib(self, "models/monsters/boss2/gibs/larm.md2", 500, GIB_METALLIC);
	ThrowGib(self, "models/monsters/boss2/gibs/rarm.md2", 500, GIB_METALLIC);
	ThrowHead(self, "models/monsters/boss2/gibs/head.md2", 500, GIB_METALLIC);

	boss2_explode(self);
	M_Remove(self, false, false);
}

static void boss2_fire_predictive_rocket(edict_t *self, int flash, float lead)
{
	vec3_t start, dir;

	if (!G_ValidTarget(self, self->enemy, false, true))
		return;

	boss2_project_flash(self, flash, start);
	boss2_predict_aim(self, start, BOSS2_ROCKET_SPEED, lead, dir);
	monster_fire_rocket(self, start, dir, 50, BOSS2_ROCKET_SPEED, flash);
}

static void Boss2PredictiveRocket(edict_t *self)
{
	boss2_fire_predictive_rocket(self, MZ2_BOSS2_ROCKET_1, -0.10f);
	boss2_fire_predictive_rocket(self, MZ2_BOSS2_ROCKET_2, -0.05f);
	boss2_fire_predictive_rocket(self, MZ2_BOSS2_ROCKET_3, 0.05f);
	boss2_fire_predictive_rocket(self, MZ2_BOSS2_ROCKET_4, 0.10f);
}

static void boss2_fire_spread_rocket(edict_t *self, int flash, float zofs, float rightofs)
{
	vec3_t forward, right;
	vec3_t start, dir, target;

	if (!G_ValidTarget(self, self->enemy, false, true))
		return;

	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, monster_flash_offset[flash], forward, right, start);
	VectorCopy(self->enemy->s.origin, target);
	target[2] += zofs;
	VectorSubtract(target, start, dir);
	if (!VectorNormalize(dir))
		VectorCopy(forward, dir);
	VectorMA(dir, rightofs, right, dir);
	VectorNormalize(dir);
	monster_fire_rocket(self, start, dir, 50, 500, flash);
}

static void Boss2Rocket(edict_t *self)
{
	if (!G_ValidTarget(self, self->enemy, false, true))
		return;

	if (self->enemy->client && random() < 0.9f)
	{
		Boss2PredictiveRocket(self);
		return;
	}

	boss2_fire_spread_rocket(self, MZ2_BOSS2_ROCKET_1, -15, 0.4f);
	boss2_fire_spread_rocket(self, MZ2_BOSS2_ROCKET_2, 0, 0.025f);
	boss2_fire_spread_rocket(self, MZ2_BOSS2_ROCKET_3, 0, -0.025f);
	boss2_fire_spread_rocket(self, MZ2_BOSS2_ROCKET_4, -15, -0.4f);
}

static void Boss2Rocket64(edict_t *self)
{
	vec3_t forward, right;
	vec3_t start, dir, target;
	float scale;

	if (!G_ValidTarget(self, self->enemy, false, true))
		return;

	AngleVectors(self->s.angles, forward, right, NULL);
	boss2_project_flash(self, MZ2_BOSS2_ROCKET_1, start);

	scale = self->s.scale ? self->s.scale : 1.0f;
	start[2] += 10.0f * scale;
	VectorMA(start, -2.0f * scale, right, start);
	VectorMA(start, -((self->count++ % 4) * 8.0f * scale), right, start);

	if (self->enemy->client && random() < 0.9f)
		boss2_predict_aim(self, start, BOSS2_ROCKET_SPEED, -0.3f, dir);
	else
	{
		VectorCopy(self->enemy->s.origin, target);
		target[2] -= 15;
		VectorSubtract(target, start, dir);
		if (!VectorNormalize(dir))
			VectorCopy(forward, dir);
	}

	monster_fire_rocket(self, start, dir, 35, BOSS2_ROCKET_SPEED, MZ2_BOSS2_ROCKET_1);
}

static void boss2_firebullet(edict_t *self, int flash)
{
	vec3_t start, dir;

	if (!G_ValidTarget(self, self->enemy, false, true))
		return;

	boss2_project_flash(self, flash, start);
	boss2_predict_aim(self, start, 0, 0, dir);
	monster_fire_bullet(self, start, dir, 6, 4, DEFAULT_BULLET_HSPREAD * 3, DEFAULT_BULLET_VSPREAD, flash);
}

static void Boss2MachineGun(edict_t *self)
{
	boss2_firebullet(self, MZ2_BOSS2_MACHINEGUN_L1);
	boss2_firebullet(self, MZ2_BOSS2_MACHINEGUN_R1);
}

static void Boss2HyperBlaster(edict_t *self)
{
	int flash;
	int effect;
	vec3_t start, dir;

	if (!G_ValidTarget(self, self->enemy, false, true))
		return;

	flash = (self->s.frame & 1) ? MZ2_BOSS2_MACHINEGUN_L2 : MZ2_BOSS2_MACHINEGUN_R2;
	effect = (self->s.frame % 4) ? 0 : EF_HYPERBLASTER;
	boss2_project_flash(self, flash, start);
	boss2_predict_aim(self, start, 1000, 0, dir);
	monster_fire_blaster(self, start, dir, 2, 1000, effect, BLASTER_PROJ_BOLT, 2.0f, false, flash);
}

static void Boss2HyperBlasterReattack(edict_t *self)
{
	Boss2HyperBlaster(self);
	boss2_reattack_mg(self);
}

static mframe_t boss2_frames_stand[] =
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
	drone_ai_stand, 0, NULL
};
static mmove_t boss2_move_stand = { FRAME_stand30, FRAME_stand50, boss2_frames_stand, NULL };

static mframe_t boss2_frames_walk[] =
{
	boss2_ai_walk, 10, NULL,
	boss2_ai_walk, 10, NULL,
	boss2_ai_walk, 10, NULL,
	boss2_ai_walk, 10, NULL,
	boss2_ai_walk, 10, NULL,
	boss2_ai_walk, 10, NULL,
	boss2_ai_walk, 10, NULL,
	boss2_ai_walk, 10, NULL,
	boss2_ai_walk, 10, NULL,
	boss2_ai_walk, 10, NULL,
	boss2_ai_walk, 10, NULL,
	boss2_ai_walk, 10, NULL,
	boss2_ai_walk, 10, NULL,
	boss2_ai_walk, 10, NULL,
	boss2_ai_walk, 10, NULL,
	boss2_ai_walk, 10, NULL,
	boss2_ai_walk, 10, NULL,
	boss2_ai_walk, 10, NULL,
	boss2_ai_walk, 10, NULL,
	boss2_ai_walk, 10, NULL
};
static mmove_t boss2_move_walk = { FRAME_walk1, FRAME_walk20, boss2_frames_walk, NULL };

static mframe_t boss2_frames_run[] =
{
	boss2_ai_run, 10, NULL,
	boss2_ai_run, 10, NULL,
	boss2_ai_run, 10, NULL,
	boss2_ai_run, 10, NULL,
	boss2_ai_run, 10, NULL,
	boss2_ai_run, 10, NULL,
	boss2_ai_run, 10, NULL,
	boss2_ai_run, 10, NULL,
	boss2_ai_run, 10, NULL,
	boss2_ai_run, 10, NULL,
	boss2_ai_run, 10, NULL,
	boss2_ai_run, 10, NULL,
	boss2_ai_run, 10, NULL,
	boss2_ai_run, 10, NULL,
	boss2_ai_run, 10, NULL,
	boss2_ai_run, 10, NULL,
	boss2_ai_run, 10, NULL,
	boss2_ai_run, 10, NULL,
	boss2_ai_run, 10, NULL,
	boss2_ai_run, 10, NULL
};
static mmove_t boss2_move_run = { FRAME_walk1, FRAME_walk20, boss2_frames_run, NULL };

static mframe_t boss2_frames_attack_pre_mg[] =
{
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, boss2_attack_mg
};
static mmove_t boss2_move_attack_pre_mg = { FRAME_attack1, FRAME_attack9, boss2_frames_attack_pre_mg, NULL };

static mframe_t boss2_frames_attack_mg[] =
{
	ai_charge, 2, Boss2MachineGun,
	ai_charge, 2, Boss2MachineGun,
	ai_charge, 2, Boss2MachineGun,
	ai_charge, 2, Boss2MachineGun,
	ai_charge, 2, Boss2MachineGun,
	ai_charge, 2, boss2_reattack_mg
};
static mmove_t boss2_move_attack_mg = { FRAME_attack10, FRAME_attack15, boss2_frames_attack_mg, NULL };

static mframe_t boss2_frames_attack_hb[] =
{
	ai_charge, 2, Boss2HyperBlaster,
	ai_charge, 2, Boss2HyperBlaster,
	ai_charge, 2, Boss2HyperBlaster,
	ai_charge, 2, Boss2HyperBlaster,
	ai_charge, 2, Boss2HyperBlaster,
	ai_charge, 2, Boss2HyperBlasterReattack
};
static mmove_t boss2_move_attack_hb = { FRAME_attack10, FRAME_attack15, boss2_frames_attack_hb, NULL };

static mframe_t boss2_frames_attack_post_mg[] =
{
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL
};
static mmove_t boss2_move_attack_post_mg = { FRAME_attack16, FRAME_attack19, boss2_frames_attack_post_mg, boss2_run };

static mframe_t boss2_frames_attack_rocket[] =
{
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_move, -5, Boss2Rocket,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL
};
static mmove_t boss2_move_attack_rocket = { FRAME_attack20, FRAME_attack40, boss2_frames_attack_rocket, boss2_run };

static mframe_t boss2_frames_attack_rocket2[] =
{
	ai_charge, 2, Boss2Rocket64,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, Boss2Rocket64,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, Boss2Rocket64,
	ai_charge, 2, NULL,
	ai_charge, 2, Boss2Rocket64,
	ai_charge, 2, NULL,
	ai_charge, 2, Boss2Rocket64,
	ai_charge, 2, Boss2Rocket64,
	ai_charge, 2, Boss2Rocket64,
	ai_charge, 2, Boss2Rocket64,
	ai_charge, 2, Boss2Rocket64,
	ai_charge, 2, Boss2Rocket64,
	ai_charge, 2, Boss2Rocket64,
	ai_charge, 2, Boss2Rocket64
};
static mmove_t boss2_move_attack_rocket2 = { FRAME_attack20, FRAME_attack39, boss2_frames_attack_rocket2, boss2_run };

static mframe_t boss2_frames_pain_heavy[] =
{
	ai_move, -2, NULL,
	ai_move, -2, NULL,
	ai_move, -2, NULL,
	ai_move, -2, NULL,
	ai_move, -2, NULL,
	ai_move, -2, NULL,
	ai_move, -2, NULL,
	ai_move, -2, NULL,
	ai_move, -2, NULL,
	ai_move, -2, NULL,
	ai_move, -2, NULL,
	ai_move, -2, NULL,
	ai_move, -2, NULL,
	ai_move, -2, NULL,
	ai_move, -2, NULL,
	ai_move, -2, NULL,
	ai_move, -2, NULL,
	ai_move, -2, NULL
};
static mmove_t boss2_move_pain_heavy = { FRAME_pain2, FRAME_pain19, boss2_frames_pain_heavy, boss2_run };

static mframe_t boss2_frames_pain_light[] =
{
	ai_move, -2, NULL,
	ai_move, -2, NULL,
	ai_move, -2, NULL,
	ai_move, -2, NULL
};
static mmove_t boss2_move_pain_light = { FRAME_pain20, FRAME_pain23, boss2_frames_pain_light, boss2_run };

static void boss2_shrink(edict_t *self)
{
	self->maxs[2] = boss2_is_small(self) ? 30 : 50;
	gi.linkentity(self);
}

static mframe_t boss2_frames_death[] =
{
	ai_move, 0, boss2_explode,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, boss2_shrink,
	ai_move, 0, NULL,
};
static mmove_t boss2_move_death = { FRAME_death2, FRAME_death10, boss2_frames_death, boss2_dead };

static mframe_t boss2_frames_deathboss[] =
{
	ai_move, 0, boss2_explode,
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
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, boss2_shrink,
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
static mmove_t boss2_move_deathboss = { FRAME_death2, FRAME_death50, boss2_frames_deathboss, boss2_dead };

static void boss2_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &boss2_move_stand;
}

static void boss2_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &boss2_move_stand;
	else
		self->monsterinfo.currentmove = &boss2_move_run;
}

static void boss2_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &boss2_move_walk;
}

static void boss2_attack(edict_t *self)
{
	vec3_t delta;
	float range;

	if (!G_ValidTarget(self, self->enemy, false, true))
		return;

	VectorSubtract(self->enemy->s.origin, self->s.origin, delta);
	range = VectorLength(delta);

	if (range <= 125 || random() <= 0.6f)
		self->monsterinfo.currentmove = boss2_is_hyper(self) ? &boss2_move_attack_hb : &boss2_move_attack_pre_mg;
	else
		self->monsterinfo.currentmove = boss2_is_hyper(self) ? &boss2_move_attack_rocket2 : &boss2_move_attack_rocket;

	M_DelayNextAttack(self, 1.0f + random() * 0.5f, true);
}

static void boss2_attack_mg(edict_t *self)
{
	self->monsterinfo.currentmove = boss2_is_hyper(self) ? &boss2_move_attack_hb : &boss2_move_attack_mg;
}

static void boss2_reattack_mg(edict_t *self)
{
	if (G_ValidTarget(self, self->enemy, false, true) && infront(self, self->enemy) && random() <= 0.7f)
		boss2_attack_mg(self);
	else
		self->monsterinfo.currentmove = &boss2_move_attack_post_mg;
}

static void boss2_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	(void)other;
	(void)kick;

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3.0f;
	if (damage < 10)
		gi.sound(self, CHAN_VOICE, sound_pain3, 1, ATTN_NORM, 0);
	else if (damage < 30)
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);

	if (skill->value == 3)
		return;
	if (damage < 30)
		self->monsterinfo.currentmove = &boss2_move_pain_light;
	else
		self->monsterinfo.currentmove = &boss2_move_pain_heavy;
}

static void boss2_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	(void)inflictor;
	(void)attacker;
	(void)damage;
	(void)point;

	M_Notify(self);

	if (self->deadflag == DEAD_DEAD)
		return;

	DroneList_Remove(self);
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->s.sound = 0;
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	self->count = 0;
	self->monsterinfo.currentmove = boss2_is_small(self) ? &boss2_move_death : &boss2_move_deathboss;
}

static void init_drone_boss2_common(edict_t *self, int variant)
{
	qboolean hyper = (variant == BOSS2_VARIANT_HYPER || variant == BOSS2_VARIANT_SMALL);
	qboolean small = (variant == BOSS2_VARIANT_SMALL);

	sound_pain1 = gi.soundindex("bosshovr/bhvpain1.wav");
	sound_pain2 = gi.soundindex("bosshovr/bhvpain2.wav");
	sound_pain3 = gi.soundindex("bosshovr/bhvpain3.wav");
	sound_death = gi.soundindex("bosshovr/bhvdeth1.wav");
	sound_search1 = gi.soundindex("bosshovr/bhvunqv1.wav");
	gi.soundindex("tank/rocket.wav");
	gi.soundindex(hyper ? "flyer/flyatck3.wav" : "infantry/infatck1.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/boss2/tris.md2");
	if (small)
	{
		VectorSet(self->mins, -34, -34, 0);
		VectorSet(self->maxs, 34, 34, 48);
	}
	else if (invasion->value)
	{
		VectorSet(self->mins, -42, -42, 0);
		VectorSet(self->maxs, 42, 42, 60);
	}
	else
	{
		VectorSet(self->mins, -56, -56, 0);
		VectorSet(self->maxs, 56, 56, 80);
	}

	gi.modelindex("models/monsters/boss2/gibs/chaingun.md2");
	gi.modelindex("models/monsters/boss2/gibs/chest.md2");
	gi.modelindex("models/monsters/boss2/gibs/cpu.md2");
	gi.modelindex("models/monsters/boss2/gibs/engine.md2");
	gi.modelindex("models/monsters/boss2/gibs/head.md2");
	gi.modelindex("models/monsters/boss2/gibs/larm.md2");
	gi.modelindex("models/monsters/boss2/gibs/rarm.md2");
	gi.modelindex("models/monsters/boss2/gibs/rocket.md2");
	gi.modelindex("models/monsters/boss2/gibs/spine.md2");
	gi.modelindex("models/monsters/boss2/gibs/wing.md2");

	if (small)
		self->health = M_BOSS2_SMALL_INITIAL_HEALTH + M_BOSS2_SMALL_ADDON_HEALTH * self->monsterinfo.level;
	else
		self->health = M_BOSS2_INITIAL_HEALTH + M_BOSS2_ADDON_HEALTH * self->monsterinfo.level;
	self->max_health = self->health;
	self->gib_health = -200;
	self->mass = small ? 1000 : 2000;
	self->mtype = small ? M_BOSS2_SMALL : M_BOSS2;
	self->style = variant;
	self->yaw_speed = small ? 80 : 50;
	self->flags |= FL_FLY | FL_IMMUNE_LASER;
	self->s.sound = gi.soundindex("bosshovr/bhvengn1.wav");
	self->s.scale = small ? 0.6f : (invasion->value ? BOSS2_INVASION_SCALE : 1.0f);

	self->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;
	if (small)
		self->monsterinfo.power_armor_power = M_BOSS2_SMALL_INITIAL_ARMOR + M_BOSS2_SMALL_ADDON_ARMOR * self->monsterinfo.level;
	else
		self->monsterinfo.power_armor_power = M_BOSS2_INITIAL_ARMOR + M_BOSS2_ADDON_ARMOR * self->monsterinfo.level;
	self->monsterinfo.max_armor = self->monsterinfo.power_armor_power;
	self->monsterinfo.control_cost = small ? M_HOVER_CONTROL_COST : M_JORG_CONTROL_COST;
	self->monsterinfo.cost = small ? M_HOVER_COST : M_COMMANDER_COST;
	self->monsterinfo.jumpup = 64;
	self->monsterinfo.jumpdn = 512;
	self->monsterinfo.sight_range = 1024;
	self->monsterinfo.aiflags |= AI_NO_CIRCLE_STRAFE;

	self->pain = boss2_pain;
	self->die = boss2_die;
	self->monsterinfo.stand = boss2_stand;
	self->monsterinfo.walk = boss2_walk;
	self->monsterinfo.run = boss2_run;
	self->monsterinfo.attack = boss2_attack;
	self->monsterinfo.idle = boss2_search;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &boss2_move_stand;
	self->monsterinfo.scale = MODEL_SCALE * self->s.scale;
	self->nextthink = level.time + FRAMETIME;

	if (!small && !invasion->value)
		G_PrintGreenText(va("A level %d hornet%s has spawned!", self->monsterinfo.level, hyper ? " hyper" : ""));
}

void init_drone_boss2(edict_t *self)
{
	init_drone_boss2_common(self, BOSS2_VARIANT_MG);
}

void init_drone_boss2_hyper(edict_t *self)
{
	init_drone_boss2_common(self, BOSS2_VARIANT_HYPER);
}

void init_drone_boss2_small(edict_t *self)
{
	init_drone_boss2_common(self, BOSS2_VARIANT_SMALL);
}
