/*
==============================================================================

black widow

==============================================================================
*/

#include "g_local.h"
#include "../../quake2/monsterframes/m_rogue_widow.h"

static constexpr int WIDOW_SUMMON_COUNT = 2;
static constexpr float WIDOW_SUMMON_COOLDOWN = 10.0f;
static constexpr float WIDOW_MELEE_RANGE = 176.0f;
static constexpr float WIDOW_INVASION_SCALE = 0.75f;
static constexpr float WIDOW_INVASION_HALF_WIDTH = 30.0f;
static constexpr float WIDOW_INVASION_HEIGHT = 108.0f;
static constexpr int WIDOW_LEGS_MAX_FRAME = 23;
static constexpr float WIDOW_LEGS_FRAME_TIME = 0.1f;
static constexpr float WIDOW_LEGS_WAIT_TIME = 1.0f;

static int sound_pain1;
static int sound_pain2;
static int sound_pain3;
static int sound_death;
static int sound_laugh;
static int sound_rail;
static int sound_step1;
static int sound_step2;
static int sound_hit;
static int sound_spawn;
static int shotsfired;

void drone_ai_stand(edict_t *self, float dist);
void drone_ai_run(edict_t *self, float dist);
void drone_ai_walk(edict_t *self, float dist);
qboolean drone_findtarget(edict_t *self, qboolean force);

static void widow_stand(edict_t *self);
static void widow_walk(edict_t *self);
static void widow_run(edict_t *self);
static void widow_attack(edict_t *self);
static void widow_fire_blaster(edict_t *self);
static void widow_fire_rail(edict_t *self);
static void widow_melee_hit(edict_t *self);
static void widow_spawn_effects(edict_t *self);
static void widow_finish_spawn(edict_t *self);
static void widow_explode(edict_t *self);
static void widow_spawn_out_start(edict_t *self);
static void widow_spawn_out_do(edict_t *self);
static void widow_step1(edict_t *self);
static void widow_step2(edict_t *self);

static vec3_t widow_beam_effects[] = {
	{ 12.58f, -43.71f, 68.88f },
	{ 3.43f, 58.72f, 68.41f }
};

static mframe_t widow_frames_stand[] =
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
	drone_ai_stand, 0, NULL
};
static mmove_t widow_move_stand = { FRAME_idle01, FRAME_idle11, widow_frames_stand, widow_stand };

static mframe_t widow_frames_walk[] =
{
	drone_ai_walk, 7, widow_step1,
	drone_ai_walk, 8, NULL,
	drone_ai_walk, 9, NULL,
	drone_ai_walk, 10, NULL,
	drone_ai_walk, 9, widow_step2,
	drone_ai_walk, 8, NULL,
	drone_ai_walk, 7, NULL,
	drone_ai_walk, 6, NULL,
	drone_ai_walk, 7, widow_step1,
	drone_ai_walk, 8, NULL,
	drone_ai_walk, 9, NULL,
	drone_ai_walk, 8, NULL,
	drone_ai_walk, 7, widow_step2
};
static mmove_t widow_move_walk = { FRAME_walk01, FRAME_walk13, widow_frames_walk, widow_walk };

static mframe_t widow_frames_run[] =
{
	drone_ai_run, 12, widow_step1,
	drone_ai_run, 16, NULL,
	drone_ai_run, 18, NULL,
	drone_ai_run, 20, widow_step2,
	drone_ai_run, 18, NULL,
	drone_ai_run, 16, NULL,
	drone_ai_run, 14, NULL,
	drone_ai_run, 12, widow_step1
};
static mmove_t widow_move_run = { FRAME_run01, FRAME_run08, widow_frames_run, NULL };

