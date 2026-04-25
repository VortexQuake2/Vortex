/*
==============================================================================

black widow 2

==============================================================================
*/

#include "g_local.h"

#define WIDOW2_FRAME_blackwidow3	0
#define WIDOW2_FRAME_walk01			1
#define WIDOW2_FRAME_walk09			9
#define WIDOW2_FRAME_spawn01		10
#define WIDOW2_FRAME_spawn04		13
#define WIDOW2_FRAME_spawn14		23
#define WIDOW2_FRAME_spawn18		27
#define WIDOW2_FRAME_firea01		28
#define WIDOW2_FRAME_firea07		34
#define WIDOW2_FRAME_fireb01		35
#define WIDOW2_FRAME_fireb04		38
#define WIDOW2_FRAME_fireb05		39
#define WIDOW2_FRAME_fireb09		43
#define WIDOW2_FRAME_tongs01		47
#define WIDOW2_FRAME_tongs08		54
#define WIDOW2_FRAME_pain01			55
#define WIDOW2_FRAME_pain05			59
#define WIDOW2_FRAME_death01		60
#define WIDOW2_FRAME_death44		103

#define WIDOW2_SUMMON_COUNT			2
#define WIDOW2_SUMMON_COOLDOWN		10.0f
#define WIDOW2_MELEE_RANGE			256.0f

static int sound_pain1;
static int sound_pain2;
static int sound_pain3;
static int sound_death;
static int sound_search;
static int sound_tongue;
static int sound_step;
static int sound_beam;
static int sound_hit;
static int sound_spawn;

static vec3_t widow2_tongue_offsets[] =
{
	{ 17.48f, 0.10f, 68.92f },
	{ 17.47f, 0.29f, 68.91f },
	{ 17.45f, 0.53f, 68.87f },
	{ 17.42f, 0.78f, 68.81f },
	{ 17.39f, 1.02f, 68.75f },
	{ 17.37f, 1.20f, 68.70f },
	{ 17.36f, 1.24f, 68.71f },
	{ 17.37f, 1.21f, 68.72f }
};

void drone_ai_stand(edict_t *self, float dist);
void drone_ai_run(edict_t *self, float dist);
void drone_ai_walk(edict_t *self, float dist);

static void widow2_stand(edict_t *self);
static void widow2_walk(edict_t *self);
static void widow2_run(edict_t *self);
static void widow2_attack(edict_t *self);
static void widow2_fire_beam(edict_t *self);
static void widow2_fire_disruptor(edict_t *self);
static void widow2_attack_beam(edict_t *self);
static void widow2_show_proboscis(edict_t *self);
static void widow2_pull_proboscis(edict_t *self);
static void widow2_melee_hit(edict_t *self);
static void widow2_spawn_effects(edict_t *self);
static void widow2_finish_spawn(edict_t *self);
static void widow2_explode(edict_t *self);
static void widow2_dead(edict_t *self);
static void widow2_step(edict_t *self);

static mframe_t widow2_frames_stand[] =
{
	drone_ai_stand, 0, NULL
};
static mmove_t widow2_move_stand = { WIDOW2_FRAME_blackwidow3, WIDOW2_FRAME_blackwidow3, widow2_frames_stand, widow2_stand };

static mframe_t widow2_frames_walk[] =
{
	drone_ai_walk, 9, widow2_step,
	drone_ai_walk, 8, NULL,
	drone_ai_walk, 7, NULL,
	drone_ai_walk, 7, NULL,
	drone_ai_walk, 6, NULL,
	drone_ai_walk, 6, widow2_step,
	drone_ai_walk, 7, NULL,
	drone_ai_walk, 8, NULL,
	drone_ai_walk, 10, NULL
};
static mmove_t widow2_move_walk = { WIDOW2_FRAME_walk01, WIDOW2_FRAME_walk09, widow2_frames_walk, widow2_walk };

