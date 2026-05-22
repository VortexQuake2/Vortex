/*
==============================================================================

TANK

==============================================================================
*/

#include "g_local.h"
#include "../../quake2/monsterframes/m_tank.h"

void mytank_refire_rocket (edict_t *self);
void mytank_doattack_rocket (edict_t *self);
void mytank_reattack_blaster (edict_t *self);
void mytank_reattack_n64_blaster2(edict_t *self);
void mytank_reattack_n64_grenade(edict_t *self);
void mytank_reattack_n64_lightning(edict_t *self);
void mytank_meleeattack (edict_t *self);
void mytank_restrike (edict_t *self);
void mytank_chain_refire (edict_t *self);
void mytank_attack_chain (edict_t *self);
void myTankStrike(edict_t *self);
void Monster_MoveSpawn(edict_t *self);
qboolean drone_findtarget(edict_t *self, qboolean force);

static int	sound_thud;
static int	sound_pain;
static int	sound_pain2;
static int	sound_idle;
static int	sound_die;
static int	sound_step;
static int	sound_sight;
static int	sound_windup;
static int	sound_strike;
static int	sound_grenade;
static int	sound_spawn;

static constexpr float TANK_N64_SCALE = 1.1f;
static constexpr int TANK_N64_BLASTER2_DAMAGE = 26;
static constexpr int TANK_N64_BLASTER2_ADDON = 4;
static constexpr int TANK_N64_BLASTER2_SPEED = 950;
static constexpr int TANK_N64_GRENADE_DAMAGE = 50;
static constexpr int TANK_N64_GRENADE_ADDON = 8;
static constexpr int TANK_N64_GRENADE_SPEED = 1500;
static constexpr int TANK_N64_HEAT_DAMAGE = 45;
static constexpr int TANK_N64_HEAT_ADDON = 7;
static constexpr int TANK_N64_HEAT_SPEED = 480;
static constexpr float TANK_N64_HEAT_TURN_FRACTION = 0.075f;
static constexpr int TANK_N64_FLECHETTE_DAMAGE = 6;
static constexpr int TANK_N64_FLECHETTE_ADDON = 1;
static constexpr int TANK_N64_FLECHETTE_SPEED = 700;
static constexpr int TANK_N64_FLECHETTE_SPEED_MAX = 1150;
static constexpr int TANK_N64_LIGHTNING_DAMAGE = 15;
static constexpr int TANK_N64_LIGHTNING_ADDON = 3;
static constexpr int TANK_N64_ATTACK_BLASTER2 = 1;
static constexpr int TANK_N64_ATTACK_GRENADE = 2;
static constexpr int TANK_N64_ATTACK_LIGHTNING = 3;
static constexpr int TANK_SPAWN_NORMAL_COUNT = 3;
static constexpr int TANK_SPAWN_N64_COUNT = 2;
static constexpr int TANK_SPAWN_MAX_COUNT = 3;
static constexpr float TANK_SPAWN_RETRY_MIN = 2.0f;
static constexpr float TANK_SPAWN_RETRY_MAX = 8.0f;
static constexpr float TANK_SPAWN_FAIL_COOLDOWN = 1.5f;
static constexpr float TANK_SPAWN_MIN_SEPARATION = 56.0f;

//
// misc
//

typedef struct
{
	int count;
	int mtypes[TANK_SPAWN_MAX_COUNT];
	vec3_t spots[TANK_SPAWN_MAX_COUNT];
	vec3_t mins[TANK_SPAWN_MAX_COUNT];
	vec3_t maxs[TANK_SPAWN_MAX_COUNT];
	float radius[TANK_SPAWN_MAX_COUNT];
} tank_spawn_plan_t;

static qboolean mytank_is_n64(const edict_t *self)
{
	return self && self->mtype == M_TANK_N64;
}

static int mytank_n64_damage(edict_t *self, int base, int addon)
{
	return base + addon * drone_damagelevel(self);
}

static void mytank_muzzleflash(edict_t *self, vec3_t start, int flash_number)
{
	if (flash_number < 0)
		return;

	gi.WriteByte(svc_muzzleflash2);
	gi.WriteShort(self - g_edicts);
	gi.WriteByte(flash_number);
	gi.multicast(start, MULTICAST_PVS);
}

static int mytank_blaster_flash(edict_t *self)
{
	if (self->s.frame == FRAME_attak110)
		return MZ2_TANK_BLASTER_1;
	if (self->s.frame == FRAME_attak113)
		return MZ2_TANK_BLASTER_2;
	return MZ2_TANK_BLASTER_3;
}

static qboolean mytank_n64_enemy_almost_dead(edict_t *self, int damage)
{
	int max_health;
	int threshold;

	if (!G_ValidTarget(self, self->enemy, true, true))
		return false;

	max_health = self->enemy->max_health > 0 ? self->enemy->max_health : self->enemy->health;
	threshold = max(max_health / 4, damage * 2);
	return self->enemy->health <= threshold;
}

static void mytank_n64_lightning_effect(edict_t *self, vec3_t start, vec3_t dir)
{
	vec3_t end;
	trace_t tr;

	VectorMA(start, 8192, dir, end);
	tr = gi.trace(start, NULL, NULL, end, self, MASK_SHOT);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_LIGHTNING);
	gi.WriteShort(self - g_edicts);
	gi.WriteShort(0);
	gi.WritePosition(start);
	gi.WritePosition(tr.endpos);
	gi.multicast(start, MULTICAST_PVS);
}

void mytank_footstep (edict_t *self)
{
	gi.sound (self, CHAN_BODY, sound_step, 1, ATTN_NORM, 0);
}

void mytank_thud (edict_t *self)
{
	gi.sound (self, CHAN_BODY, sound_thud, 1, ATTN_NORM, 0);
}

void mytank_windup (edict_t *self)
{
	gi.sound (self, CHAN_WEAPON, sound_windup, 1, ATTN_NORM, 0);
}

static void mytank_slam_origin(edict_t *self, vec3_t origin)
{
	vec3_t	forward, right, offset;
	trace_t	tr;

	AngleVectors(self->s.angles, forward, right, NULL);
	VectorSet(offset, 20, -14.3f, -21);
	G_ProjectSource(self->s.origin, offset, forward, right, origin);
	tr = gi.trace(self->s.origin, NULL, NULL, origin, self, MASK_SOLID);
	VectorCopy(tr.endpos, origin);
}

static void mytank_slam_effect(vec3_t origin)
{
	vec3_t	up;

	VectorSet(up, 0, 0, 1);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_BERSERK_SLAM);
	gi.WritePosition(origin);
	gi.WriteDir(up);
	gi.multicast(origin, MULTICAST_PHS);
}

static qboolean mytank_can_blaster(edict_t *self)
{
	return M_MonsterHasClearShotFromFlash(self, MZ2_TANK_BLASTER_1);
}

static qboolean mytank_can_rocket(edict_t *self)
{
	return M_MonsterHasClearShotFromFlash(self, MZ2_TANK_ROCKET_1);
}

static qboolean mytank_can_chain(edict_t *self)
{
	return M_MonsterHasClearShotFromFlash(self, MZ2_TANK_MACHINEGUN_5);
}

static int mytank_spawn_desired_count(edict_t *self)
{
	return mytank_is_n64(self) ? TANK_SPAWN_N64_COUNT : TANK_SPAWN_NORMAL_COUNT;
}

static int mytank_random_soldier_type(void)
{
	static const int soldier_types[] =
	{
		M_SOLDIER,
		M_SOLDIERLT,
		M_SOLDIERSS,
		M_SOLDIER_RIPPER,
		M_SOLDIER_BLUEBLASTER,
		M_SOLDIER_LASER
	};

	return soldier_types[GetRandom(0, (int)(sizeof(soldier_types) / sizeof(soldier_types[0])) - 1)];
}

static int mytank_spawn_mtype(edict_t *self)
{
	return mytank_is_n64(self) ? M_BERSERK : mytank_random_soldier_type();
}

static void mytank_spawn_bounds(int mtype, vec3_t mins, vec3_t maxs)
{
	M_SetBoundingBox(mtype, mins, maxs);

	if (mtype == M_BERSERK)
		VectorSet(maxs, 16, 16, 32);
}

static qboolean mytank_spawn_has_clear_line(edict_t *self, vec3_t spot)
{
	vec3_t start, end;
	trace_t tr;

	VectorCopy(self->s.origin, start);
	start[2] += 24.0f;
	VectorCopy(spot, end);
	end[2] += 24.0f;

	tr = gi.trace(start, NULL, NULL, end, self, MASK_SOLID);
	return tr.fraction == 1.0f;
}

static qboolean mytank_spawn_has_ground(edict_t *self, vec3_t mins, vec3_t maxs, vec3_t spot)
{
	vec3_t end;
	trace_t tr;

	VectorCopy(spot, end);
	end[2] -= 64.0f;
	tr = gi.trace(spot, mins, maxs, end, self, MASK_MONSTERSOLID);
	return !tr.startsolid && tr.fraction < 1.0f;
}