static mframe_t widow_frames_blaster[] =
{
	ai_charge, 0, widow_fire_blaster,
	ai_charge, 0, widow_fire_blaster,
	ai_charge, 0, widow_fire_blaster,
	ai_charge, 0, widow_fire_blaster,
	ai_charge, 0, widow_fire_blaster,
	ai_charge, 0, widow_fire_blaster,
	ai_charge, 0, widow_fire_blaster,
	ai_charge, 0, widow_fire_blaster,
	ai_charge, 0, widow_fire_blaster,
	ai_charge, 0, widow_fire_blaster,
	ai_charge, 0, widow_fire_blaster,
	ai_charge, 0, widow_fire_blaster,
	ai_charge, 0, widow_fire_blaster,
	ai_charge, 0, widow_fire_blaster,
	ai_charge, 0, widow_fire_blaster,
	ai_charge, 0, widow_fire_blaster,
	ai_charge, 0, widow_fire_blaster,
	ai_charge, 0, widow_fire_blaster,
	ai_charge, 0, widow_fire_blaster
};
static mmove_t widow_move_blaster = { FRAME_fired02a, FRAME_fired20, widow_frames_blaster, widow_run };

static mframe_t widow_frames_rail[] =
{
	ai_charge, 0, NULL,
	ai_charge, -6, NULL,
	ai_charge, -10, widow_fire_rail,
	ai_charge, -4, NULL,
	ai_charge, 0, NULL,
	ai_charge, -6, widow_fire_rail,
	ai_charge, -4, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
static mmove_t widow_move_rail = { FRAME_firea01, FRAME_firea09, widow_frames_rail, widow_run };
static mmove_t widow_move_rail_right = { FRAME_fireb01, FRAME_fireb09, widow_frames_rail, widow_run };
static mmove_t widow_move_rail_left = { FRAME_firec01, FRAME_firec09, widow_frames_rail, widow_run };

static mframe_t widow_frames_spawn[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, widow_spawn_effects,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, -4, widow_finish_spawn,
	ai_charge, -8, NULL,
	ai_charge, -10, NULL,
	ai_charge, -8, NULL,
	ai_charge, -4, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
static mmove_t widow_move_spawn = { FRAME_spawn01, FRAME_spawn18, widow_frames_spawn, widow_run };

static mframe_t widow_frames_pain_heavy[] =
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
	ai_move, 0, NULL
};
static mmove_t widow_move_pain_heavy = { FRAME_pain01, FRAME_pain13, widow_frames_pain_heavy, widow_run };

static mframe_t widow_frames_pain_light[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
static mmove_t widow_move_pain_light = { FRAME_pain201, FRAME_pain203, widow_frames_pain_light, widow_run };

static mframe_t widow_frames_death[] =
{
	ai_move, 0, widow_explode,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, widow_explode,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, widow_explode,
	ai_move, 0, widow_spawn_out_start,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, widow_explode,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, widow_explode,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, widow_explode,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, widow_explode,
	ai_move, 0, widow_spawn_out_do
};
static mmove_t widow_move_death = { FRAME_death01, FRAME_death31, widow_frames_death, NULL };

static mframe_t widow_frames_kick[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, widow_melee_hit,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
static mmove_t widow_move_kick = { FRAME_kick01, FRAME_kick08, widow_frames_kick, widow_run };

static void widow_step1(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_step1, 1, ATTN_NORM, 0);
}

static void widow_step2(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_step2, 1, ATTN_NORM, 0);
}

static void widow_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_laugh, 1, ATTN_NORM, 0);
}

static void widow_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &widow_move_stand;
}

static void widow_walk(edict_t *self)
{
	if (!self->goalentity)
		self->goalentity = world;
	self->monsterinfo.currentmove = &widow_move_walk;
}

static void widow_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &widow_move_stand;
	else
		self->monsterinfo.currentmove = &widow_move_run;
}

static qboolean widow_can_melee(edict_t *self)
{
	return G_EntExists(self->enemy) && entdist(self, self->enemy) <= WIDOW_MELEE_RANGE;
}

static void widow_project_flash(edict_t *self, int flash, vec3_t forward, vec3_t start)
{
	vec3_t right, offset;

	AngleVectors(self->s.angles, forward, right, NULL);
	VectorCopy(monster_flash_offset[flash], offset);
	if (self->s.scale && self->s.scale != 1.0f)
		VectorScale(offset, self->s.scale, offset);
	G_ProjectSource(self->s.origin, offset, forward, right, start);
}