static mframe_t widow2_frames_run[] =
{
	drone_ai_run, 9, widow2_step,
	drone_ai_run, 8, NULL,
	drone_ai_run, 7, NULL,
	drone_ai_run, 7, NULL,
	drone_ai_run, 6, NULL,
	drone_ai_run, 6, widow2_step,
	drone_ai_run, 7, NULL,
	drone_ai_run, 8, NULL,
	drone_ai_run, 10, NULL
};
static mmove_t widow2_move_run = { WIDOW2_FRAME_walk01, WIDOW2_FRAME_walk09, widow2_frames_run, NULL };

static mframe_t widow2_frames_pre_beam[] =
{
	ai_charge, 4, NULL,
	ai_charge, 4, widow2_step,
	ai_charge, 4, NULL,
	ai_charge, 4, widow2_attack_beam
};
static mmove_t widow2_move_pre_beam = { WIDOW2_FRAME_fireb01, WIDOW2_FRAME_fireb04, widow2_frames_pre_beam, NULL };

static mframe_t widow2_frames_beam[] =
{
	ai_charge, 0, widow2_fire_beam,
	ai_charge, 0, widow2_fire_beam,
	ai_charge, 0, widow2_fire_beam,
	ai_charge, 0, widow2_fire_beam,
	ai_charge, 0, widow2_fire_beam
};
static mmove_t widow2_move_beam = { WIDOW2_FRAME_fireb05, WIDOW2_FRAME_fireb09, widow2_frames_beam, widow2_run };

static mframe_t widow2_frames_disruptor[] =
{
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, -20, widow2_fire_disruptor,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL
};
static mmove_t widow2_move_disruptor = { WIDOW2_FRAME_firea01, WIDOW2_FRAME_firea07, widow2_frames_disruptor, widow2_run };

static mframe_t widow2_frames_spawn[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, widow2_step,
	ai_charge, 0, widow2_fire_beam,
	ai_charge, 0, NULL,
	ai_charge, 0, widow2_fire_beam,
	ai_charge, 0, NULL,
	ai_charge, 0, widow2_fire_beam,
	ai_charge, 0, NULL,
	ai_charge, 0, widow2_spawn_effects,
	ai_charge, 0, NULL,
	ai_charge, 0, widow2_fire_beam,
	ai_charge, 0, NULL,
	ai_charge, 0, widow2_finish_spawn,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
static mmove_t widow2_move_spawn = { WIDOW2_FRAME_spawn01, WIDOW2_FRAME_spawn18, widow2_frames_spawn, widow2_run };

static mframe_t widow2_frames_tongs[] =
{
	ai_charge, 0, widow2_show_proboscis,
	ai_charge, 0, widow2_show_proboscis,
	ai_charge, 0, widow2_show_proboscis,
	ai_charge, 0, widow2_pull_proboscis,
	ai_charge, 0, widow2_pull_proboscis,
	ai_charge, 0, widow2_pull_proboscis,
	ai_charge, 0, widow2_melee_hit,
	ai_charge, 0, NULL
};
static mmove_t widow2_move_tongs = { WIDOW2_FRAME_tongs01, WIDOW2_FRAME_tongs08, widow2_frames_tongs, widow2_run };

static mframe_t widow2_frames_pain[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
static mmove_t widow2_move_pain = { WIDOW2_FRAME_pain01, WIDOW2_FRAME_pain05, widow2_frames_pain, widow2_run };

static mframe_t widow2_frames_death[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, widow2_explode,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, widow2_explode,
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
	ai_move, 0, widow2_explode,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, widow2_explode,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, widow2_explode,
	ai_move, 0, widow2_explode,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, widow2_explode,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, widow2_explode,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, widow2_dead
};
static mmove_t widow2_move_death = { WIDOW2_FRAME_death01, WIDOW2_FRAME_death44, widow2_frames_death, NULL };

static void widow2_step(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_step, 1, ATTN_NORM, 0);
}

static void widow2_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

static void widow2_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &widow2_move_stand;
}

static void widow2_walk(edict_t *self)
{
	if (!self->goalentity)
		self->goalentity = world;
	self->monsterinfo.currentmove = &widow2_move_walk;
}

static void widow2_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &widow2_move_stand;
	else
		self->monsterinfo.currentmove = &widow2_move_run;
}

