/*
==============================================================================

carrier

==============================================================================
*/

#include "g_local.h"
#include "../../quake2/monsterframes/m_rogue_carrier.h"

#define CARRIER_SUMMON_COUNT		4
#define CARRIER_SUMMON_COOLDOWN		6.0f
#define CARRIER_HEAT_TURN_FRACTION	0.085f
#define CARRIER_NO_SPAWN_Z			-99999.0f
#define CARRIER_NO_SPAWN_YAW		-99999.0f
#define CARRIER_YAW_SPEED			20.0f
#define CARRIER_SPAWN_YAW_SPEED		30.0f
#define CARRIER_AI_SPAWNING		0x00400000
#define CARRIER_RAIL_REFIRE_CHANCE	0.65f
#define CARRIER_RAIL_MAX_REFIRES	1
#define CARRIER_DEFAULT_SCALE		0.75f
#define CARRIER_INVASION_SCALE		0.50f // 0.60 was too big for spambox

static int sound_pain1;
static int sound_pain2;
static int sound_pain3;
static int sound_death;
static int sound_sight;
static int sound_rail;
static int sound_spawn;

void drone_ai_stand(edict_t *self, float dist);
void drone_ai_run(edict_t *self, float dist);
void drone_ai_walk(edict_t *self, float dist);
qboolean drone_findtarget(edict_t *self, qboolean force);

static void carrier_stand(edict_t *self);
static void carrier_walk(edict_t *self);
static void carrier_run(edict_t *self);
static void carrier_attack(edict_t *self);
static void carrier_attack_grenade(edict_t *self);
static void carrier_reattack_grenade(edict_t *self);
static void carrier_attack_rail(edict_t *self);
static void carrier_reattack_rail(edict_t *self);
static void carrier_turn_to_spawn_yaw(edict_t *self);

static void carrier_project_flash(edict_t *self, int flash, vec3_t forward, vec3_t start)
{
	vec3_t right, offset;

	AngleVectors(self->s.angles, forward, right, NULL);
	VectorCopy(monster_flash_offset[flash], offset);
	if (self->s.scale && self->s.scale != 1.0f)
		VectorScale(offset, self->s.scale, offset);
	G_ProjectSource(self->s.origin, offset, forward, right, start);
}

static void carrier_spawn_ai(edict_t *self, float dist)
{
	if (!self || !self->inuse)
		return;

	if (que_typeexists(self->curses, CURSE_FROZEN))
		return;

	VectorClear(self->velocity);
	VectorClear(self->avelocity);
	if (self->monsterinfo.aiflags & CARRIER_AI_SPAWNING)
		carrier_turn_to_spawn_yaw(self);
	(void)dist;
}

static void carrier_restore_spawn_steering(edict_t *self)
{
	if (self->monsterinfo.aiflags & CARRIER_AI_SPAWNING)
	{
		if (self->delay > 0.0f)
		{
			self->yaw_speed = self->delay;
			self->delay = 0.0f;
		}
		else
			self->yaw_speed = CARRIER_YAW_SPEED;
	}

	self->monsterinfo.aiflags &= ~(AI_HOLD_FRAME | CARRIER_AI_SPAWNING);
	VectorClear(self->avelocity);
}

static const int carrier_summons[CARRIER_SUMMON_COUNT] =
{
	M_DAEDALUS,
	M_FLYER,
	M_HOVER,
	M_FLOATER
};

static void carrier_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

static void carrier_explode(edict_t *self)
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

static void carrier_dead(edict_t *self)
{
	int n;

	for (n = 0; n < 4; n++)
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", 500, GIB_ORGANIC);
	for (n = 0; n < 6; n++)
		ThrowGib(self, "models/objects/gibs/sm_metal/tris.md2", 500, GIB_METALLIC);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1_BIG);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	M_Remove(self, false, false);
}