static int widow_blaster_flash(edict_t *self)
{
	if (self->s.frame >= FRAME_spawn01 + 4 && self->s.frame <= FRAME_spawn01 + 12)
		return MZ2_WIDOW_BLASTER_SWEEP1 + self->s.frame - (FRAME_spawn01 + 4);
	if (self->s.frame == FRAME_fired02a)
		return MZ2_WIDOW_BLASTER_0;
	if (self->s.frame >= FRAME_fired03 && self->s.frame <= FRAME_fired20)
		return MZ2_WIDOW_BLASTER_100 + self->s.frame - FRAME_fired03;
	if (self->s.frame >= FRAME_run01 && self->s.frame <= FRAME_run08)
		return MZ2_WIDOW_RUN_1 + self->s.frame - FRAME_run01;
	return MZ2_WIDOW_BLASTER;
}

static void widow_fire_blaster(edict_t *self)
{
	int damage;
	int speed;
	int flash;
	int effect;
	vec3_t forward, start;

	if (!G_EntExists(self->enemy))
		return;

	damage = M_BLASTER2_DMG_BASE + M_BLASTER2_DMG_ADDON * drone_damagelevel(self);
	if (M_BLASTER2_DMG_MAX && damage > M_BLASTER2_DMG_MAX)
		damage = M_BLASTER2_DMG_MAX;
	speed = M_BLASTER2_SPEED_BASE + M_BLASTER2_SPEED_ADDON * drone_damagelevel(self);
	if (M_BLASTER2_SPEED_MAX && speed > M_BLASTER2_SPEED_MAX)
		speed = M_BLASTER2_SPEED_MAX;

	flash = widow_blaster_flash(self);
	shotsfired++;
	effect = (shotsfired % 4) ? 0 : EF_BLASTER;
	widow_project_flash(self, flash, forward, start);
	MonsterAim(self, M_PROJECTILE_ACC, speed, false, -1, forward, start);
	monster_fire_blaster2(self, start, forward, damage, speed, effect, flash);
}

static void widow_fire_rail(edict_t *self)
{
	int damage;
	int flash;
	vec3_t forward, start;

	if (!G_EntExists(self->enemy))
		return;

	damage = M_RAILGUN_DMG_BASE + M_RAILGUN_DMG_ADDON * drone_damagelevel(self);
	if (M_RAILGUN_DMG_MAX && damage > M_RAILGUN_DMG_MAX)
		damage = M_RAILGUN_DMG_MAX;

	if (self->monsterinfo.currentmove == &widow_move_rail_left)
		flash = MZ2_WIDOW_RAIL_LEFT;
	else if (self->monsterinfo.currentmove == &widow_move_rail_right)
		flash = MZ2_WIDOW_RAIL_RIGHT;
	else
		flash = MZ2_WIDOW_RAIL;
	widow_project_flash(self, flash, forward, start);
	MonsterAim(self, M_HITSCAN_INSTANT_ACC, 0, false, -1, forward, start);
	gi.sound(self, CHAN_WEAPON, sound_rail, 1, ATTN_NORM, 0);
	monster_fire_railgun(self, start, forward, damage, damage, flash);
}

static void widow_melee_hit(edict_t *self)
{
	int damage;

	damage = M_MELEE_DMG_BASE + M_MELEE_DMG_ADDON * drone_damagelevel(self);
	if (M_MeleeAttack(self, self->enemy, 160, damage, 350))
		gi.sound(self, CHAN_WEAPON, sound_hit, 1, ATTN_NORM, 0);
}

static qboolean widow_valid_spawn_spot(edict_t *self, vec3_t mins, vec3_t maxs, vec3_t spot)
{
	vec3_t start, end;
	trace_t tr;

	VectorCopy(spot, start);
	start[2] += 72;
	VectorCopy(spot, end);
	end[2] -= 128;

	tr = gi.trace(start, mins, maxs, end, self, MASK_MONSTERSOLID);
	if (tr.fraction == 1.0f || tr.startsolid || tr.allsolid)
		return false;

	VectorCopy(tr.endpos, spot);
	return G_IsValidLocation(self, spot, mins, maxs);
}