static qboolean widow2_can_melee(edict_t *self)
{
	return G_EntExists(self->enemy) && entdist(self, self->enemy) <= WIDOW2_MELEE_RANGE;
}

static void widow2_fire_beam(edict_t *self)
{
	int damage;
	int flash;
	vec3_t forward, start;

	if (!G_EntExists(self->enemy))
		return;

	damage = M_RAILGUN_DMG_BASE + M_RAILGUN_DMG_ADDON * drone_damagelevel(self);
	if (M_RAILGUN_DMG_MAX && damage > M_RAILGUN_DMG_MAX)
		damage = M_RAILGUN_DMG_MAX;

	if (self->s.frame >= WIDOW2_FRAME_fireb05 && self->s.frame <= WIDOW2_FRAME_fireb09)
		flash = MZ2_WIDOW2_BEAMER_1 + self->s.frame - WIDOW2_FRAME_fireb05;
	else if (self->s.frame >= WIDOW2_FRAME_spawn04 && self->s.frame <= WIDOW2_FRAME_spawn14)
		flash = MZ2_WIDOW2_BEAM_SWEEP_1 + self->s.frame - WIDOW2_FRAME_spawn04;
	else
		flash = MZ2_WIDOW2_BEAM_SWEEP_1;

	MonsterAim(self, M_HITSCAN_INSTANT_ACC, 0, false, flash, forward, start);
	gi.sound(self, CHAN_WEAPON, sound_beam, 1, ATTN_NORM, 0);
	monster_fire_railgun(self, start, forward, damage, damage, flash);
}

static void widow2_fire_disruptor(edict_t *self)
{
	int damage;
	int speed;
	vec3_t forward, start;

	if (!G_EntExists(self->enemy))
		return;

	damage = DISRUPTOR_INITIAL_DAMAGE + DISRUPTOR_ADDON_DAMAGE * drone_damagelevel(self);
	if (M_DISRUPTOR_DMG_MAX && damage > M_DISRUPTOR_DMG_MAX)
		damage = M_DISRUPTOR_DMG_MAX;
	speed = DISRUPTOR_INITIAL_SPEED + DISRUPTOR_ADDON_SPEED * drone_damagelevel(self);
	if (M_DISRUPTOR_SPEED_MAX && speed > M_DISRUPTOR_SPEED_MAX)
		speed = M_DISRUPTOR_SPEED_MAX;

	MonsterAim(self, M_PROJECTILE_ACC, speed, true, MZ2_WIDOW_DISRUPTOR, forward, start);
	fire_disruptor(self, start, forward, damage, speed, visible(self, self->enemy) ? self->enemy : NULL);
	gi.WriteByte(svc_muzzleflash2);
	gi.WriteShort(self - g_edicts);
	gi.WriteByte(MZ2_WIDOW_DISRUPTOR);
	gi.multicast(start, MULTICAST_PVS);
	widow2_step(self);
}

static void widow2_attack_beam(edict_t *self)
{
	self->monsterinfo.currentmove = &widow2_move_beam;
	widow2_step(self);
}

static void widow2_proboscis_start(edict_t *self, vec3_t start)
{
	int index = self->s.frame - WIDOW2_FRAME_tongs01;
	vec3_t forward, right, offset;

	if (index < 0)
		index = 0;
	else if (index > 7)
		index = 7;

	VectorCopy(widow2_tongue_offsets[index], offset);
	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, offset, forward, right, start);
}

static qboolean widow2_proboscis_ok(edict_t *self, vec3_t start, vec3_t end)
{
	vec3_t dir, angles;
	trace_t tr;

	VectorSubtract(start, end, dir);
	if (VectorLength(dir) > WIDOW2_MELEE_RANGE)
		return false;

	vectoangles(dir, angles);
	if (angles[PITCH] < -180)
		angles[PITCH] += 360;
	if (fabsf(angles[PITCH]) > 30)
		return false;

	tr = gi.trace(start, NULL, NULL, end, self, MASK_SHOT);
	return tr.ent == self->enemy;
}