static void carrier_fire_heat(edict_t *self)
{
	int damage;
	int speed;
	vec3_t forward, start;
	const int flashes[4] =
	{
		MZ2_CARRIER_ROCKET_1,
		MZ2_CARRIER_ROCKET_2,
		MZ2_CARRIER_ROCKET_3,
		MZ2_CARRIER_ROCKET_4
	};

	if (!G_EntExists(self->enemy))
		return;

	damage = M_ROCKETLAUNCHER_DMG_BASE + M_ROCKETLAUNCHER_DMG_ADDON * drone_damagelevel(self);
	if (M_ROCKETLAUNCHER_DMG_MAX && damage > M_ROCKETLAUNCHER_DMG_MAX)
		damage = M_ROCKETLAUNCHER_DMG_MAX;
	speed = M_ROCKETLAUNCHER_SPEED_BASE + M_ROCKETLAUNCHER_SPEED_ADDON * drone_damagelevel(self);
	if (M_ROCKETLAUNCHER_SPEED_MAX && speed > M_ROCKETLAUNCHER_SPEED_MAX)
		speed = M_ROCKETLAUNCHER_SPEED_MAX;

	for (int i = 0; i < 4; i++)
	{
		carrier_project_flash(self, flashes[i], forward, start);
		MonsterAim(self, M_PROJECTILE_ACC, speed, true, flashes[i], forward, start);
		monster_fire_heat(self, start, forward, damage, speed, flashes[i], CARRIER_HEAT_TURN_FRACTION);
	}
}

static void carrier_fire_bullets(edict_t *self)
{
	int damage;
	vec3_t forward, start;
	const int flashes[2][2] =
	{
		{ MZ2_CARRIER_MACHINEGUN_L1, MZ2_CARRIER_MACHINEGUN_R1 },
		{ MZ2_CARRIER_MACHINEGUN_L2, MZ2_CARRIER_MACHINEGUN_R2 }
	};
	const int flash_row = self->monsterinfo.lefty ? 1 : 0;

	if (!G_EntExists(self->enemy))
		return;

	damage = M_MACHINEGUN_DMG_BASE + M_MACHINEGUN_DMG_ADDON * drone_damagelevel(self);
	if (M_MACHINEGUN_DMG_MAX && damage > M_MACHINEGUN_DMG_MAX)
		damage = M_MACHINEGUN_DMG_MAX;

	for (int i = 0; i < 2; i++)
	{
		carrier_project_flash(self, flashes[flash_row][i], forward, start);
		MonsterAim(self, M_PROJECTILE_ACC, 0, false, -1, forward, start);
		monster_fire_bullet(self, start, forward, damage, damage, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, flashes[flash_row][i]);
	}

	self->monsterinfo.lefty = 1 - self->monsterinfo.lefty;
}

static void carrier_fire_grenade(edict_t *self)
{
	int damage;
	int speed;
	vec3_t forward, right, up, start, target, aim, offset;
	float direction;
	float spread_r, spread_u;
	int mytime;

	if (!G_EntExists(self->enemy))
		return;

	damage = M_GRENADELAUNCHER_DMG_BASE + M_GRENADELAUNCHER_DMG_ADDON * drone_damagelevel(self);
	if (M_GRENADELAUNCHER_DMG_MAX && damage > M_GRENADELAUNCHER_DMG_MAX)
		damage = M_GRENADELAUNCHER_DMG_MAX;
	speed = M_GRENADELAUNCHER_SPEED_BASE + M_GRENADELAUNCHER_SPEED_ADDON * drone_damagelevel(self);
	if (M_GRENADELAUNCHER_SPEED_MAX && speed > M_GRENADELAUNCHER_SPEED_MAX)
		speed = M_GRENADELAUNCHER_SPEED_MAX;

	AngleVectors(self->s.angles, forward, right, up);
	VectorCopy(monster_flash_offset[MZ2_CARRIER_GRENADE], offset);
	if (self->s.scale && self->s.scale != 1.0f)
		VectorScale(offset, self->s.scale, offset);
	G_ProjectSource(self->s.origin, offset, forward, right, start);

	VectorCopy(self->enemy->s.origin, target);
	target[2] += self->enemy->viewheight;
	VectorSubtract(target, start, aim);
	VectorNormalize(aim);

	direction = (random() < 0.5f) ? -1.0f : 1.0f;
	mytime = (int)((level.time - self->timestamp) / 0.4f);
	switch (mytime)
	{
	case 0:
		spread_r = 0.15f * direction;
		spread_u = 0.1f - 0.1f * direction;
		break;
	case 1:
		spread_r = 0.0f;
		spread_u = 0.1f;
		break;
	case 2:
		spread_r = -0.15f * direction;
		spread_u = 0.1f + 0.1f * direction;
		break;
	case 3:
		spread_r = 0.0f;
		spread_u = 0.1f;
		break;
	default:
		spread_r = 0.0f;
		spread_u = 0.0f;
		break;
	}

	VectorMA(aim, spread_r, right, aim);
	VectorMA(aim, spread_u, up, aim);
	VectorNormalize(aim);
	if (aim[2] > 0.15f)
		aim[2] = 0.15f;
	else if (aim[2] < -0.5f)
		aim[2] = -0.5f;

	monster_fire_grenade(self, start, aim, damage, speed, MZ2_CARRIER_GRENADE);
}