static qboolean mytank_valid_spawn_spot(edict_t *self, vec3_t mins, vec3_t maxs, vec3_t spot)
{
	static const float z_offsets[] = { 0.0f, 24.0f, -24.0f };
	vec3_t candidate;
	int i;

	for (i = 0; i < (int)(sizeof(z_offsets) / sizeof(z_offsets[0])); i++)
	{
		VectorCopy(spot, candidate);
		candidate[2] += z_offsets[i];

		if (!G_IsValidLocation(self, candidate, mins, maxs))
			continue;
		if (!mytank_spawn_has_ground(self, mins, maxs, candidate))
			continue;
		if (!mytank_spawn_has_clear_line(self, candidate))
			continue;

		VectorCopy(candidate, spot);
		return true;
	}

	return false;
}

static qboolean mytank_spawn_spot_overlaps_plan(tank_spawn_plan_t *plan, vec3_t spot)
{
	vec3_t delta;
	int i;

	for (i = 0; i < plan->count; i++)
	{
		VectorSubtract(spot, plan->spots[i], delta);
		delta[2] = 0;
		if (VectorLength(delta) < TANK_SPAWN_MIN_SEPARATION)
			return true;
	}

	return false;
}

static qboolean mytank_find_spawn_spot(edict_t *self, tank_spawn_plan_t *plan,
	vec3_t mins, vec3_t maxs, vec3_t spot)
{
	static const float offsets[][2] =
	{
		{ 80.0f, 0.0f },
		{ 40.0f, 60.0f },
		{ 40.0f, -60.0f },
		{ 0.0f, 80.0f },
		{ 0.0f, -80.0f },
		{ -72.0f, 0.0f },
		{ -40.0f, 72.0f },
		{ -40.0f, -72.0f },
		{ 120.0f, 0.0f },
		{ 0.0f, 120.0f },
		{ 0.0f, -120.0f },
		{ -120.0f, 0.0f }
	};
	vec3_t forward, right, candidate;
	int i;

	AngleVectors(self->s.angles, forward, right, NULL);

	for (i = 0; i < (int)(sizeof(offsets) / sizeof(offsets[0])); i++)
	{
		VectorCopy(self->s.origin, candidate);
		VectorMA(candidate, offsets[i][0], forward, candidate);
		VectorMA(candidate, offsets[i][1], right, candidate);
		candidate[2] += 8.0f;

		if (!mytank_valid_spawn_spot(self, mins, maxs, candidate))
			continue;
		if (mytank_spawn_spot_overlaps_plan(plan, candidate))
			continue;

		VectorCopy(candidate, spot);
		return true;
	}

	return false;
}

static qboolean mytank_plan_reinforcements(edict_t *self, tank_spawn_plan_t *plan)
{
	int desired;
	int i;

	memset(plan, 0, sizeof(*plan));
	desired = mytank_spawn_desired_count(self);

	for (i = 0; i < desired && plan->count < TANK_SPAWN_MAX_COUNT; i++)
	{
		int mtype = mytank_spawn_mtype(self);
		vec3_t mins, maxs, spot, size;
		float radius;

		mytank_spawn_bounds(mtype, mins, maxs);
		if (!mytank_find_spawn_spot(self, plan, mins, maxs, spot))
			continue;

		VectorSubtract(maxs, mins, size);
		radius = VectorLength(size) * 0.5f;

		plan->mtypes[plan->count] = mtype;
		VectorCopy(spot, plan->spots[plan->count]);
		VectorCopy(mins, plan->mins[plan->count]);
		VectorCopy(maxs, plan->maxs[plan->count]);
		plan->radius[plan->count] = radius;
		plan->count++;
	}

	return plan->count > 0;
}

static int mytank_live_reinforcement_count(edict_t *self)
{
	edict_t *scan;
	int count = 0;

	for (scan = g_edicts; scan < &g_edicts[globals.num_edicts]; scan++)
	{
		if (!scan->inuse || scan == self)
			continue;
		if (scan->owner != self || scan->activator != self)
			continue;
		if (!(scan->svflags & SVF_MONSTER))
			continue;
		if (!G_EntIsAlive(scan))
			continue;

		count++;
	}

	return count;
}

static float mytank_reinforcement_retry_delay(void)
{
	return TANK_SPAWN_RETRY_MIN + random() * (TANK_SPAWN_RETRY_MAX - TANK_SPAWN_RETRY_MIN);
}

static qboolean mytank_spawn_allowed_by_bonus(edict_t *self)
{
	if (self->monsterinfo.bonus_flags)
		return true;

	if ((self->flags & FL_CONVERTED) || (self->activator && self->activator->client))
		return false;

	return mytank_is_n64(self) && random() < 0.08f;
}

static qboolean mytank_reinforcement_state_blocks_spawn(edict_t *self)
{
	int live_count = mytank_live_reinforcement_count(self);

	if (live_count > 0)
	{
		self->monsterinfo.nextattack = live_count;
		return true;
	}

	if (self->monsterinfo.nextattack > 0)
	{
		self->monsterinfo.nextattack = 0;
		self->monsterinfo.melee_finished = level.time + mytank_reinforcement_retry_delay();
		return true;
	}

	return false;
}

static void mytank_setup_invasion_spawn(edict_t *spawned)
{
	if (!invasion->value)
		return;

	spawned->monsterinfo.aiflags &= ~AI_STAND_GROUND;
	spawned->monsterinfo.aiflags |= AI_FIND_NAVI;
	spawned->prev_navi = NULL;
	spawned->goalentity = NULL;
}

static void mytank_cleanup_failed_spawn(edict_t *spawned)
{
	DroneList_Remove(spawned);
	AI_EnemyRemoved(spawned);
	G_FreeEdict(spawned);
}

static qboolean mytank_spawn_reinforcement(edict_t *self, int mtype, vec3_t spot)
{
	edict_t *spawned;
	edict_t *summoner;
	qboolean force_start;
	vec3_t dir;

	spawned = G_Spawn();
	spawned->mtype = mtype;
	spawned->activator = self;
	spawned->owner = self;
	spawned->monsterinfo.level = self->monsterinfo.level;

	summoner = G_GetSummoner(self);
	if (summoner && summoner->inuse && summoner != self)
		spawned->creator = summoner;

	if (!M_Initialize(self, spawned, 0.0f))
	{
		G_FreeEdict(spawned);
		return false;
	}

	if (!G_IsValidLocation(self, spot, spawned->mins, spawned->maxs))
	{
		mytank_cleanup_failed_spawn(spawned);
		return false;
	}

	spawned->monsterinfo.cost = 0;
	spawned->s.effects |= EF_PLASMA;
	VectorCopy(spot, spawned->s.origin);
	VectorCopy(spot, spawned->s.old_origin);

	if (G_EntIsAlive(self->enemy))
	{
		VectorSubtract(self->enemy->s.origin, spot, dir);
		spawned->s.angles[YAW] = vectoyaw(dir);
	}
	else
		VectorCopy(self->s.angles, spawned->s.angles);

	spawned->nextthink = level.time + FRAMETIME;
	spawned->monsterinfo.attack_finished = level.time + 1.0f;
	mytank_setup_invasion_spawn(spawned);
	gi.linkentity(spawned);

	self->num_monsters_real++;

	force_start = invasion->value || pvm->value;
	if (G_ValidTarget(spawned, self->enemy, !force_start, true))
	{
		spawned->enemy = self->enemy;
		VectorCopy(self->enemy->s.origin, spawned->monsterinfo.last_sighting);
		if (spawned->monsterinfo.run)
			spawned->monsterinfo.run(spawned);
	}
	else if (force_start && drone_findtarget(spawned, true) && spawned->monsterinfo.run)
		spawned->monsterinfo.run(spawned);
	else if (spawned->monsterinfo.stand)
		spawned->monsterinfo.stand(spawned);

	return true;
}

static void mytank_spawn_spawngrows(edict_t *self)
{
	tank_spawn_plan_t plan;
	int i;

	if (!G_ValidTarget(self, self->enemy, true, true))
		return;
	if (mytank_reinforcement_state_blocks_spawn(self))
		return;

	if (!mytank_plan_reinforcements(self, &plan))
		return;

	for (i = 0; i < plan.count; i++)
	{
		vec3_t center, effect_origin;

		VectorAdd(plan.mins[i], plan.maxs[i], center);
		VectorScale(center, 0.5f, center);
		VectorAdd(plan.spots[i], center, effect_origin);
		SpawnGrow_Spawn(effect_origin, plan.radius[i], plan.radius[i] * 2.0f);
	}
}