static qboolean widow_find_spawn_spot(edict_t *self, int index, vec3_t spot)
{
	vec3_t mins, maxs;
	vec3_t forward, right;
	float side;

	VectorSet(mins, -28, -28, -18);
	VectorSet(maxs, 28, 28, 18);
	AngleVectors(self->s.angles, forward, right, NULL);

	side = (index & 1) ? 88.0f : -88.0f;
	VectorCopy(self->s.origin, spot);
	VectorMA(spot, 112.0f, forward, spot);
	VectorMA(spot, side, right, spot);
	if (widow_valid_spawn_spot(self, mins, maxs, spot))
		return true;

	VectorCopy(self->s.origin, spot);
	VectorMA(spot, -96.0f, forward, spot);
	VectorMA(spot, side, right, spot);
	return widow_valid_spawn_spot(self, mins, maxs, spot);
}

static void widow_cleanup_failed_spawn(edict_t *owner, edict_t *spawned)
{
	if (owner && owner->client)
		layout_remove_tracked_entity(&owner->client->layout, spawned);

	DroneList_Remove(spawned);
	AI_EnemyRemoved(spawned);
	G_FreeEdict(spawned);
}

static void widow_setup_invasion_spawn(edict_t *spawned)
{
	if (!invasion->value)
		return;

	spawned->monsterinfo.aiflags &= ~AI_STAND_GROUND;
	spawned->monsterinfo.aiflags |= AI_FIND_NAVI;
	spawned->prev_navi = NULL;
	spawned->goalentity = NULL;
}

static void widow_start_spawned_monster(edict_t *self, edict_t *spawned)
{
	const qboolean force_start = invasion->value || pvm->value;

	if (G_ValidTarget(spawned, self->enemy, !force_start, true))
	{
		spawned->enemy = self->enemy;
		VectorCopy(self->enemy->s.origin, spawned->monsterinfo.last_sighting);
	}
	else if (force_start)
		drone_findtarget(spawned, true);

	if ((spawned->enemy || spawned->goalentity) && spawned->monsterinfo.run)
		spawned->monsterinfo.run(spawned);
	else if (spawned->monsterinfo.stand)
		spawned->monsterinfo.stand(spawned);
}

static qboolean widow_spawn_stalker(edict_t *self, int index)
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

	if (!widow_find_spawn_spot(self, index, spot))
	{
		widow_cleanup_failed_spawn(owner, spawned);
		return false;
	}

	spawned->monsterinfo.cost = 0;
	spawned->s.effects |= EF_PLASMA;
	VectorCopy(spot, spawned->s.origin);
	VectorCopy(spot, spawned->s.old_origin);
	VectorCopy(self->s.angles, spawned->s.angles);
	spawned->nextthink = level.time + FRAMETIME;
	spawned->monsterinfo.attack_finished = level.time + 1.0f;
	widow_setup_invasion_spawn(spawned);

	gi.linkentity(spawned);
	owner->num_monsters += spawned->monsterinfo.control_cost;
	owner->num_monsters_real++;

	widow_start_spawned_monster(self, spawned);

	return true;
}

static void widow_spawn_effects(edict_t *self)
{
	vec3_t mins, maxs, size, spot, effect_origin;
	float radius;

	VectorSet(mins, -28, -28, -18);
	VectorSet(maxs, 28, 28, 18);
	VectorSubtract(maxs, mins, size);
	radius = VectorLength(size) * 0.5f;

	for (int i = 0; i < WIDOW_SUMMON_COUNT; i++)
	{
		if (!widow_find_spawn_spot(self, i, spot))
			continue;

		VectorAdd(mins, maxs, effect_origin);
		VectorAdd(spot, effect_origin, effect_origin);
		SpawnGrow_Spawn(effect_origin, radius, radius * 2.0f);
	}
}

static void widow_finish_spawn(edict_t *self)
{
	int spawned = 0;

	for (int i = 0; i < WIDOW_SUMMON_COUNT; i++)
	{
		if (widow_spawn_stalker(self, i))
			spawned++;
	}

	if (spawned)
		self->monsterinfo.melee_finished = level.time + WIDOW_SUMMON_COOLDOWN;
}