static void carrier_fire_rail(edict_t *self)
{
	int damage;
	vec3_t forward, start;

	if (!G_EntExists(self->enemy))
		return;

	damage = M_RAILGUN_DMG_BASE + M_RAILGUN_DMG_ADDON * drone_damagelevel(self);
	if (M_RAILGUN_DMG_MAX && damage > M_RAILGUN_DMG_MAX)
		damage = M_RAILGUN_DMG_MAX;

	carrier_project_flash(self, MZ2_CARRIER_RAILGUN, forward, start);
	if (self->pos2[2] > CARRIER_NO_SPAWN_Z + 1.0f)
	{
		VectorSubtract(self->pos2, start, forward);
		VectorNormalize(forward);
	}
	else
		MonsterAim(self, 0.25f, 0, false, MZ2_CARRIER_RAILGUN, forward, start);
	gi.sound(self, CHAN_WEAPON, sound_rail, 1, ATTN_NORM, 0);
	monster_fire_railgun(self, start, forward, damage, damage, MZ2_CARRIER_RAILGUN);
}

static void carrier_save_rail_target(edict_t *self)
{
	if (!G_EntExists(self->enemy))
		return;

	VectorCopy(self->enemy->s.origin, self->pos2);
	self->pos2[2] += self->enemy->viewheight;
}

static qboolean carrier_valid_spawn_spot(edict_t *self, vec3_t mins, vec3_t maxs, vec3_t spot)
{
	if (G_IsValidLocation(self, spot, mins, maxs))
		return true;

	spot[2] += 32;
	if (G_IsValidLocation(self, spot, mins, maxs))
		return true;

	spot[2] -= 64;
	return G_IsValidLocation(self, spot, mins, maxs);
}

static void carrier_spawn_basis(edict_t *self, vec3_t forward, vec3_t right)
{
	vec3_t angles;

	VectorCopy(self->s.angles, angles);
	if ((self->monsterinfo.aiflags & CARRIER_AI_SPAWNING)
		&& self->angle > CARRIER_NO_SPAWN_YAW + 1.0f)
		angles[YAW] = self->angle;

	AngleVectors(angles, forward, right, NULL);
}

static int carrier_spawn_type(int index)
{
	return carrier_summons[index % CARRIER_SUMMON_COUNT];
}

static void carrier_spawn_bounds(int mtype, vec3_t mins, vec3_t maxs)
{
	switch (mtype)
	{
	case M_FLYER:
		VectorSet(mins, -16, -16, -24);
		VectorSet(maxs, 16, 16, 8);
		break;
	case M_FLOATER:
		VectorSet(mins, -24, -24, -24);
		VectorSet(maxs, 24, 24, 40);
		break;
	case M_DAEDALUS:
	case M_HOVER:
	default:
		VectorSet(mins, -24, -24, -24);
		VectorSet(maxs, 24, 24, 32);
		break;
	}
}