void Monster_MoveSpawn(edict_t *self)
{
	tank_spawn_plan_t plan;
	int spawned = 0;
	int i;

	if (!self->groundentity || !G_ValidTarget(self, self->enemy, true, true))
	{
		self->monsterinfo.melee_finished = level.time + TANK_SPAWN_FAIL_COOLDOWN;
		return;
	}

	if (mytank_reinforcement_state_blocks_spawn(self))
		return;

	if (!mytank_plan_reinforcements(self, &plan))
	{
		self->monsterinfo.melee_finished = level.time + TANK_SPAWN_FAIL_COOLDOWN;
		return;
	}

	for (i = 0; i < plan.count; i++)
	{
		if (mytank_spawn_reinforcement(self, plan.mtypes[i], plan.spots[i]))
			spawned++;
	}

	if (spawned)
	{
		gi.sound(self, CHAN_WEAPON, sound_spawn, 1, ATTN_NORM, 0);
		self->monsterinfo.nextattack = spawned;
		self->monsterinfo.melee_finished = level.time;
	}
	else
		self->monsterinfo.melee_finished = level.time + TANK_SPAWN_FAIL_COOLDOWN;
}

static void mytank_spawn_punch(edict_t *self)
{
	vec3_t origin;

	myTankStrike(self);
	mytank_slam_origin(self, origin);
	mytank_slam_effect(origin);
	Monster_MoveSpawn(self);
}

static qboolean mytank_can_spawn_reinforcements(edict_t *self)
{
	tank_spawn_plan_t plan;

	if (!self->groundentity)
		return false;
	if (mytank_reinforcement_state_blocks_spawn(self))
		return false;
	if (self->monsterinfo.melee_finished > level.time)
		return false;
	if (!G_ValidTarget(self, self->enemy, true, true))
		return false;
	if (!infront(self, self->enemy))
		return false;
	if (!mytank_spawn_allowed_by_bonus(self))
		return false;
	return mytank_plan_reinforcements(self, &plan);
}

static void mytank_remove_reinforcements(edict_t *self)
{
	edict_t *scan;

	for (scan = g_edicts; scan < &g_edicts[globals.num_edicts]; scan++)
	{
		if (!scan->inuse || scan == self)
			continue;
		if (scan->owner != self || scan->activator != self)
			continue;
		if (!(scan->svflags & SVF_MONSTER))
			continue;
		if (!G_EntIsAlive(scan))
			continue;

		M_Remove(scan, false, true);
	}
}

void mytank_idle (edict_t *self)
{
	int		range;
	vec3_t	v;

	//GHz: Commanders teleport back to owner
	if (self->mtype == M_COMMANDER && (self->s.skinnum & 2) && !(self->monsterinfo.aiflags & AI_STAND_GROUND))
	{
		VectorSubtract(self->activator->s.origin, self->s.origin, v);
		range = VectorLength (v);
		if (range > 256)
		{
			TeleportNearTarget (self, self->activator, 16, true);
		}
	}
	gi.sound (self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
	self->superspeed = false; //GHz: No more sliding
}


//
// stand
//

mframe_t mytank_frames_stand []=
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
mmove_t	mytank_move_stand = {FRAME_stand01, FRAME_stand30, mytank_frames_stand, NULL};
	
void mytank_stand (edict_t *self)
{
	self->monsterinfo.currentmove = &mytank_move_stand;
}

void tank_walk (edict_t *self);

mframe_t tank_frames_start_walk [] =
{
	drone_ai_walk,  0, NULL,
	drone_ai_walk,  6, NULL,
	drone_ai_walk,  6, NULL,
	drone_ai_walk, 11, mytank_footstep
};
mmove_t	tank_move_start_walk = {FRAME_walk01, FRAME_walk04, tank_frames_start_walk, tank_walk};

mframe_t tank_frames_walk [] =
{
	drone_ai_walk, 4,	NULL,
	drone_ai_walk, 5,	NULL,
	drone_ai_walk, 3,	NULL,
	drone_ai_walk, 2,	NULL,
	drone_ai_walk, 5,	NULL,
	drone_ai_walk, 5,	NULL,
	drone_ai_walk, 4,	NULL,
	drone_ai_walk, 4,	mytank_footstep,
	drone_ai_walk, 3,	NULL,
	drone_ai_walk, 5,	NULL,
	drone_ai_walk, 4,	NULL,
	drone_ai_walk, 5,	NULL,
	drone_ai_walk, 7,	NULL,
	drone_ai_walk, 7,	NULL,
	drone_ai_walk, 6,	NULL,
	drone_ai_walk, 6,	mytank_footstep
};
mmove_t	tank_move_walk = {FRAME_walk05, FRAME_walk20, tank_frames_walk, NULL};

mframe_t tank_frames_stop_walk [] =
{
	drone_ai_walk,  3, NULL,
	drone_ai_walk,  3, NULL,
	drone_ai_walk,  2, NULL,
	drone_ai_walk,  2, NULL,
	drone_ai_walk,  4, mytank_footstep
};
mmove_t	tank_move_stop_walk = {FRAME_walk21, FRAME_walk25, tank_frames_stop_walk, mytank_stand};

void tank_walk (edict_t *self)
{
	//gi.dprintf("tank_walk called at %.1f on s.frame %d health %d deadflag %d\n", level.time, self->s.frame, self->health, self->deadflag);
	if (!self->goalentity)
		self->goalentity = world;
	self->monsterinfo.currentmove = &tank_move_walk;
}

//
// run
//

void mytank_run (edict_t *self);

mframe_t mytank_frames_start_run [] =
{
	drone_ai_run,  15, NULL,
	drone_ai_run,  15, NULL,
	drone_ai_run,  15, NULL,
	drone_ai_run, 15, mytank_footstep
};
mmove_t	mytank_move_start_run = {FRAME_walk01, FRAME_walk04, mytank_frames_start_run, mytank_run};

mframe_t mytank_frames_run [] =
{
	drone_ai_run, 15,	NULL,
	drone_ai_run, 15,	NULL,
	drone_ai_run, 15,	NULL,
	drone_ai_run, 15,	NULL,
	drone_ai_run, 15,	NULL,
	drone_ai_run, 15,	NULL,
	drone_ai_run, 15,	NULL,
	drone_ai_run, 15,	mytank_footstep,
	drone_ai_run, 15,	NULL,
	drone_ai_run, 15,	NULL,
	drone_ai_run, 15,	NULL,
	drone_ai_run, 15,	NULL,
	drone_ai_run, 15,	NULL,
	drone_ai_run, 15,	NULL,
	drone_ai_run, 15,	NULL,
	drone_ai_run, 15,	mytank_footstep
};
mmove_t	mytank_move_run = {FRAME_walk05, FRAME_walk20, mytank_frames_run, NULL};

void mytank_run (edict_t *self)
{
	//gi.dprintf("mytank_run called at %.1f, health %d deadflag %d\n", (level.time), self->health, self->deadflag);//DEBUG

	if (self->deadflag == DEAD_DEAD)
	{/*
		gi.dprintf("mytank_run called while dead! WTF!\n");
		if (self->monsterinfo.currentmove == &mytank_move_death)
			gi.dprintf("run called while in death animation\n");
		else if (self->monsterinfo.currentmove == &tank_move_walk)
			gi.dprintf("run called while monster in walk animation\n");
		else if (self->monsterinfo.currentmove == &mytank_move_run || self->monsterinfo.currentmove == &mytank_move_start_run)
			gi.dprintf("run called while monster in run animation\n");
	*/
		return;
	}
	if (self->enemy && self->enemy->client)
		self->monsterinfo.aiflags |= AI_BRUTAL;
	else
		self->monsterinfo.aiflags &= ~AI_BRUTAL;

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		self->monsterinfo.currentmove = &mytank_move_stand;
		return;
	}

	if (self->monsterinfo.currentmove == &mytank_move_start_run)
	{
		self->monsterinfo.currentmove = &mytank_move_run;
	}
	else
	{
		self->monsterinfo.currentmove = &mytank_move_start_run;
	}
}

//
// attacks
//

void myTankRail (edict_t *self)
{
	int		flash_number, damage;
	vec3_t	forward, start;

	if (self->s.frame == FRAME_attak110)
		flash_number = MZ2_TANK_BLASTER_1;
	else if (self->s.frame == FRAME_attak113)
		flash_number = MZ2_TANK_BLASTER_2;
	else
		flash_number = MZ2_TANK_BLASTER_3;

	damage = M_RAILGUN_DMG_BASE + M_RAILGUN_DMG_ADDON* drone_damagelevel(self); // dmg: tank_rail
	if (M_RAILGUN_DMG_MAX && damage > M_RAILGUN_DMG_MAX)
		damage = M_RAILGUN_DMG_MAX;

	MonsterAim(self, M_HITSCAN_INSTANT_ACC, 0, false, flash_number, forward, start);
	if (!M_MonsterHasClearShotFrom(self, start))
		return;

	monster_fire_railgun(self, start, forward, damage, damage, flash_number);
}

void myTankN64FireBlaster2(edict_t* self)
{
	int flash_number;
	int damage;
	int speed;
	vec3_t forward, start;

	if (!self->enemy || !self->enemy->inuse)
		return;

	flash_number = mytank_blaster_flash(self);
	damage = mytank_n64_damage(self, TANK_N64_BLASTER2_DAMAGE, TANK_N64_BLASTER2_ADDON);
	speed = TANK_N64_BLASTER2_SPEED;
	MonsterAim(self, M_PROJECTILE_ACC, speed, false, flash_number, forward, start);
	if (!M_MonsterHasClearShotFrom(self, start))
		return;

	monster_fire_blaster2(self, start, forward, damage, speed, EF_BLASTER, flash_number);
}