static qboolean widow_can_spawn_stalker(edict_t *self)
{
	vec3_t spot;

	for (int i = 0; i < WIDOW_SUMMON_COUNT; i++)
	{
		if (widow_find_spawn_spot(self, i, spot))
			return true;
	}

	return false;
}

static void widow_start_spawn(edict_t *self)
{
	gi.sound(self, CHAN_WEAPON, sound_spawn, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = &widow_move_spawn;
}

static void widow_melee(edict_t *self)
{
	if (!widow_can_melee(self))
	{
		self->monsterinfo.melee_finished = level.time + 0.5f;
		return;
	}

	self->monsterinfo.currentmove = &widow_move_kick;
	self->monsterinfo.melee_finished = level.time + 1.2f;
	M_DelayNextAttack(self, 1.0f, true);
}

static void widow_attack(edict_t *self)
{
	float r;

	if (!G_EntExists(self->enemy))
		return;

	r = random();
	if (level.time >= self->monsterinfo.melee_finished && widow_can_spawn_stalker(self))
		widow_start_spawn(self);
	else if (widow_can_melee(self) && r < 0.40f)
		widow_melee(self);
	else if (r < 0.82f)
	{
		if (random() < 0.33f)
			self->monsterinfo.currentmove = &widow_move_rail_left;
		else if (random() < 0.5f)
			self->monsterinfo.currentmove = &widow_move_rail_right;
		else
			self->monsterinfo.currentmove = &widow_move_rail;
	}
	else
		self->monsterinfo.currentmove = &widow_move_blaster;

	M_DelayNextAttack(self, 1.0f + random(), true);
}

static void widow_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 5.0f;
	if (damage < 15)
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else if (damage < 75)
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain3, 1, ATTN_NORM, 0);

	if (skill->value == 3)
		return;

	if (damage >= 40 && damage < 200)
	{
		if (random() < (0.6f - (0.2f * skill->value)))
			self->monsterinfo.currentmove = &widow_move_pain_light;
	}
	else if (damage >= 200)
	{
		if (random() < (0.75f - (0.1f * skill->value)))
			self->monsterinfo.currentmove = &widow_move_pain_heavy;
	}
}

static void widow_explode(edict_t *self)
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

static void widow_project_source2(vec3_t origin, vec3_t offset, vec3_t forward, vec3_t right, vec3_t up, vec3_t result)
{
	result[0] = origin[0] + forward[0] * offset[0] + right[0] * offset[1] + up[0] * offset[2];
	result[1] = origin[1] + forward[1] * offset[0] + right[1] * offset[1] + up[1] * offset[2];
	result[2] = origin[2] + forward[2] * offset[0] + right[2] * offset[1] + up[2] * offset[2];
}

static void widow_temp_explosion(vec3_t point)
{
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(point);
	gi.multicast(point, MULTICAST_ALL);
}

static void widowlegs_throw_gib_at(edict_t *self, char *gibname, int damage, int type, vec3_t point)
{
	edict_t *gib;
	float scale;

	if (nolag->value)
		return;

	scale = self->s.scale ? self->s.scale : 1.0f;
	gib = ThrowGibEx(self, gibname, damage, type, scale);
	if (!gib)
		return;

	VectorCopy(point, gib->s.origin);
	VectorCopy(point, gib->s.old_origin);
	gi.linkentity(gib);
}

static void widowlegs_project(edict_t *self, vec3_t offset, vec3_t point)
{
	vec3_t forward, right, up;

	AngleVectors(self->s.angles, forward, right, up);
	widow_project_source2(self->s.origin, offset, forward, right, up, point);
}