static qboolean carrier_find_spawn_spot(edict_t *self, vec3_t mins, vec3_t maxs, int index, vec3_t spot)
{
	vec3_t forward, right;
	static const vec3_t local_offsets[8] =
	{
		{ 105.0f,   0.0f, -58.0f },
		{ 150.0f,   0.0f, -58.0f },
		{ 200.0f,   0.0f, -58.0f },
		{ 140.0f,  72.0f, -58.0f },
		{ 140.0f, -72.0f, -58.0f },
		{ 190.0f,  96.0f, -48.0f },
		{ 190.0f, -96.0f, -48.0f },
		{ 220.0f,   0.0f, -32.0f }
	};
	static const float ring_dirs[8][2] =
	{
		{  1.0f,   0.0f },
		{  0.707f, 0.707f },
		{  0.0f,   1.0f },
		{ -0.707f, 0.707f },
		{ -1.0f,   0.0f },
		{ -0.707f,-0.707f },
		{  0.0f,  -1.0f },
		{  0.707f,-0.707f }
	};
	static const float radii[3] = { 160.0f, 240.0f, 320.0f };
	static const float z_offsets[3] = { -58.0f, -32.0f, 8.0f };

	carrier_spawn_basis(self, forward, right);

	for (int i = 0; i < 8; i++)
	{
		const int offset_index = i;

		VectorCopy(self->s.origin, spot);
		VectorMA(spot, local_offsets[offset_index][0], forward, spot);
		VectorMA(spot, local_offsets[offset_index][1], right, spot);
		spot[2] += local_offsets[offset_index][2];
		if (carrier_valid_spawn_spot(self, mins, maxs, spot))
			return true;
	}

	for (int radius_index = 0; radius_index < 3; radius_index++)
	{
		for (int z_index = 0; z_index < 3; z_index++)
		{
			for (int dir_index = 0; dir_index < 8; dir_index++)
			{
				const int rotated_index = dir_index;

				VectorCopy(self->s.origin, spot);
				VectorMA(spot, radii[radius_index] * ring_dirs[rotated_index][0], forward, spot);
				VectorMA(spot, radii[radius_index] * ring_dirs[rotated_index][1], right, spot);
				spot[2] += z_offsets[z_index];
				if (carrier_valid_spawn_spot(self, mins, maxs, spot))
					return true;
			}
		}
	}

	return false;
}

static void carrier_cleanup_failed_spawn(edict_t *owner, edict_t *spawned)
{
	if (owner && owner->client)
		layout_remove_tracked_entity(&owner->client->layout, spawned);

	DroneList_Remove(spawned);
	AI_EnemyRemoved(spawned);
	G_FreeEdict(spawned);
}

static void carrier_setup_invasion_spawn(edict_t *spawned)
{
	if (!invasion->value)
		return;

	spawned->monsterinfo.aiflags &= ~AI_STAND_GROUND;
	spawned->monsterinfo.aiflags |= AI_FIND_NAVI;
	spawned->prev_navi = NULL;
	spawned->goalentity = NULL;
}

static void carrier_start_spawned_monster(edict_t *self, edict_t *spawned)
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