void myTankN64FireGrenade(edict_t* self)
{
	int flash_number;
	int damage;
	float radius;
	vec3_t forward, start;

	if (!self->enemy || !self->enemy->inuse)
		return;

	flash_number = mytank_blaster_flash(self);
	damage = mytank_n64_damage(self, TANK_N64_GRENADE_DAMAGE, TANK_N64_GRENADE_ADDON);
	MonsterAim(self, M_PROJECTILE_ACC, TANK_N64_GRENADE_SPEED, true, flash_number, forward, start);
	if (!M_MonsterHasClearShotFrom(self, start))
		return;

	radius = damage > 150 ? 150.0f : (float)damage;
	damage = (int)vrx_increase_monster_damage_by_talent(self->activator, damage);
	fire_grenade(self, start, forward, damage, TANK_N64_GRENADE_SPEED, 2.5, radius, damage);
	mytank_muzzleflash(self, start, MZ2_UNUSED);
	gi.sound(self, CHAN_WEAPON, sound_grenade, 1, ATTN_NORM, 0);
}

void myTankN64FireLightning(edict_t* self)
{
	int flash_number;
	int damage;
	vec3_t forward, start;

	if (!self->enemy || !self->enemy->inuse)
		return;

	flash_number = mytank_blaster_flash(self);
	MonsterAim(self, M_HITSCAN_INSTANT_ACC, 0, false, flash_number, forward, start);
	if (!M_MonsterHasClearShotFrom(self, start))
		return;

	damage = mytank_n64_damage(self, TANK_N64_LIGHTNING_DAMAGE, TANK_N64_LIGHTNING_ADDON);
	mytank_n64_lightning_effect(self, start, forward);
	damage = (int)vrx_increase_monster_damage_by_talent(self->activator, damage);
	fire_bullet(self, start, forward, damage, 18, 0, 0, MOD_LIGHTNING);
	mytank_muzzleflash(self, start, flash_number);
}

void myTankN64Blaster(edict_t* self)
{
	if (self->style == TANK_N64_ATTACK_GRENADE)
	{
		myTankN64FireGrenade(self);
		return;
	}
	if (self->style == TANK_N64_ATTACK_LIGHTNING)
	{
		myTankN64FireLightning(self);
		return;
	}

	myTankN64FireBlaster2(self);
}

void myTankBlaster(edict_t* self)
{
	int		flash_number, speed, damage;
	vec3_t	forward, start;

	if (mytank_is_n64(self))
	{
		myTankN64Blaster(self);
		return;
	}

	// alternate attack for commander
	if (self->s.skinnum & 2)
	{
		myTankRail(self);
		return;
	}

	if (self->s.frame == FRAME_attak110)
		flash_number = MZ2_TANK_BLASTER_1;
	else if (self->s.frame == FRAME_attak113)
		flash_number = MZ2_TANK_BLASTER_2;
	else
		flash_number = MZ2_TANK_BLASTER_3;

	damage = M_BLASTER_DMG_BASE + M_BLASTER_DMG_ADDON * drone_damagelevel(self);
	if (M_BLASTER_DMG_MAX && damage > M_BLASTER_DMG_MAX)
		damage = M_BLASTER_DMG_MAX;

	speed = M_BLASTER_SPEED_BASE + M_BLASTER_SPEED_ADDON * drone_damagelevel(self);
	if (M_BLASTER_SPEED_MAX && speed > M_BLASTER_SPEED_MAX)
		speed = M_BLASTER_SPEED_MAX;

	MonsterAim(self, M_PROJECTILE_ACC, speed, false, flash_number, forward, start);
	if (!M_MonsterHasClearShotFrom(self, start))
		return;

	monster_fire_blaster(self, start, forward, damage, speed, EF_BLASTER, BLASTER_PROJ_BOLT, 2.0, true, flash_number);
}

void myTankStrike (edict_t *self)
{
	gi.sound (self, CHAN_WEAPON, sound_strike, 1, ATTN_NORM, 0);
}	

void myTankRocket(edict_t* self)
{
	int		flash_number, damage, speed;
	vec3_t	forward, start;
	qboolean use_heat;

	// sanity check
	if (!self->enemy || !self->enemy->inuse)
		return;

	if (self->s.frame == FRAME_attak324)
		flash_number = MZ2_TANK_ROCKET_1;
	else if (self->s.frame == FRAME_attak327)
		flash_number = MZ2_TANK_ROCKET_2;
	else
		flash_number = MZ2_TANK_ROCKET_3;

	use_heat = mytank_is_n64(self) && (self->count || random() < 0.35f);
	if (use_heat)
	{
		damage = mytank_n64_damage(self, TANK_N64_HEAT_DAMAGE, TANK_N64_HEAT_ADDON);
		speed = TANK_N64_HEAT_SPEED + 20 * drone_damagelevel(self);
	}
	else
	{
		damage = M_ROCKETLAUNCHER_DMG_BASE + M_ROCKETLAUNCHER_DMG_ADDON * drone_damagelevel(self);
		if (M_ROCKETLAUNCHER_DMG_MAX && damage > M_ROCKETLAUNCHER_DMG_MAX)
			damage = M_ROCKETLAUNCHER_DMG_MAX;
		speed = M_ROCKETLAUNCHER_SPEED_BASE + M_ROCKETLAUNCHER_SPEED_ADDON * drone_damagelevel(self);
		if (M_ROCKETLAUNCHER_SPEED_MAX && speed > M_ROCKETLAUNCHER_SPEED_MAX)
			speed = M_ROCKETLAUNCHER_SPEED_MAX;
	}

	MonsterAim(self, M_PROJECTILE_ACC, speed, true, flash_number, forward, start);
	if (!M_MonsterHasClearShotFrom(self, start))
		return;

	if (use_heat)
		monster_fire_heat(self, start, forward, damage, speed, flash_number,
			self->accel > 0 ? self->accel : TANK_N64_HEAT_TURN_FRACTION);
	else
		monster_fire_rocket(self, start, forward, damage, speed, flash_number);
}

void myTankMachineGun(edict_t* self)
{
	vec3_t	forward, right, start, dir, vec, forward_right;
	int		flash_number, damage, speed;

	// sanity check
	if (!self->enemy || !self->enemy->inuse)
		return;

	flash_number = MZ2_TANK_MACHINEGUN_1 + (self->s.frame - FRAME_attak406);

	// tank machinegun does 2x damage compared to other monster machineguns
	damage = 2 * (M_MACHINEGUN_DMG_BASE + M_MACHINEGUN_DMG_ADDON * drone_damagelevel(self));
	if (M_MACHINEGUN_DMG_MAX && damage > 2 * M_MACHINEGUN_DMG_MAX)
		damage = 2 * M_MACHINEGUN_DMG_MAX;

	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, monster_flash_offset[flash_number], forward, right, start);

	if (self->enemy)
	{
		VectorCopy(self->enemy->s.origin, vec);
		vec[2] += self->enemy->viewheight;
		VectorSubtract(vec, start, vec);
		vectoangles(vec, dir);
	}
	else
		dir[0] = 0;
	if (self->s.frame <= FRAME_attak415)
		dir[1] = self->s.angles[1] - 8 * (self->s.frame - FRAME_attak411);
	else
		dir[1] = self->s.angles[1] + 8 * (self->s.frame - FRAME_attak419);
	dir[2] = 0;

	AngleVectors(dir, forward, NULL, NULL);

	if (!M_MonsterHasClearShotFrom(self, start))
		return;

	if (mytank_is_n64(self))
	{
		damage = mytank_n64_damage(self, TANK_N64_FLECHETTE_DAMAGE, TANK_N64_FLECHETTE_ADDON);
		speed = TANK_N64_FLECHETTE_SPEED + 25 * drone_damagelevel(self);
		if (speed > TANK_N64_FLECHETTE_SPEED_MAX)
			speed = TANK_N64_FLECHETTE_SPEED_MAX;

		damage = (int)vrx_increase_monster_damage_by_talent(self->activator, damage);
		fire_flechette(self, start, forward, damage, speed, damage / 2);

		AngleVectors(dir, NULL, right, NULL);
		VectorMA(forward, 0.05f, right, forward_right);
		VectorNormalize(forward_right);
		fire_flechette(self, start, forward_right, damage, speed, damage / 2);
		mytank_muzzleflash(self, start, flash_number);
		return;
	}

	monster_fire_bullet(self, start, forward, damage, 40,
		DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flash_number);
}

mframe_t mytank_frames_attack_blast [] =
{
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	myTankBlaster,		// 10
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	myTankBlaster,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	myTankBlaster			// 16
};
mmove_t mytank_move_attack_blast = {FRAME_attak101, FRAME_attak116, mytank_frames_attack_blast, mytank_reattack_blaster};

mframe_t mytank_frames_reattack_blast [] =
{
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	myTankBlaster,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	myTankBlaster			// 16
};
mmove_t mytank_move_reattack_blast = {FRAME_attak111, FRAME_attak116, mytank_frames_reattack_blast, mytank_reattack_blaster};