static qboolean widow2_proboscis_target(edict_t *self, vec3_t start, vec3_t end)
{
	if (!G_EntExists(self->enemy))
		return false;

	G_EntMidPoint(self->enemy, end);
	if (widow2_proboscis_ok(self, start, end))
		return true;

	VectorCopy(self->enemy->s.origin, end);
	end[2] = self->enemy->absmax[2] - 8;
	if (widow2_proboscis_ok(self, start, end))
		return true;

	VectorCopy(self->enemy->s.origin, end);
	end[2] = self->enemy->absmin[2] + 8;
	return widow2_proboscis_ok(self, start, end);
}

static qboolean widow2_draw_proboscis(edict_t *self, vec3_t start, vec3_t end)
{
	widow2_proboscis_start(self, start);
	if (!widow2_proboscis_target(self, start, end))
		return false;

	gi.sound(self, CHAN_WEAPON, sound_tongue, 1, ATTN_NORM, 0);
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_PARASITE_ATTACK);
	gi.WriteShort(self - g_edicts);
	gi.WritePosition(start);
	gi.WritePosition(end);
	gi.multicast(self->s.origin, MULTICAST_PVS);
	return true;
}

static void widow2_pull_enemy(edict_t *self)
{
	vec3_t pull;

	if (!G_EntExists(self->enemy))
		return;

	if (self->enemy->groundentity)
	{
		self->enemy->s.origin[2] += 1;
		self->enemy->groundentity = NULL;
	}

	VectorSubtract(self->s.origin, self->enemy->s.origin, pull);
	VectorNormalize(pull);
	VectorMA(self->enemy->velocity, 700, pull, self->enemy->velocity);
}

static void widow2_show_proboscis(edict_t *self)
{
	vec3_t start, end;

	widow2_draw_proboscis(self, start, end);
}

static void widow2_pull_proboscis(edict_t *self)
{
	vec3_t start, end;

	if (widow2_draw_proboscis(self, start, end))
		widow2_pull_enemy(self);
}

static void widow2_melee_hit(edict_t *self)
{
	int damage;
	vec3_t start, end;

	if (!widow2_draw_proboscis(self, start, end))
		return;

	widow2_pull_enemy(self);

	damage = M_MELEE_DMG_BASE + M_MELEE_DMG_ADDON * drone_damagelevel(self);
	if (M_MeleeAttack(self, self->enemy, WIDOW2_MELEE_RANGE, damage, 500))
		gi.sound(self, CHAN_WEAPON, sound_hit, 1, ATTN_NORM, 0);
}

static qboolean widow2_valid_spawn_spot(edict_t *self, vec3_t mins, vec3_t maxs, vec3_t spot)
{
	vec3_t start, end;
	trace_t tr;

	VectorCopy(spot, start);
	start[2] += 96;
	VectorCopy(spot, end);
	end[2] -= 160;

	tr = gi.trace(start, mins, maxs, end, self, MASK_MONSTERSOLID);
	if (tr.fraction == 1.0f || tr.startsolid || tr.allsolid)
		return false;

	VectorCopy(tr.endpos, spot);
	return G_IsValidLocation(self, spot, mins, maxs);
}

static qboolean widow2_find_spawn_spot(edict_t *self, int index, vec3_t spot)
{
	vec3_t mins, maxs;
	vec3_t forward, right;
	float side;

	VectorSet(mins, -28, -28, -18);
	VectorSet(maxs, 28, 28, 18);
	AngleVectors(self->s.angles, forward, right, NULL);

	side = (index & 1) ? 112.0f : -112.0f;
	VectorCopy(self->s.origin, spot);
	VectorMA(spot, 144.0f, forward, spot);
	VectorMA(spot, side, right, spot);
	if (widow2_valid_spawn_spot(self, mins, maxs, spot))
		return true;

	VectorCopy(self->s.origin, spot);
	VectorMA(spot, -120.0f, forward, spot);
	VectorMA(spot, side, right, spot);
	return widow2_valid_spawn_spot(self, mins, maxs, spot);
}

static void widow2_cleanup_failed_spawn(edict_t *owner, edict_t *spawned)
{
	if (owner && owner->client)
		layout_remove_tracked_entity(&owner->client->layout, spawned);

	DroneList_Remove(spawned);
	AI_EnemyRemoved(spawned);
	G_FreeEdict(spawned);
}