static qboolean carrier_spawn_monster(edict_t *self, int index)
{
	edict_t *owner;
	edict_t *spawned;
	vec3_t saved_spot;
	vec3_t spot;

	owner = (self->activator && self->activator->inuse) ? self->activator : self;
	spawned = G_Spawn();
	spawned->mtype = carrier_spawn_type(index);
	spawned->activator = owner;
	spawned->monsterinfo.level = self->monsterinfo.level;

	if (!M_Initialize(owner, spawned, 0.0f))
	{
		G_FreeEdict(spawned);
		return false;
	}

	VectorCopy(self->pos1, saved_spot);
	if (saved_spot[2] > CARRIER_NO_SPAWN_Z + 1.0f && G_IsValidLocation(self, saved_spot, spawned->mins, spawned->maxs))
		VectorCopy(saved_spot, spot);
	else if (!carrier_find_spawn_spot(self, spawned->mins, spawned->maxs, index, spot))
	{
		carrier_cleanup_failed_spawn(owner, spawned);
		return false;
	}

	spawned->monsterinfo.cost = 0;
	spawned->s.effects |= EF_PLASMA;
	VectorCopy(spot, spawned->s.origin);
	VectorCopy(spot, spawned->s.old_origin);
	VectorCopy(self->s.angles, spawned->s.angles);
	spawned->nextthink = level.time + FRAMETIME;
	spawned->monsterinfo.attack_finished = level.time + 1.0;
	carrier_setup_invasion_spawn(spawned);

	gi.linkentity(spawned);
	owner->num_monsters += spawned->monsterinfo.control_cost;
	owner->num_monsters_real++;

	if (sound_spawn)
		gi.sound(self, CHAN_BODY, sound_spawn, 1, ATTN_NORM, 0);
	carrier_start_spawned_monster(self, spawned);

	return true;
}

static qboolean carrier_spawngrow(edict_t *self, int index)
{
	vec3_t mins, maxs, size, spot;
	float radius;

	carrier_spawn_bounds(carrier_spawn_type(index), mins, maxs);
	VectorSubtract(maxs, mins, size);
	radius = VectorLength(size) * 0.5f;

	VectorSet(self->pos1, 0, 0, CARRIER_NO_SPAWN_Z);
	if (!carrier_find_spawn_spot(self, mins, maxs, index, spot))
		return false;

	VectorCopy(spot, self->pos1);
	SpawnGrow_Spawn(spot, radius, radius * 2.0f);
	return true;
}

static float carrier_yaw_delta(float a, float b)
{
	float delta = anglemod(a - b);

	if (delta > 180.0f)
		delta = 360.0f - delta;
	return delta;
}

static qboolean carrier_set_spawn_base_yaw(edict_t *self)
{
	vec3_t to_enemy;

	if (!G_EntExists(self->enemy))
		return false;

	VectorSubtract(self->enemy->s.origin, self->s.origin, to_enemy);
	self->move_angles[YAW] = vectoyaw(to_enemy);
	return true;
}

static void carrier_set_spawn_yaw(edict_t *self)
{
	static const float yaw_offsets[CARRIER_SUMMON_COUNT] = { -30.0f, 0.0f, 30.0f, 0.0f };

	self->angle = anglemod(self->move_angles[YAW] + yaw_offsets[self->count % CARRIER_SUMMON_COUNT]);
	self->ideal_yaw = self->angle;
	self->teleport_time = level.time + 1.0f;
}

static void carrier_turn_to_spawn_yaw(edict_t *self)
{
	if (self->angle <= CARRIER_NO_SPAWN_YAW + 1.0f)
		return;

	VectorClear(self->velocity);
	VectorClear(self->avelocity);
	self->ideal_yaw = self->angle;
	M_ChangeYaw(self);
}

static void carrier_prep_spawn(edict_t *self)
{
	self->count = 0;
	self->timestamp = level.time;
	self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	if (!(self->monsterinfo.aiflags & CARRIER_AI_SPAWNING))
		self->delay = self->yaw_speed > 0.0f ? self->yaw_speed : CARRIER_YAW_SPEED;
	self->monsterinfo.aiflags |= CARRIER_AI_SPAWNING;
	self->yaw_speed = CARRIER_SPAWN_YAW_SPEED;
	self->angle = CARRIER_NO_SPAWN_YAW;
	VectorClear(self->velocity);
	VectorClear(self->avelocity);
	VectorSet(self->pos1, 0, 0, CARRIER_NO_SPAWN_Z);
	if (carrier_set_spawn_base_yaw(self))
		carrier_set_spawn_yaw(self);
}

static void carrier_start_spawn(edict_t *self)
{
	if (self->angle <= CARRIER_NO_SPAWN_YAW + 1.0f
		&& carrier_set_spawn_base_yaw(self))
		carrier_set_spawn_yaw(self);
}