mframe_t mytank_n64_frames_attack_blaster2 [] =
{
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	myTankN64FireBlaster2,
	ai_charge, 0,	myTankN64FireBlaster2,
	ai_charge, 0,	myTankN64FireBlaster2,
	ai_charge, 0,	myTankN64FireBlaster2,
	ai_charge, 0,	myTankN64FireBlaster2,
	ai_charge, 0,	myTankN64FireBlaster2,
	ai_charge, 0,	myTankN64FireBlaster2
};
mmove_t mytank_move_n64_attack_blaster2 = {FRAME_attak101, FRAME_attak116, mytank_n64_frames_attack_blaster2, mytank_reattack_n64_blaster2};

mframe_t mytank_n64_frames_reattack_blaster2 [] =
{
	ai_charge, 0,	myTankN64FireBlaster2,
	ai_charge, 0,	myTankN64FireBlaster2,
	ai_charge, 0,	myTankN64FireBlaster2,
	ai_charge, 0,	myTankN64FireBlaster2,
	ai_charge, 0,	myTankN64FireBlaster2,
	ai_charge, 0,	myTankN64FireBlaster2
};
mmove_t mytank_move_n64_reattack_blaster2 = {FRAME_attak111, FRAME_attak116, mytank_n64_frames_reattack_blaster2, mytank_reattack_n64_blaster2};

mframe_t mytank_n64_frames_attack_grenade [] =
{
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	myTankN64FireGrenade,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	myTankN64FireGrenade,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	myTankN64FireGrenade
};
mmove_t mytank_move_n64_attack_grenade = {FRAME_attak101, FRAME_attak116, mytank_n64_frames_attack_grenade, mytank_reattack_n64_grenade};

mframe_t mytank_n64_frames_reattack_grenade [] =
{
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	myTankN64FireGrenade,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	myTankN64FireGrenade
};
mmove_t mytank_move_n64_reattack_grenade = {FRAME_attak111, FRAME_attak116, mytank_n64_frames_reattack_grenade, mytank_reattack_n64_grenade};

mframe_t mytank_n64_frames_attack_lightning [] =
{
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	NULL,
	ai_charge, 0,	myTankN64FireLightning,
	ai_charge, 0,	myTankN64FireLightning,
	ai_charge, 0,	myTankN64FireLightning,
	ai_charge, 0,	myTankN64FireLightning,
	ai_charge, 0,	myTankN64FireLightning,
	ai_charge, 0,	myTankN64FireLightning,
	ai_charge, 0,	myTankN64FireLightning
};
mmove_t mytank_move_n64_attack_lightning = {FRAME_attak101, FRAME_attak116, mytank_n64_frames_attack_lightning, mytank_reattack_n64_lightning};

mframe_t mytank_n64_frames_reattack_lightning [] =
{
	ai_charge, 0,	myTankN64FireLightning,
	ai_charge, 0,	myTankN64FireLightning,
	ai_charge, 0,	myTankN64FireLightning,
	ai_charge, 0,	myTankN64FireLightning,
	ai_charge, 0,	myTankN64FireLightning,
	ai_charge, 0,	myTankN64FireLightning
};
mmove_t mytank_move_n64_reattack_lightning = {FRAME_attak111, FRAME_attak116, mytank_n64_frames_reattack_lightning, mytank_reattack_n64_lightning};

void mytank_delay (edict_t *self)
{
	if (!self->enemy || !self->enemy->inuse)
		return;

	// delay next attack if we're not standing ground, our enemy isn't within rocket range
	// (we need to get closer) and we are not a tank commander/boss
	if (!(self->monsterinfo.aiflags & AI_STAND_GROUND) && (entdist(self, self->enemy) > 512) 
		&& (self->monsterinfo.control_cost < M_COMMANDER_CONTROL_COST))
		self->monsterinfo.attack_finished = level.time + GetRandom(20, 30)*FRAMETIME;
}

mframe_t mytank_frames_attack_post_blast [] =	
{
	ai_move, 0,		NULL,				// 17
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,
	ai_move, 0,		NULL,//mytank_delay,
	ai_move, 0,	mytank_footstep		// 22
};
mmove_t mytank_move_attack_post_blast = {FRAME_attak117, FRAME_attak122, mytank_frames_attack_post_blast, mytank_run};

void mytank_reattack_blaster(edict_t* self)
{
	M_ContinueAttack(self, &mytank_move_reattack_blast,
		&mytank_move_attack_post_blast, 0, 768, 0.8);
}

void mytank_reattack_n64_blaster2(edict_t* self)
{
	self->style = TANK_N64_ATTACK_BLASTER2;
	M_ContinueAttack(self, &mytank_move_n64_reattack_blaster2,
		&mytank_move_attack_post_blast, 0, 768, 0.8);
}

void mytank_reattack_n64_grenade(edict_t* self)
{
	self->style = TANK_N64_ATTACK_GRENADE;
	M_ContinueAttack(self, &mytank_move_n64_reattack_grenade,
		&mytank_move_attack_post_blast, 0, 768, 0.75);
}

void mytank_reattack_n64_lightning(edict_t* self)
{
	self->style = TANK_N64_ATTACK_LIGHTNING;
	M_ContinueAttack(self, &mytank_move_n64_reattack_lightning,
		&mytank_move_attack_post_blast, 0, 768, 0.8);
}


void mytank_poststrike (edict_t *self)
{
	self->enemy = NULL;
	mytank_run (self);
}

mframe_t mytank_frames_attack_strike [] =
{
	ai_move, 3,   NULL,
	ai_move, 2,   NULL,
	ai_move, 2,   NULL,
	ai_move, 1,   NULL,
	ai_move, 6,   NULL,
	ai_move, 7,   NULL,
	ai_move, 9,   mytank_footstep,
	ai_move, 2,   NULL,
	ai_move, 1,   NULL,
	ai_move, 2,   NULL,
	ai_move, 2,   mytank_footstep,
	ai_move, 2,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, -2,  NULL,
	ai_move, -2,  NULL,
	ai_move, 0,   mytank_windup,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   myTankStrike,
	ai_move, 0,   NULL,
	ai_move, -1,  NULL,
	ai_move, -1,  NULL,
	ai_move, -1,  NULL,
	ai_move, -1,  NULL,
	ai_move, -1,  NULL,
	ai_move, -3,  NULL,
	ai_move, -10, NULL,
	ai_move, -10, NULL,
	ai_move, -2,  NULL,
	ai_move, -3,  NULL,
	ai_move, -2,  mytank_footstep
};
mmove_t mytank_move_attack_strike = {FRAME_attak201, FRAME_attak238, mytank_frames_attack_strike, mytank_poststrike};

mframe_t mytank_frames_strike [] =
{
	//ai_move, 0,   mytank_windup,
	//ai_move, 0,   NULL,
	//ai_move, 0,   NULL,
	ai_move, 0,   mytank_windup,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   mytank_meleeattack,
};
mmove_t mytank_move_strike = {FRAME_attak222, FRAME_attak226, mytank_frames_strike, mytank_restrike};

mframe_t mytank_frames_post_strike [] =
{
	ai_move, 0,   NULL,
	ai_move, -1,  NULL,
	ai_move, -1,  NULL,
	ai_move, -1,  NULL,
	ai_move, -1,  NULL,
	ai_move, -1,  NULL,
	ai_move, -3,  NULL,
	ai_move, -10, NULL,
	ai_move, -10, NULL,
	ai_move, -2,  NULL,
	ai_move, -3,  NULL,//mytank_delay,
	ai_move, -2,  mytank_footstep
};
mmove_t mytank_move_post_strike = {FRAME_attak227, FRAME_attak238, mytank_frames_post_strike, mytank_run};

mframe_t mytank_frames_spawn [] =
{
	ai_move, 0,   NULL,						// 221
	ai_move, 0,   mytank_windup,			// 222
	ai_move, 0,   NULL,						// 223
	ai_move, 0,   mytank_spawn_spawngrows,	// 224
	ai_move, 0,   mytank_spawn_spawngrows,	// 225
	ai_move, 0,   mytank_spawn_punch,		// 226
	ai_move, 0,   NULL,						// 227
	ai_move, -1,  NULL,
	ai_move, -1,  NULL,
	ai_move, -1,  NULL,
	ai_move, -1,  NULL,
	ai_move, -1,  NULL,
	ai_move, -3,  NULL,
	ai_move, -10, NULL,
	ai_move, -10, NULL,
	ai_move, -2,  NULL,
	ai_move, -3,  NULL,
	ai_move, -2,  mytank_footstep			// 238
};
mmove_t mytank_move_spawn = {FRAME_attak221, FRAME_attak238, mytank_frames_spawn, mytank_run};