static qboolean widow2_spawn_stalker(edict_t *self, int index)
{
	edict_t *owner;
	edict_t *spawned;
	vec3_t spot;

	owner = (self->activator && self->activator->inuse) ? self->activator : self;
	spawned = G_Spawn();
	spawned->mtype = M_STALKER;
	spawned->activator = owner;
	spawned->monsterinfo.level = self->monsterinfo.level;

	if (!M_Initialize(owner, spawned, 0.0f))
	{
		G_FreeEdict(spawned);
		return false;
	}

	if (!widow2_find_spawn_spot(self, index, spot))
	{
		widow2_cleanup_failed_spawn(owner, spawned);
		return false;
	}

	spawned->monsterinfo.cost = 0;
	spawned->s.effects |= EF_PLASMA;
	VectorCopy(spot, spawned->s.origin);
	VectorCopy(spot, spawned->s.old_origin);
	VectorCopy(self->s.angles, spawned->s.angles);
	spawned->nextthink = level.time + FRAMETIME;
	spawned->monsterinfo.attack_finished = level.time + 1.0f;

	if (invasion->value)
	{
		spawned->monsterinfo.aiflags &= ~AI_STAND_GROUND;
		spawned->monsterinfo.aiflags |= AI_FIND_NAVI;
		spawned->prev_navi = NULL;
		spawned->goalentity = NULL;
	}

	if (G_ValidTarget(spawned, self->enemy, true, true))
		spawned->enemy = self->enemy;

	gi.linkentity(spawned);
	owner->num_monsters += spawned->monsterinfo.control_cost;
	owner->num_monsters_real++;

	if (spawned->enemy && spawned->monsterinfo.run)
		spawned->monsterinfo.run(spawned);
	else if (spawned->monsterinfo.stand)
		spawned->monsterinfo.stand(spawned);

	return true;
}

static void widow2_spawn_effects(edict_t *self)
{
	vec3_t mins, maxs, size, spot, effect_origin;
	float radius;

	VectorSet(mins, -28, -28, -18);
	VectorSet(maxs, 28, 28, 18);
	VectorSubtract(maxs, mins, size);
	radius = VectorLength(size) * 0.5f;

	for (int i = 0; i < WIDOW2_SUMMON_COUNT; i++)
	{
		if (!widow2_find_spawn_spot(self, i, spot))
			continue;

		VectorAdd(mins, maxs, effect_origin);
		VectorAdd(spot, effect_origin, effect_origin);
		SpawnGrow_Spawn(effect_origin, radius, radius * 2.0f);
	}
}

static void widow2_finish_spawn(edict_t *self)
{
	int spawned = 0;

	for (int i = 0; i < WIDOW2_SUMMON_COUNT; i++)
	{
		if (widow2_spawn_stalker(self, i))
			spawned++;
	}

	if (spawned)
		self->monsterinfo.melee_finished = level.time + WIDOW2_SUMMON_COOLDOWN;
}

static void widow2_start_spawn(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_spawn, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = &widow2_move_spawn;
}

static void widow2_melee(edict_t *self)
{
	if (!widow2_can_melee(self))
	{
		self->monsterinfo.melee_finished = level.time + 0.5f;
		return;
	}

	self->monsterinfo.currentmove = &widow2_move_tongs;
	self->monsterinfo.melee_finished = level.time + 1.5f;
	M_DelayNextAttack(self, 1.0f, true);
}

static void widow2_attack(edict_t *self)
{
	float r;

	if (!G_EntExists(self->enemy))
		return;

	r = random();
	if (widow2_can_melee(self) && r < 0.35f)
		widow2_melee(self);
	else if (level.time >= self->monsterinfo.melee_finished && r < 0.60f)
		widow2_start_spawn(self);
	else if (r < 0.80f)
		self->monsterinfo.currentmove = &widow2_move_disruptor;
	else
		self->monsterinfo.currentmove = &widow2_move_pre_beam;

	M_DelayNextAttack(self, 1.0f + random(), true);
}