static void carrier_ready_spawn(edict_t *self)
{
	if (self->angle <= CARRIER_NO_SPAWN_YAW + 1.0f)
	{
		if (!carrier_set_spawn_base_yaw(self))
		{
			self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
			return;
		}
		carrier_set_spawn_yaw(self);
	}

	if (carrier_yaw_delta(self->s.angles[YAW], self->angle) > 0.5f
		&& level.time < self->teleport_time)
	{
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
		return;
	}

	self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	carrier_spawngrow(self, self->count);
}

static void carrier_spawn_check(edict_t *self)
{
	self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	if (self->count < CARRIER_SUMMON_COUNT)
	{
		carrier_spawn_monster(self, self->count);
		self->count++;
	}

	if (self->count < CARRIER_SUMMON_COUNT)
	{
		self->angle = CARRIER_NO_SPAWN_YAW;
		VectorSet(self->pos1, 0, 0, CARRIER_NO_SPAWN_Z);
		carrier_set_spawn_yaw(self);
		self->monsterinfo.nextframe = FRAME_spawn08;
	}
	else
		self->monsterinfo.melee_finished = level.time + CARRIER_SUMMON_COOLDOWN;
}

static void carrier_done_spawn(edict_t *self)
{
	self->count = 0;
	carrier_restore_spawn_steering(self);
	self->angle = CARRIER_NO_SPAWN_YAW;
	VectorSet(self->pos1, 0, 0, CARRIER_NO_SPAWN_Z);
}

mframe_t carrier_frames_stand[] =
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
	drone_ai_stand, 0, NULL
};
mmove_t carrier_move_stand = { FRAME_search01, FRAME_search13, carrier_frames_stand, NULL };

static void carrier_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &carrier_move_stand;
}

mframe_t carrier_frames_run[] =
{
	drone_ai_run, 12, NULL,
	drone_ai_run, 14, NULL,
	drone_ai_run, 16, NULL,
	drone_ai_run, 18, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 20, NULL,
	drone_ai_run, 18, NULL,
	drone_ai_run, 16, NULL,
	drone_ai_run, 14, NULL,
	drone_ai_run, 12, NULL,
	drone_ai_run, 10, NULL,
	drone_ai_run, 10, NULL,
	drone_ai_run, 10, NULL
};
mmove_t carrier_move_run = { FRAME_search01, FRAME_search13, carrier_frames_run, NULL };

static void carrier_run(edict_t *self)
{
	carrier_restore_spawn_steering(self);

	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &carrier_move_stand;
	else
		self->monsterinfo.currentmove = &carrier_move_run;
}

static void carrier_walk(edict_t *self)
{
	if (!self->goalentity)
		self->goalentity = world;
	self->monsterinfo.currentmove = &carrier_move_run;
}

mframe_t carrier_frames_attack_mg[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, carrier_fire_bullets,
	ai_charge, 0, carrier_fire_bullets,
	ai_charge, 0, carrier_fire_bullets,
	ai_charge, 0, NULL
};
mmove_t carrier_move_attack_mg = { FRAME_firea06, FRAME_firea11, carrier_frames_attack_mg, carrier_run };