mframe_t mytank_frames_attack_pre_rocket [] =
{
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,			// 10

	ai_charge, 0,  NULL,
	ai_charge, 1,  NULL,
	ai_charge, 2,  NULL,
	ai_charge, 7,  NULL,
	ai_charge, 7,  NULL,
	ai_charge, 7,  mytank_footstep,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,			// 20

	ai_charge, -3, NULL
};
mmove_t mytank_move_attack_pre_rocket = {FRAME_attak301, FRAME_attak321, mytank_frames_attack_pre_rocket, mytank_doattack_rocket};

mframe_t mytank_frames_attack_fire_rocket [] =
{
	ai_charge, 0, NULL,			// Loop Start	22 
	ai_charge, 0,  NULL,
	ai_charge, 0,  myTankRocket,		// 24
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  myTankRocket,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0, myTankRocket		// 30	Loop End
};
mmove_t mytank_move_attack_fire_rocket = {FRAME_attak322, FRAME_attak330, mytank_frames_attack_fire_rocket, mytank_refire_rocket};

mframe_t mytank_frames_attack_post_rocket [] =
{	
	ai_charge, 0,  NULL,			// 31
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,			// 40

	ai_charge, 0,  NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, mytank_footstep,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,			// 50

	ai_charge, 0,  NULL,
	ai_charge, 0,  NULL,
	ai_charge, 0,  mytank_delay
};
mmove_t mytank_move_attack_post_rocket = {FRAME_attak331, FRAME_attak353, mytank_frames_attack_post_rocket, mytank_run};

mframe_t mytank_frames_attack_chain_start [] =
{
	ai_charge, 0, NULL,	// 168
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL	// 172
};
mmove_t mytank_move_attack_chain_start = {FRAME_attak401, FRAME_attak405, mytank_frames_attack_chain_start, mytank_attack_chain};

mframe_t mytank_frames_attack_chain_end [] =
{
	ai_charge, 0, NULL,	// 192
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, mytank_delay	// 196
};
mmove_t mytank_move_attack_chain_end = {FRAME_attak425, FRAME_attak429, mytank_frames_attack_chain_end, mytank_run};

mframe_t mytank_frames_attack_chain [] =
{
	ai_charge,		0, myTankMachineGun,	// 173
	ai_charge,      0, myTankMachineGun,
	ai_charge,      0, myTankMachineGun,
	ai_charge,      0, myTankMachineGun,
	ai_charge,      0, myTankMachineGun,
	ai_charge,      0, myTankMachineGun,
	ai_charge,      0, myTankMachineGun,
	ai_charge,      0, myTankMachineGun,
	ai_charge,      0, myTankMachineGun,
	ai_charge,      0, myTankMachineGun,
	ai_charge,      0, myTankMachineGun,
	ai_charge,      0, myTankMachineGun,
	ai_charge,      0, myTankMachineGun,
	ai_charge,      0, myTankMachineGun,
	ai_charge,      0, myTankMachineGun,
	ai_charge,      0, myTankMachineGun,
	ai_charge,      0, myTankMachineGun,
	ai_charge,      0, myTankMachineGun,
	ai_charge,      0, myTankMachineGun		// 191
};
mmove_t mytank_move_attack_chain = {FRAME_attak406, FRAME_attak424, mytank_frames_attack_chain, mytank_chain_refire};

void mytank_attack_chain (edict_t *self)
{
	// continue attack sequence unless enemy is no longer valid
	if (G_ValidTarget(self, self->enemy, true, true))
		self->monsterinfo.currentmove = &mytank_move_attack_chain;
	else
		mytank_run(self);
}

void mytank_chain_refire(edict_t* self)
{
	M_ContinueAttack(self, &mytank_move_attack_chain,
		&mytank_move_attack_chain_end, 0, 8192, 0.8);
}

void mytank_restrike(edict_t* self)
{
	M_ContinueAttack(self, &mytank_move_strike,
		&mytank_move_post_strike, 0, 128, 0.66);
}

void mytank_refire_rocket(edict_t* self)
{
	M_ContinueAttack(self, &mytank_move_attack_fire_rocket,
		&mytank_move_attack_post_rocket, 0, 512, 0.8);
}

void mytank_doattack_rocket (edict_t *self)
{
	self->monsterinfo.currentmove = &mytank_move_attack_fire_rocket;
}

static qboolean mytank_should_melee(edict_t *self, float range)
{
	return self->groundentity && range <= 144.0f;
}

void mytank_melee (edict_t *self)
{
	
}

void mytank_meleeattack (edict_t *self)
{
	int damage;
	trace_t tr;
	edict_t *other=NULL;
	vec3_t	v, damage_origin;

	// tank must be on the ground to punch
	if (!self->groundentity)
		return;

	self->lastsound = level.framenum;

	damage = M_MELEE_DMG_BASE + M_MELEE_DMG_ADDON * drone_damagelevel(self); // dmg: berserker_attack_strike
	if (M_MELEE_DMG_MAX && damage > M_MELEE_DMG_MAX)
		damage = M_MELEE_DMG_MAX;

	gi.sound (self, CHAN_AUTO, gi.soundindex ("tank/tnkatck5.wav"), 1, ATTN_NORM, 0);
	mytank_slam_origin(self, damage_origin);
	mytank_slam_effect(damage_origin);
	
	while ((other = findradius(other, damage_origin, 128)) != NULL)
	{
		if (!G_ValidTarget(self, other, true, true))
			continue;
		// miss the attack if we are cursed/confused
		if (que_typeexists(self->curses, CURSE) && rand() > 0.2)
			continue;
		// bosses don't have to be facing their enemy, others do
		//if ((self->monsterinfo.control_cost < 3) && !nearfov(self, other, 0, 60))//!infront(self, other))
		//	continue;

		VectorSubtract(other->s.origin, damage_origin, v);
		VectorNormalize(v);
		tr = gi.trace(damage_origin, NULL, NULL, other->s.origin, self, (MASK_PLAYERSOLID | MASK_MONSTERSOLID));
		T_Damage (other, self, self, v, tr.endpos, tr.plane.normal, damage, 200, 0, MOD_TANK_PUNCH);
		//other->velocity[2] += 200;//damage / 2;
	}
}

/*
void mytank_meleeattack (edict_t *self)
{
	int			damage;
	vec3_t		v;
	edict_t		*other = NULL;
	trace_t		tr;

	if (!self->enemy)
		return;
	if (!self->enemy->inuse)
		return;
	if (self->enemy->health <= 0)
		return;

	damage = 100 + 20*self->monsterinfo.level;

	while ((other = findradius(other, self->s.origin, 256)) != NULL)
	{
		if (other == self)
			continue;
		if (!other->inuse)
			continue;
		if (!other->takedamage)
			continue;
		if (other->solid == SOLID_NOT)
			continue;
		if (!other->groundentity)
			continue;
		if (OnSameTeam(self, other))
			continue;
		if (!visible(self, other))
			continue;
		VectorSubtract(other->s.origin, self->s.origin, v);
		VectorNormalize(v);
		tr = gi.trace(self->s.origin, NULL, NULL, other->s.origin, self, (MASK_PLAYERSOLID | MASK_MONSTERSOLID));
		T_Damage (other, self, self, v, other->s.origin, tr.plane.normal, damage, damage, 0, MOD_UNKNOWN);
		other->velocity[2] += damage / 2;
	}
	myTankStrike(self);
}
*/


/*
qboolean TeleportNearTarget (edict_t *self, edict_t *target)
{
	int		x;
	vec3_t	forward, right, start, point, dir;
	trace_t tr;

	//GHz: Get starting position and relative angles
	VectorCopy(target->s.origin, start);
	start[2]++;
	AngleVectors(self->s.angles, forward, right, NULL);

	for (x = 0; x < 4; x++)
	{
		//GHz: Get direction
		switch (x)
		{
		case 0: VectorCopy(forward, dir);break;
		case 1: VectorCopy(right, dir);break;
		case 2: VectorInverse(forward);VectorCopy(forward, dir);break;
		case 3: VectorInverse(right);VectorCopy(right, dir);break;
		}

		//GHz: Check target for valid spot
		VectorMA(start, 75, dir, start);
		tr = gi.trace(start, self->mins, self->maxs, start, target, MASK_SHOT);
		if (!(tr.contents & MASK_SHOT))
		{
			//GHz: Check for spot for landing
			VectorCopy(start, point);
			point[2] -= 32;
			tr = gi.trace(start, NULL, NULL, point, NULL, MASK_SHOT);
			if (tr.fraction != 1.0)
			{
				//self->s.event = EV_PLAYER_TELEPORT;
				gi.WriteByte (svc_temp_entity);
				gi.WriteByte (TE_BOSSTPORT);
				gi.WritePosition (self->s.origin);
				gi.multicast (self->s.origin, MULTICAST_PVS);

				gi.WriteByte (svc_temp_entity);
				gi.WriteByte (TE_BOSSTPORT);
				gi.WritePosition (start);
				gi.multicast (start, MULTICAST_PVS);

				VectorCopy(start, self->s.origin);
				return true;
			}
		}
	}
	return false;
}
*/