static void widowlegs_think(edict_t *self)
{
	vec3_t offset;
	vec3_t point;

	if (self->s.frame == 17)
	{
		VectorSet(offset, 11.77f, -7.24f, 23.31f);
		widowlegs_project(self, offset, point);
		widow_temp_explosion(point);
	}

	if (self->s.frame < WIDOW_LEGS_MAX_FRAME)
	{
		self->s.frame++;
		self->nextthink = level.time + WIDOW_LEGS_FRAME_TIME;
		return;
	}

	if (!self->wait)
		self->wait = level.time + WIDOW_LEGS_WAIT_TIME;

	if ((level.time > self->wait - 0.5f) && !self->count)
	{
		self->count = 1;

		VectorSet(offset, 31.0f, -88.7f, 10.96f);
		widowlegs_project(self, offset, point);
		widow_temp_explosion(point);

		VectorSet(offset, -12.67f, -4.39f, 15.68f);
		widowlegs_project(self, offset, point);
		widow_temp_explosion(point);
	}

	if (level.time > self->wait)
	{
		VectorSet(offset, -65.6f, -8.44f, 28.59f);
		widowlegs_project(self, offset, point);
		widow_temp_explosion(point);
		widowlegs_throw_gib_at(self, "models/monsters/blackwidow/gib1/tris.md2", 90, GIB_METALLIC | GIB_UPRIGHT, point);
		widowlegs_throw_gib_at(self, "models/monsters/blackwidow/gib2/tris.md2", 90, GIB_METALLIC | GIB_UPRIGHT, point);

		VectorSet(offset, -1.04f, -51.18f, 7.04f);
		widowlegs_project(self, offset, point);
		widow_temp_explosion(point);
		widowlegs_throw_gib_at(self, "models/monsters/blackwidow/gib1/tris.md2", 90, GIB_METALLIC | GIB_UPRIGHT, point);
		widowlegs_throw_gib_at(self, "models/monsters/blackwidow/gib2/tris.md2", 90, GIB_METALLIC | GIB_UPRIGHT, point);
		widowlegs_throw_gib_at(self, "models/monsters/blackwidow/gib3/tris.md2", 90, GIB_METALLIC | GIB_UPRIGHT, point);

		G_FreeEdict(self);
		return;
	}

	self->nextthink = level.time + WIDOW_LEGS_FRAME_TIME;
}

static void widowlegs_spawn(vec3_t startpos, vec3_t angles, float scale)
{
	edict_t *ent;

	if (nolag->value)
		return;

	ent = G_Spawn();
	VectorCopy(startpos, ent->s.origin);
	VectorCopy(angles, ent->s.angles);
	ent->solid = SOLID_NOT;
	ent->s.renderfx = RF_IR_VISIBLE;
	ent->movetype = MOVETYPE_NONE;
	ent->classname = "widowlegs";
	ent->s.modelindex = gi.modelindex("models/monsters/legs/tris.md2");
	if (scale)
		ent->s.scale = scale;
	ent->think = widowlegs_think;
	ent->nextthink = level.time + WIDOW_LEGS_FRAME_TIME;
	gi.linkentity(ent);
}

static void widow_spawn_out_start(edict_t *self)
{
	vec3_t startpoint;
	vec3_t forward, right, up;

	AngleVectors(self->s.angles, forward, right, up);

	widow_project_source2(self->s.origin, widow_beam_effects[0], forward, right, up, startpoint);
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_WIDOWBEAMOUT);
	gi.WriteShort(20001);
	gi.WritePosition(startpoint);
	gi.multicast(startpoint, MULTICAST_ALL);

	widow_project_source2(self->s.origin, widow_beam_effects[1], forward, right, up, startpoint);
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_WIDOWBEAMOUT);
	gi.WriteShort(20002);
	gi.WritePosition(startpoint);
	gi.multicast(startpoint, MULTICAST_ALL);

	gi.sound(self, CHAN_VOICE, gi.soundindex("misc/bwidowbeamout.wav"), 1, ATTN_NORM, 0);
}

static void widow_spawn_out_do(edict_t *self)
{
	vec3_t startpoint;
	vec3_t forward, right, up;

	AngleVectors(self->s.angles, forward, right, up);

	widow_project_source2(self->s.origin, widow_beam_effects[0], forward, right, up, startpoint);
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_WIDOWSPLASH);
	gi.WritePosition(startpoint);
	gi.multicast(startpoint, MULTICAST_ALL);

	widow_project_source2(self->s.origin, widow_beam_effects[1], forward, right, up, startpoint);
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_WIDOWSPLASH);
	gi.WritePosition(startpoint);
	gi.multicast(startpoint, MULTICAST_ALL);

	VectorCopy(self->s.origin, startpoint);
	startpoint[2] += 36;
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_BOSSTPORT);
	gi.WritePosition(startpoint);
	gi.multicast(startpoint, MULTICAST_PHS);

	widowlegs_spawn(self->s.origin, self->s.angles, self->s.scale);
	vrx_throw_drone_gibs(self, 500);

	M_Remove(self, false, false);
}