mframe_t carrier_frames_attack_heat[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, carrier_fire_heat,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t carrier_move_attack_heat = { FRAME_fireb01, FRAME_fireb04, carrier_frames_attack_heat, carrier_run };

mframe_t carrier_frames_attack_pre_grenade[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, carrier_attack_grenade
};
mmove_t carrier_move_attack_pre_grenade = { FRAME_fireb01, FRAME_fireb06, carrier_frames_attack_pre_grenade, NULL };

mframe_t carrier_frames_attack_grenade[] =
{
	ai_charge, -15, carrier_fire_grenade,
	ai_charge, 4, NULL,
	ai_charge, 4, NULL,
	ai_charge, 4, carrier_reattack_grenade
};
mmove_t carrier_move_attack_grenade = { FRAME_fireb07, FRAME_fireb10, carrier_frames_attack_grenade, NULL };

mframe_t carrier_frames_attack_post_grenade[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t carrier_move_attack_post_grenade = { FRAME_fireb11, FRAME_fireb16, carrier_frames_attack_post_grenade, carrier_run };

mframe_t carrier_frames_attack_rail[] =
{
	ai_charge, 2, NULL,
	ai_charge, 2, carrier_save_rail_target,
	ai_charge, 2, NULL,
	ai_charge, -20, carrier_fire_rail,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, NULL,
	ai_charge, 2, carrier_reattack_rail
};
mmove_t carrier_move_attack_rail = { FRAME_search01, FRAME_search09, carrier_frames_attack_rail, carrier_run };

mframe_t carrier_frames_spawn[] =
{
	carrier_spawn_ai, -2, NULL,
	carrier_spawn_ai, -2, NULL,
	carrier_spawn_ai, -2, NULL,
	carrier_spawn_ai, -2, NULL,
	carrier_spawn_ai, -2, NULL,
	carrier_spawn_ai, -2, NULL,
	carrier_spawn_ai, -2, carrier_prep_spawn,
	carrier_spawn_ai, -2, carrier_start_spawn,
	carrier_spawn_ai, -2, carrier_ready_spawn,
	carrier_spawn_ai, -2, NULL,
	carrier_spawn_ai, -2, NULL,
	carrier_spawn_ai, -10, carrier_spawn_check,
	carrier_spawn_ai, -2, NULL,
	carrier_spawn_ai, -2, NULL,
	carrier_spawn_ai, -2, NULL,
	carrier_spawn_ai, -2, NULL,
	carrier_spawn_ai, -2, NULL,
	carrier_spawn_ai, -2, carrier_done_spawn
};
mmove_t carrier_move_spawn = { FRAME_spawn01, FRAME_spawn18, carrier_frames_spawn, carrier_run };

static void carrier_attack_grenade(edict_t *self)
{
	self->timestamp = level.time;
	self->monsterinfo.currentmove = &carrier_move_attack_grenade;
}

static void carrier_reattack_grenade(edict_t *self)
{
	if (G_EntExists(self->enemy) && infront(self, self->enemy)
		&& self->timestamp + 1.3f > level.time)
	{
		self->monsterinfo.currentmove = &carrier_move_attack_grenade;
		return;
	}

	self->monsterinfo.currentmove = &carrier_move_attack_post_grenade;
}

static void carrier_begin_spawn(edict_t *self)
{
	self->monsterinfo.currentmove = &carrier_move_spawn;
}

static void carrier_attack_rail(edict_t *self)
{
	self->count = 0;
	self->timestamp = level.time;
	VectorSet(self->pos2, 0, 0, CARRIER_NO_SPAWN_Z);
	self->monsterinfo.currentmove = &carrier_move_attack_rail;
}

static void carrier_reattack_rail(edict_t *self)
{
	if (self->count < CARRIER_RAIL_MAX_REFIRES
		&& G_ValidTarget(self, self->enemy, true, true)
		&& infront(self, self->enemy)
		&& random() < CARRIER_RAIL_REFIRE_CHANCE)
	{
		self->count++;
		VectorSet(self->pos2, 0, 0, CARRIER_NO_SPAWN_Z);
		self->monsterinfo.currentmove = &carrier_move_attack_rail;
		self->monsterinfo.nextframe = FRAME_search01;
		M_DelayNextAttack(self, 0.0f, true);
		return;
	}

	self->count = 0;
	self->monsterinfo.attack_finished = level.time + 1.0f;
}

static void carrier_attack(edict_t *self)
{
	float r;

	if (!G_EntExists(self->enemy))
		return;

	r = random();
	if (level.time >= self->monsterinfo.melee_finished && r < 0.25f)
		carrier_begin_spawn(self);
	else if (r < 0.50f)
		self->monsterinfo.currentmove = &carrier_move_attack_heat;
	else if (r < 0.80f)
		carrier_attack_rail(self);
	else if (r < 0.92f)
		self->monsterinfo.currentmove = &carrier_move_attack_pre_grenade;
	else
		self->monsterinfo.currentmove = &carrier_move_attack_mg;

	M_DelayNextAttack(self, 1.0f + random(), true);
}

mframe_t carrier_frames_pain[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t carrier_move_pain = { FRAME_spawn01, FRAME_spawn04, carrier_frames_pain, carrier_run };

static void carrier_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	carrier_restore_spawn_steering(self);

	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3.0;
	if (random() < 0.33f)
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else if (random() < 0.5f)
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain3, 1, ATTN_NORM, 0);

	if (skill->value != 3)
		self->monsterinfo.currentmove = &carrier_move_pain;
}

mframe_t carrier_frames_death[] =
{
	ai_move, 0, carrier_explode,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, carrier_explode,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, carrier_explode,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, carrier_explode,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, carrier_explode
};
mmove_t carrier_move_death = { FRAME_death01, FRAME_death16, carrier_frames_death, carrier_dead };

static void carrier_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	M_Notify(self);
	carrier_restore_spawn_steering(self);

	if (self->deadflag == DEAD_DEAD)
		return;

	DroneList_Remove(self);
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	vrx_update_drone_death_skin(self);
	self->monsterinfo.currentmove = &carrier_move_death;
}