void commander_attack (edict_t *self)
{
	const float r = random();
	float range = entdist(self, self->enemy);
	qboolean can_blast;
	qboolean can_rocket;

	if (mytank_should_melee(self, range))
	{
		self->monsterinfo.currentmove = &mytank_move_strike;
		self->monsterinfo.attack_finished = level.time + 2.0;
		return;
	}

	// short range attack
	if (range <= 128 && r <= 0.6)
	{
		self->monsterinfo.currentmove = &mytank_move_strike;
	}
	else
	{
		// try to teleport to enemy if we are not standing ground
		if (!(self->monsterinfo.aiflags & AI_STAND_GROUND))
		{
			if (TeleportNearTarget(self, self->enemy, 16.0, true))
			{
				range = entdist(self, self->enemy);
				if (mytank_should_melee(self, range))
				{
					self->monsterinfo.currentmove = &mytank_move_strike;
					self->monsterinfo.attack_finished = level.time + 0.5;
					return;
				}
			}
		}

		can_blast = mytank_can_blaster(self);
		can_rocket = mytank_can_rocket(self);

		// medium range attack
		if (range <= 512)
		{
			if (r <= 0.2 && can_blast)
				self->monsterinfo.currentmove = &mytank_move_attack_blast;
			else if (can_rocket)
				self->monsterinfo.currentmove = &mytank_move_attack_fire_rocket;
			else if (can_blast)
				self->monsterinfo.currentmove = &mytank_move_attack_blast;
			else
				return;
		}
		// long range attack
		else
		{
			if (can_blast)
				self->monsterinfo.currentmove = &mytank_move_attack_blast;
			else if (can_rocket)
				self->monsterinfo.currentmove = &mytank_move_attack_fire_rocket;
			else
				return;
		}
	}

	// don't call attack function for awhile
	self->monsterinfo.attack_finished = level.time + 2.0;
}

static void tank_n64_select_blaster_attack(edict_t *self, float grenade_chance)
{
	int damage;

	damage = mytank_n64_damage(self, TANK_N64_BLASTER2_DAMAGE, TANK_N64_BLASTER2_ADDON);
	if (mytank_n64_enemy_almost_dead(self, damage) && random() < 0.55f)
	{
		self->style = TANK_N64_ATTACK_LIGHTNING;
		self->monsterinfo.currentmove = &mytank_move_n64_attack_lightning;
		return;
	}

	if (random() < grenade_chance)
	{
		self->style = TANK_N64_ATTACK_GRENADE;
		self->monsterinfo.currentmove = &mytank_move_n64_attack_grenade;
		return;
	}

	self->style = TANK_N64_ATTACK_BLASTER2;
	self->monsterinfo.currentmove = &mytank_move_n64_attack_blaster2;
}

void tank_attack_n64(edict_t* self)
{
	const float r = random();
	const float range = entdist(self, self->enemy);
	qboolean can_blast;
	qboolean can_rocket;
	qboolean can_chain;

	if (mytank_should_melee(self, range))
	{
		self->monsterinfo.currentmove = &mytank_move_strike;
		M_DelayNextAttack(self, 0, true);
		return;
	}

	if (mytank_can_spawn_reinforcements(self))
	{
		self->monsterinfo.currentmove = &mytank_move_spawn;
		M_DelayNextAttack(self, 0, true);
		return;
	}

	can_blast = mytank_can_blaster(self);
	can_rocket = mytank_can_rocket(self);
	can_chain = mytank_can_chain(self);

	if (range <= 128)
	{
		if (can_chain && r < 0.5f)
			self->monsterinfo.currentmove = &mytank_move_attack_chain;
		else if (can_blast)
			tank_n64_select_blaster_attack(self, 0.7f);
		else if (can_rocket)
			self->monsterinfo.currentmove = &mytank_move_attack_fire_rocket;
		else
			return;
	}
	else if (range <= 250)
	{
		if (can_chain && r < 0.25f)
			self->monsterinfo.currentmove = &mytank_move_attack_chain;
		else if (can_blast)
			tank_n64_select_blaster_attack(self, 0.5f);
		else if (can_rocket)
			self->monsterinfo.currentmove = &mytank_move_attack_fire_rocket;
		else if (can_chain)
			self->monsterinfo.currentmove = &mytank_move_attack_chain;
		else
			return;
	}
	else
	{
		if (can_chain && r < 0.33f)
			self->monsterinfo.currentmove = &mytank_move_attack_chain;
		else if (can_rocket && r < 0.66f)
			self->monsterinfo.currentmove = &mytank_move_attack_fire_rocket;
		else if (can_blast)
			tank_n64_select_blaster_attack(self, 0.5f);
		else if (can_rocket)
			self->monsterinfo.currentmove = &mytank_move_attack_fire_rocket;
		else if (can_chain)
			self->monsterinfo.currentmove = &mytank_move_attack_chain;
		else
			return;
	}

	M_DelayNextAttack(self, 0, true);
}

void tank_attack(edict_t* self)
{
	const float r = random();
	const float range = entdist(self, self->enemy);
	qboolean can_blast;
	qboolean can_rocket;
	qboolean can_chain;

	//gi.dprintf("%d tank_attack()\n", level.framenum);

	if (mytank_should_melee(self, range))
	{
		self->monsterinfo.currentmove = &mytank_move_strike;
		M_DelayNextAttack(self, 0, true);
		return;
	}

	if (mytank_can_spawn_reinforcements(self))
	{
		self->monsterinfo.currentmove = &mytank_move_spawn;
		M_DelayNextAttack(self, 0, true);
		return;
	}

	can_blast = mytank_can_blaster(self);
	can_rocket = mytank_can_rocket(self);
	can_chain = mytank_can_chain(self);

	// short range attack (60% strike, then 20% blaster, 80% rocket)
	if (range <= 128)
	{
		if ((!self->enemy->client || r <= 0.6) && self->groundentity)
		{
			self->monsterinfo.currentmove = &mytank_move_strike;
		}
		else
		{
			if (r <= 0.2 && can_blast)
				self->monsterinfo.currentmove = &mytank_move_attack_blast;
			else if (can_rocket)
				self->monsterinfo.currentmove = &mytank_move_attack_fire_rocket;
			else if (can_blast)
				self->monsterinfo.currentmove = &mytank_move_attack_blast;
			else if (can_chain)
				self->monsterinfo.currentmove = &mytank_move_attack_chain;
			else
				return;
		}
	}
	// medium range attack (20% chain, 40% blaster, 40% rocket)
	else if (range <= 512)
	{
		if (r <= 0.2 && can_chain)
			self->monsterinfo.currentmove = &mytank_move_attack_chain;
		else if (r <= 0.6 && can_blast)
			self->monsterinfo.currentmove = &mytank_move_attack_blast;
		else if (can_rocket)
			self->monsterinfo.currentmove = &mytank_move_attack_fire_rocket;
		else if (can_blast)
			self->monsterinfo.currentmove = &mytank_move_attack_blast;
		else if (can_chain)
			self->monsterinfo.currentmove = &mytank_move_attack_chain;
		else
			return;
	}
	// long range attack (20% blaster, 80% chain)
	else
	{
		if (r <= 0.2 && can_blast)
			self->monsterinfo.currentmove = &mytank_move_attack_blast;
		else if (can_chain)
			self->monsterinfo.currentmove = &mytank_move_attack_chain;
		else if (can_blast)
			self->monsterinfo.currentmove = &mytank_move_attack_blast;
		else
			return;
	}

	M_DelayNextAttack(self, 0, true);
}

void mytank_attack (edict_t *self)
{
	if (mytank_is_n64(self))
		tank_attack_n64(self);
	else if (self->s.skinnum & 2)
		commander_attack(self);
	else
		tank_attack(self);
}

void mytank_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
	//mytank_attack(self);
}

void tank_nextmove(edict_t* self)
{
	if (G_EntExists(self->enemy))
		mytank_run(self);
	else
		tank_walk(self);
}

// pain
mframe_t tank_frames_pain_long[] =
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
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
};
mmove_t tank_move_pain_long = { FRAME_pain301, FRAME_pain316, tank_frames_pain_long, tank_nextmove };

mframe_t tank_frames_pain_short1[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
};
mmove_t tank_move_pain_short1 = { FRAME_pain201, FRAME_pain205, tank_frames_pain_short1, tank_nextmove };

mframe_t tank_frames_pain_short2[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
};

mmove_t tank_move_pain_short2 = { FRAME_pain101, FRAME_pain104, tank_frames_pain_short2, tank_nextmove };