static void widow2_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3.0f;
	if (random() < 0.33f)
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else if (random() < 0.5f)
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain3, 1, ATTN_NORM, 0);

	if (skill->value != 3)
		self->monsterinfo.currentmove = &widow2_move_pain;
}

static void widow2_explode(edict_t *self)
{
	vec3_t org;

	VectorCopy(self->s.origin, org);
	org[0] += crandom() * self->maxs[0];
	org[1] += crandom() * self->maxs[1];
	org[2] += random() * self->maxs[2];

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1_BIG);
	gi.WritePosition(org);
	gi.multicast(self->s.origin, MULTICAST_PVS);
}

static void widow2_dead(edict_t *self)
{
	for (int n = 0; n < 5; n++)
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", 500, GIB_ORGANIC);
	for (int n = 0; n < 8; n++)
		ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2", 500, GIB_METALLIC);

	M_Remove(self, false, false);
}

static void widow2_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	M_Notify(self);

	if (self->deadflag == DEAD_DEAD)
		return;

	DroneList_Remove(self);
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.currentmove = &widow2_move_death;
}

void init_drone_widow2(edict_t *self)
{
	sound_pain1 = gi.soundindex("widow/bw2pain1.wav");
	sound_pain2 = gi.soundindex("widow/bw2pain2.wav");
	sound_pain3 = gi.soundindex("widow/bw2pain3.wav");
	sound_death = gi.soundindex("widow/death.wav");
	sound_search = gi.soundindex("bosshovr/bhvunqv1.wav");
	sound_tongue = gi.soundindex("brain/brnatck3.wav");
	sound_step = gi.soundindex("widow/bwstep1.wav");
	gi.soundindex("weapons/disrupt.wav");
	sound_beam = gi.soundindex("weapons/disint2.wav");
	sound_hit = gi.soundindex("infantry/melee2.wav");
	sound_spawn = gi.soundindex("medic_commander/monsterspawn1.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/blackwidow2/tris.md2");
	VectorSet(self->mins, -70, -70, 0);
	VectorSet(self->maxs, 70, 70, 144);

	gi.modelindex("models/items/spawngro3/tris.md2");
	gi.modelindex("models/monsters/stalker/tris.md2");
	gi.modelindex("models/monsters/blackwidow2/gib1/tris.md2");
	gi.modelindex("models/monsters/blackwidow2/gib2/tris.md2");
	gi.modelindex("models/monsters/blackwidow2/gib3/tris.md2");
	gi.modelindex("models/monsters/blackwidow2/gib4/tris.md2");

	self->health = M_WIDOW2_INITIAL_HEALTH + M_WIDOW2_ADDON_HEALTH * self->monsterinfo.level;
	self->max_health = self->health;
	self->gib_health = -5000;
	self->mass = 2500;
	self->mtype = M_WIDOW2;
	self->yaw_speed = 30;
	self->flags |= FL_IMMUNE_LASER;

	self->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;
	self->monsterinfo.power_armor_power = M_WIDOW2_INITIAL_ARMOR + M_WIDOW2_ADDON_ARMOR * self->monsterinfo.level;
	self->monsterinfo.max_armor = self->monsterinfo.power_armor_power;
	self->monsterinfo.control_cost = M_JORG_CONTROL_COST;
	self->monsterinfo.cost = M_COMMANDER_COST;
	self->monsterinfo.jumpup = 64;
	self->monsterinfo.jumpdn = 512;
	self->monsterinfo.aiflags |= AI_NO_CIRCLE_STRAFE;

	self->pain = widow2_pain;
	self->die = widow2_die;
	self->monsterinfo.stand = widow2_stand;
	self->monsterinfo.walk = widow2_walk;
	self->monsterinfo.run = widow2_run;
	self->monsterinfo.attack = widow2_attack;
	self->monsterinfo.melee = widow2_melee;
	self->monsterinfo.sight = widow2_sight;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &widow2_move_stand;
	self->monsterinfo.scale = 2.0f;
	self->nextthink = level.time + FRAMETIME;

	G_PrintGreenText(va("A level %d widow2 has spawned!", self->monsterinfo.level));
}