void init_drone_carrier(edict_t *self)
{
	sound_pain1 = gi.soundindex("carrier/pain_md.wav");
	sound_pain2 = gi.soundindex("carrier/pain_lg.wav");
	sound_pain3 = gi.soundindex("carrier/pain_sm.wav");
	sound_death = gi.soundindex("carrier/death.wav");
	sound_sight = gi.soundindex("carrier/sight.wav");
	sound_rail = gi.soundindex("gladiator/railgun.wav");
	sound_spawn = gi.soundindex("medic_commander/monsterspawn1.wav");
	self->s.sound = gi.soundindex("bosshovr/bhvengn1.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/carrier/tris.md2");
	if (invasion->value)
	{
		self->s.scale = CARRIER_INVASION_SCALE;
		VectorSet(self->mins, -34, -34, -20);
		VectorSet(self->maxs, 34, 34, 68);
	}
	else
	{
		self->s.scale = CARRIER_DEFAULT_SCALE;
		VectorSet(self->mins, -56, -56, -44);
		VectorSet(self->maxs, 56, 56, 44);
	}

	gi.modelindex("models/items/spawngro3/tris.md2");
	gi.modelindex("models/monsters/flyer/tris.md2");
	gi.modelindex("models/monsters/float/tris.md2");
	gi.modelindex("models/monsters/hover/tris.md2");

	self->health = M_CARRIER_INITIAL_HEALTH + M_CARRIER_ADDON_HEALTH * self->monsterinfo.level;
	self->max_health = self->health;
	self->gib_health = -2000;
	self->mass = 1000;
	self->mtype = M_CARRIER;
	self->flags |= FL_FLY;
	self->yaw_speed = CARRIER_YAW_SPEED;

	self->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;
	self->monsterinfo.power_armor_power = M_CARRIER_INITIAL_ARMOR + M_CARRIER_ADDON_ARMOR * self->monsterinfo.level;
	self->monsterinfo.max_armor = self->monsterinfo.power_armor_power;
	self->monsterinfo.control_cost = M_JORG_CONTROL_COST;
	self->monsterinfo.cost = M_COMMANDER_COST;
	self->monsterinfo.jumpup = 64;
	self->monsterinfo.jumpdn = 512;
	self->monsterinfo.aiflags |= AI_NO_CIRCLE_STRAFE;

	self->pain = carrier_pain;
	self->die = carrier_die;
	self->monsterinfo.stand = carrier_stand;
	self->monsterinfo.walk = carrier_walk;
	self->monsterinfo.run = carrier_run;
	self->monsterinfo.attack = carrier_attack;
	self->monsterinfo.sight = carrier_sight;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &carrier_move_stand;
	self->monsterinfo.scale = MODEL_SCALE;
	self->nextthink = level.time + FRAMETIME;

	if (!invasion->value)
		G_PrintGreenText(va("A level %d carrier has spawned!", self->monsterinfo.level));
}