void tank_pain(edict_t* self, edict_t* other, float kick, int damage)
{
	const double rng = random();
	const qboolean is_idling = self->monsterinfo.currentmove == &tank_move_start_walk ||
		self->monsterinfo.currentmove == &tank_move_stop_walk ||
		self->monsterinfo.currentmove == &mytank_move_stand;
	const qboolean moving_without_enemy = self->monsterinfo.currentmove == &tank_move_walk && !self->enemy;

	if (self->health < (self->max_health / 2))
		self->s.skinnum |= 1;

	// we're already in a pain state
	if (self->monsterinfo.currentmove == &tank_move_pain_long ||
		self->monsterinfo.currentmove == &tank_move_pain_short1 ||
		self->monsterinfo.currentmove == &tank_move_pain_short2)
		return;

	// player-spawned monsters don't get pain state induced
	if (G_GetClient(self))
		return;

	// no pain in invasion hard mode
	if (invasion->value == 2)
		return;

	// if we're fidgeting, always go into pain state.
	if (rng <= (1.0f - self->monsterinfo.pain_chance) &&
		!(is_idling || moving_without_enemy))
		return;

	gi.sound(self, CHAN_VOICE, self->mtype == M_COMMANDER ? sound_pain2 : sound_pain, 1, ATTN_NORM, 0);

	if (is_idling || moving_without_enemy) 
		self->monsterinfo.currentmove = &tank_move_pain_long;
	else {
		if (random() < 0.5)
			self->monsterinfo.currentmove = &tank_move_pain_short1;
		else
			self->monsterinfo.currentmove = &tank_move_pain_short2;
	}
}

//
// death
//

void mytank_dead(edict_t* self)
{
	VectorSet(self->mins, -16, -16, -16);
	VectorSet(self->maxs, 16, 16, -0);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	//self->nextthink = 0;
	gi.linkentity(self);
	M_PrepBodyRemoval(self);
}

static void mytank_shrink(edict_t *self)
{
	self->maxs[2] = 0;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}

mframe_t mytank_frames_death1[] =
{
	ai_move, -7,  NULL,
	ai_move, -2,  NULL,
	ai_move, -2,  NULL,
	ai_move, 1,   NULL,
	ai_move, 3,   NULL,
	ai_move, 6,   NULL,
	ai_move, 1,   NULL,
	ai_move, 1,   NULL,
	ai_move, 2,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, -2,  NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, -3,  NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, -4,  NULL,
	ai_move, -6,  NULL,
	ai_move, -4,  NULL,
	ai_move, -5,  NULL,
	ai_move, -7,  mytank_shrink,
	ai_move, -15, mytank_thud,
	ai_move, -5,  NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL,
	ai_move, 0,   NULL
};
mmove_t	mytank_move_death = { FRAME_death101, FRAME_death132, mytank_frames_death1, mytank_dead };

void mytank_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	//gi.dprintf("mytank_die called at %.1f\n", level.time);//DEBUG
	M_Notify(self);
	mytank_remove_reinforcements(self);

#ifdef OLD_NOLAG_STYLE
	// reduce lag by removing the entity right away
	if (nolag->value)
	{
		M_Remove(self, false, true);
		return;
	}
#endif

	// check for gibbed body
	if (self->health <= self->gib_health)
	{
		gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
		vrx_throw_drone_gibs(self, damage);
		//self->deadflag = DEAD_DEAD;
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

	vrx_drop_tank_death_arm(self, damage);

	DroneList_Remove(self);

	// begin death sequence
	gi.sound (self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	vrx_update_drone_death_skin(self);
	self->monsterinfo.currentmove = &mytank_move_death;
	
	if (self->activator && !self->activator->client)
	{
		self->activator->num_monsters_real--;
		// gi.bprintf(PRINT_HIGH, "releasing %p (%d)\n", self, self->activator->num_monsters_real);
	}
}

//
// monster_tank
//

/*QUAKED monster_tank (1 .5 0) (-32 -32 -16) (32 32 72) Ambush Trigger_Spawn Sight
*/
/*QUAKED monster_mytank_commander (1 .5 0) (-32 -32 -16) (32 32 72) Ambush Trigger_Spawn Sight
*/
void init_drone_tank (edict_t *self)
{
//	if (deathmatch->value)
//	{
//		G_FreeEdict (self);
//		return;
//	}

	self->s.modelindex = gi.modelindex ("models/monsters/tank/tris.md2");
	VectorSet (self->mins, -24, -24, -16);
	VectorSet (self->maxs, 24, 24, 64);
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	sound_pain = gi.soundindex ("tank/tnkpain2.wav");
	sound_pain2 = gi.soundindex ("tank/pain.wav");
	sound_thud = gi.soundindex ("tank/tnkdeth2.wav");
	sound_idle = gi.soundindex ("tank/tnkidle1.wav");
	sound_die = gi.soundindex ("tank/death.wav");
	sound_step = gi.soundindex ("tank/step.wav");
	sound_windup = gi.soundindex ("tank/tnkatck4.wav");
	sound_strike = gi.soundindex ("tank/tnkatck5.wav");
	sound_grenade = gi.soundindex ("guncmdr/gcdratck3.wav");
	sound_sight = gi.soundindex ("tank/sight1.wav");
	sound_spawn = gi.soundindex("medic_commander/monsterspawn1.wav");

	gi.soundindex ("tank/tnkatck1.wav");
	gi.soundindex ("tank/tnkatk2a.wav");
	gi.soundindex ("tank/tnkatk2b.wav");
	gi.soundindex ("tank/tnkatk2c.wav");
	gi.soundindex ("tank/tnkatk2d.wav");
	gi.soundindex ("tank/tnkatk2e.wav");
	gi.soundindex ("tank/tnkatck3.wav");
	gi.modelindex("models/items/spawngro3/tris.md2");

//	if (self->activator && self->activator->client)
	self->health = M_TANK_INITIAL_HEALTH + M_TANK_ADDON_HEALTH*self->monsterinfo.level; // hlt: tank
	//else self->health = 100 + 65*self->monsterinfo.level;

	self->max_health = self->health;
	self->gib_health = -2 * BASE_GIB_HEALTH;

	//if (self->activator && self->activator->client)
	M_SetMonsterArmor(self, M_TANK_INITIAL_ARMOR + M_TANK_ADDON_ARMOR*self->monsterinfo.level); // pow: tank
	//else M_SetMonsterArmor(self, 200 + 105*self->monsterinfo.level);

	self->monsterinfo.control_cost = M_TANK_CONTROL_COST;
	self->monsterinfo.cost = M_TANK_COST;
	self->mtype = M_TANK;
	
	if (random() > 0.5)
		self->item = FindItemByClassname("ammo_bullets");
	else
		self->item = FindItemByClassname("ammo_rockets");

	self->mass = 500;

	self->monsterinfo.pain_chance = 0.15f;
	self->pain = tank_pain;
	self->die = mytank_die;
	//self->touch = mytank_touch;
	self->monsterinfo.stand = mytank_stand;
	self->monsterinfo.walk = tank_walk;
	self->monsterinfo.run = mytank_run;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = mytank_attack;
	self->monsterinfo.melee = mytank_melee;
	self->monsterinfo.sight = mytank_sight;
	self->monsterinfo.idle = mytank_idle;
	self->monsterinfo.jumpup = 64;
	self->monsterinfo.jumpdn = 512;
	self->monsterinfo.aiflags |= AI_NO_CIRCLE_STRAFE;
	//self->monsterinfo.melee = 1;

	gi.linkentity (self);
	
	self->monsterinfo.currentmove = &mytank_move_stand;
	self->monsterinfo.scale = MODEL_SCALE;

//	walkmonster_start(self);
	self->nextthink = level.time + FRAMETIME;

//	self->activator->num_monsters += self->monsterinfo.control_cost;
}

void init_drone_tank_n64(edict_t *self)
{
	init_drone_tank(self);

	self->mtype = M_TANK_N64;
	self->s.skinnum = 2;
	self->s.scale = TANK_N64_SCALE;
	VectorSet(self->mins, -32.0f * TANK_N64_SCALE, -32.0f * TANK_N64_SCALE, -16.0f * TANK_N64_SCALE);
	VectorSet(self->maxs, 32.0f * TANK_N64_SCALE, 32.0f * TANK_N64_SCALE, 64.0f * TANK_N64_SCALE);

	self->health = M_TANK_N64_INITIAL_HEALTH + M_TANK_N64_ADDON_HEALTH * self->monsterinfo.level;
	self->max_health = self->health;
	M_SetMonsterArmor(self, M_TANK_N64_INITIAL_ARMOR + M_TANK_N64_ADDON_ARMOR * self->monsterinfo.level);
	self->monsterinfo.scale = MODEL_SCALE * TANK_N64_SCALE;
	self->accel = TANK_N64_HEAT_TURN_FRACTION;
	self->count = random() < 0.5f;
	self->mass = 550;

	gi.linkentity(self);
}

void init_drone_commander (edict_t *self)
{
	init_drone_tank(self);

	// modify health and armor
	if (invasion->value < 2)
		self->health = 2500 + 675*self->monsterinfo.level; // hlt: commander_normal_and_invasion
	else
		self->health = 5000 + 750*self->monsterinfo.level; // hlt: commander_invasion_hard

	self->max_health = self->health;
	M_SetMonsterArmor(self, 675*self->monsterinfo.level); // pow: commander_normal_and_invasion,commander_invasion_hard

	self->monsterinfo.control_cost = M_COMMANDER_CONTROL_COST;
	self->monsterinfo.cost = M_COMMANDER_COST;
	self->mtype = M_COMMANDER;
	self->s.skinnum = 2;

	if (!invasion->value)
		G_PrintGreenText(va("A level %d tank commander has spawned!", self->monsterinfo.level));
}