static void widow_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	M_Notify(self);

	if (self->deadflag == DEAD_DEAD)
		return;

	DroneList_Remove(self);
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	vrx_update_drone_death_skin(self);
	self->monsterinfo.currentmove = &widow_move_death;
}

void init_drone_widow(edict_t *self)
{
	sound_pain1 = gi.soundindex("widow/bw1pain1.wav");
	sound_pain2 = gi.soundindex("widow/bw1pain2.wav");
	sound_pain3 = gi.soundindex("widow/bw1pain3.wav");
	sound_death = gi.soundindex("widow/death.wav");
	sound_laugh = gi.soundindex("widow/laugh.wav");
	sound_rail = gi.soundindex("gladiator/railgun.wav");
	sound_step1 = gi.soundindex("widow/bwstep1.wav");
	sound_step2 = gi.soundindex("widow/bwstep2.wav");
	sound_hit = gi.soundindex("tank/tnkatck3.wav");
	sound_spawn = gi.soundindex("medic_commander/monsterspawn1.wav");
	gi.soundindex("misc/bwidowbeamout.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/blackwidow/tris.md2");
	VectorSet(self->mins, -40, -40, 0);
	VectorSet(self->maxs, 40, 40, 144);
	if (invasion->value)
	{
		self->s.scale = WIDOW_INVASION_SCALE;
		VectorSet(self->mins, -WIDOW_INVASION_HALF_WIDTH, -WIDOW_INVASION_HALF_WIDTH, 0);
		VectorSet(self->maxs, WIDOW_INVASION_HALF_WIDTH, WIDOW_INVASION_HALF_WIDTH, WIDOW_INVASION_HEIGHT);
	}

	gi.modelindex("models/items/spawngro3/tris.md2");
	gi.modelindex("models/monsters/stalker/tris.md2");
	gi.modelindex("models/monsters/legs/tris.md2");
	gi.modelindex("models/monsters/blackwidow/gib1/tris.md2");
	gi.modelindex("models/monsters/blackwidow/gib2/tris.md2");
	gi.modelindex("models/monsters/blackwidow/gib3/tris.md2");
	gi.modelindex("models/monsters/blackwidow/gib4/tris.md2");

	self->health = M_WIDOW_INITIAL_HEALTH + M_WIDOW_ADDON_HEALTH * self->monsterinfo.level;
	self->max_health = self->health;
	self->gib_health = -5000;
	self->mass = 1500;
	self->mtype = M_WIDOW;
	self->yaw_speed = 30;
	self->flags |= FL_IMMUNE_LASER;

	M_SetMonsterArmor(self, M_WIDOW_INITIAL_ARMOR + M_WIDOW_ADDON_ARMOR * self->monsterinfo.level);
	self->monsterinfo.control_cost = M_JORG_CONTROL_COST;
	self->monsterinfo.cost = M_COMMANDER_COST;
	self->monsterinfo.jumpup = 64;
	self->monsterinfo.jumpdn = 512;
	self->monsterinfo.aiflags |= AI_NO_CIRCLE_STRAFE;

	self->pain = widow_pain;
	self->die = widow_die;
	self->monsterinfo.stand = widow_stand;
	self->monsterinfo.walk = widow_walk;
	self->monsterinfo.run = widow_run;
	self->monsterinfo.attack = widow_attack;
	self->monsterinfo.melee = widow_melee;
	self->monsterinfo.sight = widow_sight;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &widow_move_stand;
	self->monsterinfo.scale = 2.0f;
	self->nextthink = level.time + FRAMETIME;

	if (!invasion->value)
		G_PrintGreenText(va("A level %d widow has spawned!", self->monsterinfo.level));
}
