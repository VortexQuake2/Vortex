/*
==============================================================================

ARACHNID

==============================================================================
*/

#include "g_local.h"
#include "../../quake2/monsterframes/m_arachnid.h"

static int sound_pain;
static int sound_death;
static int sound_sight;
static int sound_step;
static int sound_charge;
static int sound_heat_fire;
static int sound_melee;
static int sound_melee_hit;
static int sound_monster_spawn;	// medic-commander spawn sound
//static int sound_angry;			// arachnid/angry.wav enrage cry (PSX asset only)


static constexpr float ARACHNID_DEFAULT_SCALE = 0.75f;
static constexpr float ARACHNID_INVASION_SCALE = 0.60f;
static constexpr float ARACHNID_HEAT_DEFAULT_SCALE = 0.85f;
static constexpr float ARACHNID_HEAT_INVASION_SCALE = 0.68f;
static constexpr float ARACHNID_ATTACK_RECOVERY_TIME = 0.5f;
static constexpr float ARACHNID_HEAT_TURN_FRACTION = 0.095f;
static constexpr float ARACHNID_DODGE_SIDE_SPEED = 280.0f;
static constexpr float ARACHNID_DODGE_UP_SPEED = 250.0f;
static constexpr float ARACHNID_DODGE_SIDE_PROBE = 64.0f;
static constexpr float ARACHNID_DODGE_COOLDOWN = 1.5f;
static constexpr float ARACHNID_DODGE_TIMEOUT = 1.2f;

// Ceiling walking (fire_plasma arachnid only; adapted from the stalker, disabled in invasion)
static constexpr int ARACHNID_CEILING_NONE = 0;
static constexpr int ARACHNID_CEILING_ON = 1;
static constexpr int ARACHNID_CEILING_JUMPING = 2;
static constexpr int ARACHNID_CEILING_TRACE_DIST = 256;
static constexpr int ARACHNID_CEILING_JUMP_SPEED = 600;
static constexpr int ARACHNID_CEILING_MIN_SPEED = 400;

static void arachnid_stand(edict_t *self);
static void arachnid_run(edict_t *self);
static qboolean arachnid_on_ceiling(edict_t *self); // defined below; used by arachnid_plasma above it
extern mmove_t arachnid_move_melee;
qboolean drone_findtarget(edict_t *self, qboolean force); // used by the rail variant's stalker summon

static void arachnid_footstep(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_step, 0.5, ATTN_IDLE, 0);
}

static void arachnid_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

mframe_t arachnid_frames_stand[] =
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
mmove_t arachnid_move_stand = { FRAME_idle1, FRAME_idle13, arachnid_frames_stand, NULL };

static void arachnid_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &arachnid_move_stand;
}

mframe_t arachnid_frames_walk[] =
{
	drone_ai_walk, 8, arachnid_footstep,
	drone_ai_walk, 8, NULL,
	drone_ai_walk, 8, NULL,
	drone_ai_walk, 8, NULL,
	drone_ai_walk, 8, NULL,
	drone_ai_walk, 8, arachnid_footstep,
	drone_ai_walk, 8, NULL,
	drone_ai_walk, 8, NULL,
	drone_ai_walk, 8, NULL,
	drone_ai_walk, 8, NULL
};
mmove_t arachnid_move_walk = { FRAME_walk1, FRAME_walk10, arachnid_frames_walk, NULL };

static void arachnid_walk(edict_t *self)
{
	if (!self->goalentity)
		self->goalentity = world;
	self->monsterinfo.currentmove = &arachnid_move_walk;
}

mframe_t arachnid_frames_run[] =
{
	drone_ai_run, 13, arachnid_footstep,
	drone_ai_run, 13, NULL,
	drone_ai_run, 13, NULL,
	drone_ai_run, 13, NULL,
	drone_ai_run, 13, NULL,
	drone_ai_run, 13, arachnid_footstep,
	drone_ai_run, 13, NULL,
	drone_ai_run, 13, NULL,
	drone_ai_run, 13, NULL,
	drone_ai_run, 13, NULL
};
mmove_t arachnid_move_run = { FRAME_walk1, FRAME_walk10, arachnid_frames_run, NULL };

static void arachnid_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &arachnid_move_stand;
	else
		self->monsterinfo.currentmove = &arachnid_move_run;
}

static void arachnid_finish_ranged_attack(edict_t *self)
{
	M_DelayNextAttack(self, ARACHNID_ATTACK_RECOVERY_TIME, false);
	arachnid_run(self);
}

static void arachnid_charge_plasma(edict_t *self)
{
	if (!G_ValidTarget(self, self->enemy, true, true))
		return;

	//gi.sound(self, CHAN_WEAPON, sound_charge, 1, ATTN_NORM, 0);
}

static int arachnid_plasma_flash(edict_t *self)
{
	switch (self->s.frame)
	{
	case FRAME_rails7:
		return MZ2_ARACHNID_RAIL2;
	case FRAME_rails_up2:
	case FRAME_rails_up9:
		return MZ2_ARACHNID_RAIL_UP1;
	case FRAME_rails_up5:
	case FRAME_rails_up11:
		return MZ2_ARACHNID_RAIL_UP2;
	case FRAME_rails3:
	default:
		return MZ2_ARACHNID_RAIL1;
	}
}

static void arachnid_plasma(edict_t *self)
{
	int damage, speed, flash_number;
	vec3_t start, forward;

	if (!G_ValidTarget(self, self->enemy, true, true))
		return;

	damage = M_PLASMA_DMG_BASE + M_PLASMA_DMG_ADDON * drone_damagelevel(self);
	if (M_PLASMA_DMG_MAX && damage > M_PLASMA_DMG_MAX)
		damage = M_PLASMA_DMG_MAX;
	speed = M_PLASMA_SPEED_BASE + M_PLASMA_SPEED_ADDON * drone_damagelevel(self);
	if (M_PLASMA_SPEED_MAX && speed > M_PLASMA_SPEED_MAX)
		speed = M_PLASMA_SPEED_MAX;

	flash_number = arachnid_plasma_flash(self);
	if (arachnid_on_ceiling(self))
	{
		// inverted on the ceiling (ROLL=180): build the muzzle ourselves with the
		// flash offset's Z mirrored (the hardcoded +Z in G_ProjectSource would push
		// the origin up into the ceiling), then pass -1 so MonsterAim skips its
		// upright-only absmin[2]+32 clamp - same idiom as daedalus_fire_grenade
		vec3_t right, offset;

		AngleVectors(self->s.angles, forward, right, NULL);
		VectorCopy(monster_flash_offset[flash_number], offset);
		offset[2] = -offset[2];
		G_ProjectSource(self->s.origin, offset, forward, right, start);
		MonsterAim(self, M_PROJECTILE_ACC, speed, false, -1, forward, start);
	}
	else
	{
		MonsterAim(self, M_PROJECTILE_ACC, speed, false, flash_number, forward, start);
	}
	if (!M_MonsterHasClearShotFrom(self, start))
		return;

	gi.sound(self, CHAN_WEAPON, sound_charge, 1, ATTN_NORM, 0);
	fire_plasma(self, start, forward, damage, speed, M_PLASMA_DAMAGE_RADIUS, damage);
}

mframe_t arachnid_frames_attack1[] =
{
	ai_charge, 0, arachnid_charge_plasma,
	ai_charge, 0, arachnid_plasma,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, arachnid_charge_plasma,
	ai_charge, 0, arachnid_plasma,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, arachnid_charge_plasma,
	ai_charge, 0, NULL
};
mmove_t arachnid_move_attack1 = { FRAME_rails2, FRAME_rails11, arachnid_frames_attack1, arachnid_finish_ranged_attack };

mframe_t arachnid_frames_attack_up1[] =
{
	ai_charge, 0, arachnid_charge_plasma,
	ai_charge, 0, arachnid_plasma,
	ai_charge, 0, NULL,
	ai_charge, 0, arachnid_charge_plasma,
	ai_charge, 0, arachnid_plasma,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, arachnid_charge_plasma,
	ai_charge, 0, arachnid_plasma,
	ai_charge, 0, arachnid_charge_plasma,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t arachnid_move_attack_up1 = { FRAME_rails_up1, FRAME_rails_up13, arachnid_frames_attack_up1, arachnid_finish_ranged_attack };

static int arachnid_heat_flash(edict_t *self)
{
	switch (self->s.frame)
	{
	case FRAME_rails8:
		return MZ2_ARACHNID_RAIL2;
	case FRAME_rails_up4:
	case FRAME_rails_up10:
		return MZ2_ARACHNID_RAIL_UP1;
	case FRAME_rails_up6:
	case FRAME_rails_up12:
		return MZ2_ARACHNID_RAIL_UP2;
	case FRAME_rails6:
	case FRAME_rails10:
	default:
		return MZ2_ARACHNID_RAIL1;
	}
}

static void arachnid_heat_mark(edict_t *self)
{
	if (!G_ValidTarget(self, self->enemy, true, true))
		return;
}

static void arachnid_heat(edict_t *self)
{
	int damage, speed, flash_number;
	vec3_t start, forward;

	if (!G_ValidTarget(self, self->enemy, true, true))
		return;

	damage = M_ROCKETLAUNCHER_DMG_BASE + M_ROCKETLAUNCHER_DMG_ADDON * drone_damagelevel(self);
	if (M_ROCKETLAUNCHER_DMG_MAX && damage > M_ROCKETLAUNCHER_DMG_MAX)
		damage = M_ROCKETLAUNCHER_DMG_MAX;
	speed = M_ROCKETLAUNCHER_SPEED_BASE + M_ROCKETLAUNCHER_SPEED_ADDON * drone_damagelevel(self);
	if (M_ROCKETLAUNCHER_SPEED_MAX && speed > M_ROCKETLAUNCHER_SPEED_MAX)
		speed = M_ROCKETLAUNCHER_SPEED_MAX;

	flash_number = arachnid_heat_flash(self);
	MonsterAim(self, M_PROJECTILE_ACC, speed, true, flash_number, forward, start);
	if (!M_MonsterHasClearShotFrom(self, start))
		return;

	if (monster_fire_heat(self, start, forward, damage, speed, flash_number, ARACHNID_HEAT_TURN_FRACTION))
		gi.positioned_sound(start, self, CHAN_WEAPON, sound_heat_fire, 1, ATTN_NORM, 0);
}

mframe_t arachnid_heat_frames_attack1[] =
{
	ai_charge, 0, arachnid_heat_mark,
	ai_charge, 0, arachnid_heat,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, arachnid_heat,
	ai_charge, 0, NULL
};
mmove_t arachnid_heat_move_attack1 = { FRAME_rails5, FRAME_rails11, arachnid_heat_frames_attack1, arachnid_finish_ranged_attack };

mframe_t arachnid_heat_frames_attack_up1[] =
{
	ai_charge, 0, arachnid_heat_mark,
	ai_charge, 0, arachnid_heat,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, arachnid_heat,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t arachnid_heat_move_attack_up1 = { FRAME_rails_up3, FRAME_rails_up16, arachnid_heat_frames_attack_up1, arachnid_finish_ranged_attack };

static void arachnid_jump_wait_land(edict_t *self)
{
	if (self->groundentity || self->waterlevel > 1)
	{
		self->gravity = 1.0f;
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
		VectorClear(self->velocity);
		self->monsterinfo.nextframe = self->s.frame + 1;
		return;
	}

	if (level.time > self->monsterinfo.pausetime)
	{
		self->gravity = 1.0f;
		self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
		self->velocity[0] = 0;
		self->velocity[1] = 0;
		self->monsterinfo.nextframe = self->s.frame + 1;
		return;
	}

	self->gravity = 1.3f;
	self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

static void arachnid_jump_wait_land_ai(edict_t *self, float dist)
{
	ai_move(self, dist);
	arachnid_jump_wait_land(self);
}

mframe_t arachnid_frames_dodge_jump[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	arachnid_jump_wait_land_ai, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t arachnid_move_dodge_jump = { FRAME_walk1, FRAME_walk7, arachnid_frames_dodge_jump, arachnid_run };

static qboolean arachnid_is_dodge_move(edict_t *self)
{
	return self->monsterinfo.currentmove == &arachnid_move_dodge_jump;
}

static float arachnid_dodge_side_sign(edict_t *self)
{
	return self->monsterinfo.lefty ? -1.0f : 1.0f;
}

static qboolean arachnid_dodge_side_clear(edict_t *self, vec3_t right, float side_sign)
{
	vec3_t end;
	trace_t tr;

	VectorMA(self->s.origin, side_sign * ARACHNID_DODGE_SIDE_PROBE, right, end);
	tr = gi.trace(self->s.origin, self->mins, self->maxs, end, self, MASK_MONSTERSOLID);
	return !tr.startsolid && !tr.allsolid && tr.fraction > 0.65f;
}

static qboolean arachnid_start_dodge_jump(edict_t *self, vec3_t impact)
{
	vec3_t right, diff;
	float side_sign;
	float side_dot;

	// Clean sideways hop (mirrors the reference spider_dodge_jump): strafe
	// perpendicular to our current facing, away from the shot. We deliberately
	// do NOT turn to face the attacker first - doing so made the arachnid look
	// like it was lunging toward the incoming projectile.
	AngleVectors(self->s.angles, NULL, right, NULL);
	VectorSubtract(impact, self->s.origin, diff);
	side_dot = DotProduct(right, diff);
	if (fabsf(side_dot) < 8.0f)
		self->monsterinfo.lefty = random() < 0.5f;	// shot is head-on: pick a side at random
	else
		drone_set_dodge_side(self, impact);			// otherwise hop away from the impact side
	side_sign = arachnid_dodge_side_sign(self);

	if (!arachnid_dodge_side_clear(self, right, side_sign))
	{
		if (!arachnid_dodge_side_clear(self, right, -side_sign))
			return false;

		self->monsterinfo.lefty = 1 - self->monsterinfo.lefty;
		side_sign = -side_sign;
	}

	VectorClear(self->velocity);
	VectorMA(self->velocity, side_sign * ARACHNID_DODGE_SIDE_SPEED, right, self->velocity);
	self->velocity[2] = ARACHNID_DODGE_UP_SPEED;
	self->s.origin[2] += 1;
	self->groundentity = NULL;
	self->gravity = 1.0f;
	self->monsterinfo.pausetime = level.time + ARACHNID_DODGE_TIMEOUT;
	self->monsterinfo.dodge_time = level.time + ARACHNID_DODGE_COOLDOWN;
	self->monsterinfo.currentmove = &arachnid_move_dodge_jump;
	gi.linkentity(self);
	return true;
}

// ===========================================================================
// Ceiling walking (fire_plasma arachnid only) - adapted from the stalker.
// ===========================================================================

static qboolean arachnid_ceiling_allowed(edict_t *self)
{
	// only the plasma variant climbs, and never during invasion (matches stalker policy)
	return self && self->health > 0 && self->deadflag == DEAD_NO &&
		self->mtype == M_ARACHNID_PLASMA && !invasion->value;
}

static qboolean arachnid_on_ceiling(edict_t *self)
{
	return self->style == ARACHNID_CEILING_ON;
}

static qboolean arachnid_find_ceiling(edict_t *self, float max_dist, float *ceiling_z)
{
	trace_t tr;
	vec3_t end;

	VectorCopy(self->s.origin, end);
	end[2] += max_dist;
	tr = gi.trace(self->s.origin, self->mins, self->maxs, end, self, MASK_MONSTERSOLID);

	if (tr.fraction == 1.0 || !(tr.contents & CONTENTS_SOLID) || tr.ent != world)
		return false;
	if (tr.plane.normal[2] > -0.7)
		return false;

	if (ceiling_z)
		*ceiling_z = tr.endpos[2] + self->maxs[2];
	return true;
}

// keep the collision box aligned with the model (minisentry SENTRY_FLIPPED pattern):
// when inverted on the ceiling the tall body points down and the short legs point up,
// which is a swap-and-negate of the upright Z extents (mins.z=-18, maxs.z=invasion?34:42)
static void arachnid_apply_ceiling_bbox(edict_t *self, qboolean inverted)
{
	float height = invasion->value ? 34 : 42;

	if (inverted)
	{
		self->mins[2] = -height;
		self->maxs[2] = 18;
	}
	else
	{
		self->mins[2] = -18;
		self->maxs[2] = height;
	}
}

static void arachnid_attach_ceiling(edict_t *self, float ceiling_z)
{
	self->style = ARACHNID_CEILING_ON;
	self->flags |= FL_FLY;
	self->gravity = 0;
	self->groundentity = NULL;
	self->s.angles[ROLL] = 180;
	arachnid_apply_ceiling_bbox(self, true); // flip the box before using maxs[2] below
	self->s.origin[2] = ceiling_z - self->maxs[2] - 1;
	VectorClear(self->velocity);
	self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	gi.linkentity(self);
}

static void arachnid_set_floor(edict_t *self)
{
	qboolean was_inverted = arachnid_on_ceiling(self); // style == ARACHNID_CEILING_ON
	float old_maxs = self->maxs[2];

	self->style = ARACHNID_CEILING_NONE;
	self->flags &= ~FL_FLY;
	self->gravity = 1.0;
	self->s.angles[ROLL] = 0;
	arachnid_apply_ceiling_bbox(self, false); // restore the upright box (safe even if never attached)

	// Peeling off the ceiling grows the box upward (maxs.z 18 -> 42). Drop the origin by that
	// growth so the taller upright body keeps the exact vertical span it occupied while clinging
	// (which was already free) instead of poking up into the ceiling and getting stuck. Same total
	// height, so the spot is guaranteed clear; gravity then pulls it down. (minisentry flip pattern)
	if (was_inverted)
		self->s.origin[2] -= (self->maxs[2] - old_maxs);

	self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	gi.linkentity(self);
}

static void arachnid_drop_from_ceiling(edict_t *self)
{
	arachnid_set_floor(self);
	self->groundentity = NULL;
	self->velocity[2] = -300;
	gi.linkentity(self);
}

static void arachnid_abort_ceiling_jump(edict_t *self)
{
	arachnid_set_floor(self);
	self->monsterinfo.dodge_time = level.time + 2.0f;
	if (self->groundentity)
		VectorClear(self->velocity);
	else if (self->velocity[2] > 0)
		self->velocity[2] = 0;
	arachnid_run(self);
}

static void arachnid_ceiling_jump_up(edict_t *self)
{
	float ceiling_z;

	if (!arachnid_ceiling_allowed(self))
		return;

	if (arachnid_on_ceiling(self))
	{
		arachnid_set_floor(self);
		self->velocity[2] = -300;
		return;
	}

	if (!self->groundentity)
		return;

	if (arachnid_find_ceiling(self, ARACHNID_CEILING_TRACE_DIST, &ceiling_z))
	{
		self->pos1[2] = ceiling_z;
		self->flags |= FL_FLY;
		self->gravity = 0;
		self->velocity[2] = ARACHNID_CEILING_JUMP_SPEED;
	}
	else
	{
		arachnid_abort_ceiling_jump(self);
		return;
	}

	self->style = ARACHNID_CEILING_JUMPING;
	self->s.origin[2] += 1;
	self->groundentity = NULL;
	self->velocity[0] += crandom() * 5;
	self->velocity[1] += crandom() * 5;
	self->monsterinfo.pausetime = level.time + 1.3;
	self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

static void arachnid_ceiling_jump_wait_land(edict_t *self)
{
	float ceiling_z;

	if (self->style != ARACHNID_CEILING_JUMPING)
		return;

	if (!arachnid_ceiling_allowed(self))
	{
		arachnid_set_floor(self);
		return;
	}

	if (arachnid_find_ceiling(self, ARACHNID_CEILING_TRACE_DIST, &ceiling_z))
		self->pos1[2] = ceiling_z;

	if (self->pos1[2] && self->s.origin[2] + self->maxs[2] >= self->pos1[2] - 12)
	{
		arachnid_attach_ceiling(self, self->pos1[2]);
		return;
	}

	if (self->pos1[2])
	{
		self->flags |= FL_FLY;
		self->gravity = 0;
		if (self->velocity[2] < ARACHNID_CEILING_MIN_SPEED)
			self->velocity[2] = ARACHNID_CEILING_MIN_SPEED;
	}

	if (self->groundentity || level.time > self->monsterinfo.pausetime)
	{
		arachnid_abort_ceiling_jump(self);
		return;
	}

	self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

static void arachnid_ceiling_jump_wait_land_ai(edict_t *self, float dist)
{
	ai_move(self, dist);
	arachnid_ceiling_jump_wait_land(self);
}

mframe_t arachnid_frames_ceiling_jump[] =
{
	ai_move, 1, arachnid_ceiling_jump_up,
	arachnid_ceiling_jump_wait_land_ai, 1, NULL,
	ai_move, -1, NULL,
	ai_move, -1, NULL
};
mmove_t arachnid_move_ceiling_jump = { FRAME_walk1, FRAME_walk4, arachnid_frames_ceiling_jump, arachnid_run };

static qboolean arachnid_start_ceiling_jump(edict_t *self, float cooldown)
{
	float ceiling_z;

	if (!arachnid_ceiling_allowed(self) || arachnid_on_ceiling(self) || !self->groundentity)
		return false;
	if (arachnid_is_dodge_move(self) || level.time < self->monsterinfo.dodge_time)
		return false;
	if (!arachnid_find_ceiling(self, ARACHNID_CEILING_TRACE_DIST, &ceiling_z))
	{
		self->monsterinfo.dodge_time = level.time + cooldown;
		return false;
	}

	self->monsterinfo.dodge_time = level.time + cooldown;
	self->pos1[2] = ceiling_z;
	self->monsterinfo.currentmove = &arachnid_move_ceiling_jump;
	arachnid_ceiling_jump_up(self);
	return true;
}

static void arachnid_ceiling_prethink(edict_t *self)
{
	float ceiling_z;

	if (self->style == ARACHNID_CEILING_JUMPING)
	{
		arachnid_ceiling_jump_wait_land(self);
		return;
	}

	if (!arachnid_on_ceiling(self))
		return;

	if (!arachnid_ceiling_allowed(self))
	{
		arachnid_drop_from_ceiling(self);
		return;
	}

	// come back down once there's nothing left to fight up here
	if (!G_EntIsAlive(self->enemy))
	{
		arachnid_drop_from_ceiling(self);
		return;
	}

	if (arachnid_find_ceiling(self, 96, &ceiling_z))
		arachnid_attach_ceiling(self, ceiling_z);
	else
		arachnid_drop_from_ceiling(self);
}

static void arachnid_dodge(edict_t *self, edict_t *attacker, vec3_t dir, int radius)
{
	if (level.time < self->monsterinfo.dodge_time)
		return;
	if (!attacker || OnSameTeam(self, attacker))
		return;
	if (!self->groundentity || self->health <= 0 || arachnid_is_dodge_move(self))
		return;
	if (self->monsterinfo.currentmove == &arachnid_move_melee ||
		self->monsterinfo.currentmove == &arachnid_move_attack1 ||
		self->monsterinfo.currentmove == &arachnid_move_attack_up1 ||
		self->monsterinfo.currentmove == &arachnid_heat_move_attack1 ||
		self->monsterinfo.currentmove == &arachnid_heat_move_attack_up1)
		return;

	if (!G_EntIsAlive(self->enemy) && G_EntIsAlive(attacker))
		self->enemy = attacker;
	self->monsterinfo.attacker = attacker;

	// plasma arachnid: sometimes evade by leaping up to the ceiling
	if (arachnid_ceiling_allowed(self) && (radius || random() < 0.35f) &&
		arachnid_start_ceiling_jump(self, 1.0f + random() * 1.5f))
		return;

	if (!radius && random() > 0.75f)
		return;

	if (!arachnid_start_dodge_jump(self, dir))
		self->monsterinfo.dodge_time = level.time + 0.3f;
}

static void arachnid_melee_charge(edict_t *self)
{
	if (!G_ValidTarget(self, self->enemy, true, true) || entdist(self, self->enemy) > 96)
		return;

	gi.sound(self, CHAN_WEAPON, sound_melee, 1, ATTN_NORM, 0);
}

static void arachnid_melee_hit(edict_t *self)
{
	int damage;

	if (!G_ValidTarget(self, self->enemy, true, true))
		return;
	if (entdist(self, self->enemy) > 96)
	{
		self->monsterinfo.melee_finished = level.time + 1.0;
		return;
	}

	damage = M_MELEE_DMG_BASE + M_MELEE_DMG_ADDON * drone_damagelevel(self);
	if (M_MELEE_DMG_MAX && damage > M_MELEE_DMG_MAX)
		damage = M_MELEE_DMG_MAX;

	if (M_MeleeAttack(self, self->enemy, 96, damage, 100))
		gi.sound(self, CHAN_WEAPON, sound_melee_hit, 1, ATTN_NORM, 0);
	else
		self->monsterinfo.melee_finished = level.time + 1.0;
}

mframe_t arachnid_frames_melee[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, arachnid_melee_charge,
	ai_charge, 0, arachnid_melee_hit,
	ai_charge, 0, arachnid_melee_hit,
	ai_charge, 0, arachnid_melee_charge,
	ai_charge, 0, arachnid_melee_hit,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, arachnid_melee_charge,
	ai_charge, 0, NULL,
	ai_charge, 0, arachnid_melee_hit,
	ai_charge, 0, NULL
};
mmove_t arachnid_move_melee = { FRAME_melee_atk1, FRAME_melee_atk12, arachnid_frames_melee, arachnid_run };

static void arachnid_set_ranged_attack(edict_t *self)
{
	qboolean high_target;

	if (!G_ValidTarget(self, self->enemy, true, true))
		return;

	high_target = (self->enemy->s.origin[2] - self->s.origin[2]) > 150;
	if (self->mtype == M_ARACHNID_HEAT)
		self->monsterinfo.currentmove = high_target ? &arachnid_heat_move_attack_up1 : &arachnid_heat_move_attack1;
	else
		self->monsterinfo.currentmove = high_target ? &arachnid_move_attack_up1 : &arachnid_move_attack1;
}

static void arachnid_attack(edict_t *self)
{
	if (!G_ValidTarget(self, self->enemy, true, true))
		return;

	if (arachnid_on_ceiling(self))
	{
		// drop back down only when the enemy gets too close to snipe from above
		// (the prethink handles dropping once the enemy is dead or the ceiling
		// is gone) - otherwise it would leave the ceiling far too soon
		if (entdist(self, self->enemy) < 96)
		{
			arachnid_set_floor(self);
			self->velocity[2] = -300;
			arachnid_run(self);
			M_DelayNextAttack(self, 0.6, true);
			return;
		}

		// otherwise stay put and keep firing plasma from the ceiling
		arachnid_set_ranged_attack(self);
		M_DelayNextAttack(self, 0, true);
		return;
	}

	// sometimes leap up to the ceiling to reposition
	if (arachnid_ceiling_allowed(self) && self->groundentity &&
		level.time > self->monsterinfo.melee_finished && random() < 0.33f &&
		arachnid_start_ceiling_jump(self, 3.0))
	{
		self->monsterinfo.melee_finished = level.time + 3.0;
		M_DelayNextAttack(self, 1.0, true);
		return;
	}

	if (self->monsterinfo.melee_finished < level.time && entdist(self, self->enemy) < MELEE_DISTANCE)
		self->monsterinfo.currentmove = &arachnid_move_melee;
	else
		arachnid_set_ranged_attack(self);

	M_DelayNextAttack(self, 0, true);
}

static void arachnid_heat_attack(edict_t *self)
{
	arachnid_attack(self);
}

static void arachnid_melee(edict_t *self)
{
	if (!G_ValidTarget(self, self->enemy, true, true))
		return;
	if (entdist(self, self->enemy) > 96)
		arachnid_set_ranged_attack(self);
	else
		self->monsterinfo.currentmove = &arachnid_move_melee;
	M_DelayNextAttack(self, 0, true);
}

mframe_t arachnid_frames_pain1[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t arachnid_move_pain1 = { FRAME_pain11, FRAME_pain15, arachnid_frames_pain1, arachnid_run };

mframe_t arachnid_frames_pain2[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t arachnid_move_pain2 = { FRAME_pain21, FRAME_pain26, arachnid_frames_pain2, arachnid_run };

static void arachnid_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (arachnid_is_dodge_move(self))
		return;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3.0;
	gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

	if (invasion->value == 2)
		return;

	// knocked out of a ceiling leap -> land and recover
	if (self->style == ARACHNID_CEILING_JUMPING && damage > 10)
	{
		arachnid_abort_ceiling_jump(self);
		self->monsterinfo.currentmove = &arachnid_move_pain1;
		return;
	}

	// ride out the hit while clinging to the ceiling
	if (arachnid_on_ceiling(self))
		return;

	if (random() < 0.5)
		self->monsterinfo.currentmove = &arachnid_move_pain1;
	else
		self->monsterinfo.currentmove = &arachnid_move_pain2;
}

static void arachnid_dead(edict_t *self)
{
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
	M_PrepBodyRemoval(self);
}

mframe_t arachnid_frames_death[] =
{
	ai_move, 0, NULL,
	ai_move, -1.23, NULL,
	ai_move, -1.23, NULL,
	ai_move, -1.23, NULL,
	ai_move, -1.23, NULL,
	ai_move, -1.64, NULL,
	ai_move, -1.64, NULL,
	ai_move, -2.45, NULL,
	ai_move, -8.63, NULL,
	ai_move, -4.0, NULL,
	ai_move, -4.5, NULL,
	ai_move, -6.8, NULL,
	ai_move, -8.0, NULL,
	ai_move, -5.4, NULL,
	ai_move, -3.4, NULL,
	ai_move, -1.9, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t arachnid_move_death = { FRAME_death1, FRAME_death20, arachnid_frames_death, arachnid_dead };

static void arachnid_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	M_Notify(self);

	arachnid_set_floor(self); // ensure the corpse falls if killed on the ceiling

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
	self->monsterinfo.currentmove = &arachnid_move_death;

	if (self->activator && !self->activator->client)
		self->activator->num_monsters_real--;
}

// ===========================================================================
// Rail variant (M_ARACHNID): a grounded railgun sniper. Each time it
// engages it rolls between a fast, accurate rail volley and an "enrage" that
// summons a pair of stalkers to swarm the target.
// ===========================================================================

static constexpr float ARACHNID_RAIL_HEALTH_SCALE = 1.5f;	// tankier than the plasma variant
static constexpr int   ARACHNID_RAIL_SUMMON_MISS_THRESHOLD = 4;	// consecutive missed rails before it enrages and summons
static constexpr int   ARACHNID_RAIL_STALKER_COUNT = 2;		// stalkers spawned per summon
static constexpr float ARACHNID_RAIL_SUMMON_COOLDOWN = 14.0f;	// min time between summons (stored in self->wait)
static constexpr int   ARACHNID_RAIL_RAPID_MISS_THRESHOLD = 3;	// fewer misses than the summon, so the enrage fires sooner/more often
static constexpr float ARACHNID_RAIL_RAPID_COOLDOWN = 8.0f;	// min time between enrage bursts (stored in self->last_move_time)

// The rail volley fires on rails4 + rails8; map those to the two barrels so the second shot
// uses RAIL2 instead of falling through arachnid_plasma_flash's default to RAIL1.
static int arachnid_rail_flash(edict_t *self)
{
	switch (self->s.frame)
	{
	case FRAME_rails8:
		return MZ2_ARACHNID_RAIL2;
	case FRAME_rails4:
	default:
		return MZ2_ARACHNID_RAIL1;
	}
}

static void arachnid_rail_fire(edict_t *self)
{
	int damage, flash_number;
	vec3_t start, forward;

	if (!G_ValidTarget(self, self->enemy, true, true))
		return;

	damage = M_RAILGUN_DMG_BASE + M_RAILGUN_DMG_ADDON * drone_damagelevel(self);
	if (M_RAILGUN_DMG_MAX && damage > M_RAILGUN_DMG_MAX)
		damage = M_RAILGUN_DMG_MAX;

	flash_number = arachnid_rail_flash(self); // alternate barrels: rails4 -> RAIL1, rails8 -> RAIL2
	MonsterAim(self, M_HITSCAN_INSTANT_ACC, 0, false, flash_number, forward, start);
	if (!M_MonsterHasClearShotFrom(self, start))
		return;

	gi.sound(self, CHAN_WEAPON, sound_heat_fire, 1, ATTN_NORM, 0);
	// track consecutive misses (PSX style): a clean hit resets the meter, a miss
	// nudges it toward the enrage summon
	if (monster_fire_railgun(self, start, forward, damage, damage, flash_number))
		self->count = 0;
	else
		self->count++;
}

// Two accurate rail shots across the firing animation: charge then fire, twice (PSX cadence).
mframe_t arachnid_rail_frames_attack[] =
{
	ai_charge, 0, arachnid_charge_plasma,
	ai_charge, 0, NULL,
	ai_charge, 0, arachnid_rail_fire,
	ai_charge, 0, NULL,
	ai_charge, 0, arachnid_charge_plasma,
	ai_charge, 0, NULL,
	ai_charge, 0, arachnid_rail_fire,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t arachnid_rail_move_attack = { FRAME_rails2, FRAME_rails11, arachnid_rail_frames_attack, arachnid_finish_ranged_attack };

// --- stalker summon (mirrors the black widow's native stalker spawn) ---

static qboolean arachnid_rail_valid_spawn_spot(edict_t *self, vec3_t mins, vec3_t maxs, vec3_t spot)
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

static qboolean arachnid_rail_find_spawn_spot(edict_t *self, int index, vec3_t spot)
{
	vec3_t mins, maxs, forward, right;
	float side;

	VectorSet(mins, -28, -28, -18);
	VectorSet(maxs, 28, 28, 18);
	AngleVectors(self->s.angles, forward, right, NULL);

	side = (index & 1) ? 112.0f : -112.0f;
	VectorCopy(self->s.origin, spot);
	VectorMA(spot, 96.0f, forward, spot);
	VectorMA(spot, side, right, spot);
	if (arachnid_rail_valid_spawn_spot(self, mins, maxs, spot))
		return true;

	VectorCopy(self->s.origin, spot);
	VectorMA(spot, -96.0f, forward, spot);
	VectorMA(spot, side, right, spot);
	return arachnid_rail_valid_spawn_spot(self, mins, maxs, spot);
}

static void arachnid_rail_cleanup_failed_spawn(edict_t *owner, edict_t *spawned)
{
	if (owner && owner->client)
		layout_remove_tracked_entity(&owner->client->layout, spawned);

	DroneList_Remove(spawned);
	AI_EnemyRemoved(spawned);
	G_FreeEdict(spawned);
}

static void arachnid_rail_start_spawned(edict_t *self, edict_t *spawned)
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

static qboolean arachnid_rail_spawn_stalker(edict_t *self, int index)
{
	edict_t *owner, *spawned;
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

	if (!arachnid_rail_find_spawn_spot(self, index, spot))
	{
		arachnid_rail_cleanup_failed_spawn(owner, spawned);
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

	gi.linkentity(spawned);
	owner->num_monsters += spawned->monsterinfo.control_cost;
	owner->num_monsters_real++;

	// PSX-style spawn-in effect at the stalker's location
	{
		vec3_t mins, maxs, size;
		float radius;

		VectorSet(mins, -28, -28, -18);
		VectorSet(maxs, 28, 28, 18);
		VectorSubtract(maxs, mins, size);
		radius = VectorLength(size) * 0.5f;
		SpawnGrow_Spawn(spot, radius, radius * 2.0f);
	}

	arachnid_rail_start_spawned(self, spawned);
	return true;
}

static void arachnid_rail_summon(edict_t *self)
{
	int i;

	if (!G_EntExists(self->enemy))
		return;

	for (i = 0; i < ARACHNID_RAIL_STALKER_COUNT; i++)
		arachnid_rail_spawn_stalker(self, i);

	gi.sound(self, CHAN_WEAPON, sound_monster_spawn, 1, ATTN_NORM, 0);
//	gi.sound(self, CHAN_VOICE, sound_angry, 1, ATTN_NORM, 0);

}

mframe_t arachnid_rail_frames_summon[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, arachnid_rail_summon,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t arachnid_rail_move_summon = { FRAME_rails2, FRAME_rails11, arachnid_rail_frames_summon, arachnid_finish_ranged_attack };

// --- enrage rapid-fire (mirrors the PSX arachnid's taunt -> rapid rail burst) ---

// the burst fires on melee_in5/8/11/14; alternate the two barrels across those shots
static int arachnid_rail_rapid_flash(edict_t *self)
{
	switch (self->s.frame)
	{
	case FRAME_melee_in8:
	case FRAME_melee_in14:
		return MZ2_ARACHNID_RAIL2;
	case FRAME_melee_in5:
	case FRAME_melee_in11:
	default:
		return MZ2_ARACHNID_RAIL1;
	}
}

static void arachnid_rail_fire_rapid(edict_t *self)
{
	int damage, flash_number;
	vec3_t start, forward;

	if (!G_ValidTarget(self, self->enemy, true, true))
		return;

	damage = M_RAILGUN_DMG_BASE + M_RAILGUN_DMG_ADDON * drone_damagelevel(self);
	if (M_RAILGUN_DMG_MAX && damage > M_RAILGUN_DMG_MAX)
		damage = M_RAILGUN_DMG_MAX;

	flash_number = arachnid_rail_rapid_flash(self);
	MonsterAim(self, M_HITSCAN_INSTANT_ACC, 0, false, flash_number, forward, start);
	if (!M_MonsterHasClearShotFrom(self, start))
		return;

	gi.sound(self, CHAN_WEAPON, sound_heat_fire, 1, ATTN_NORM, 0);
	// enrage burst: the miss meter was reset when the enrage began, so don't track it here
	monster_fire_railgun(self, start, forward, damage, damage, flash_number);
}

// faster cadence than the normal volley: four rails ~300ms apart with no charge wind-up
mframe_t arachnid_rail_frames_rapid[] =
{
	ai_charge, 0, NULL,
	ai_move,   0, arachnid_rail_fire_rapid,
	ai_move,   0, NULL,
	ai_move,   0, NULL,
	ai_move,   0, arachnid_rail_fire_rapid,
	ai_move,   0, NULL,
	ai_move,   0, NULL,
	ai_move,   0, arachnid_rail_fire_rapid,
	ai_move,   0, NULL,
	ai_move,   0, NULL,
	ai_move,   0, arachnid_rail_fire_rapid,
	ai_move,   0, NULL,
	ai_charge, 0, NULL
};
mmove_t arachnid_rail_move_rapid = { FRAME_melee_in4, FRAME_melee_in16, arachnid_rail_frames_rapid, arachnid_finish_ranged_attack };

static void arachnid_rail_rapid_fire(edict_t *self)
{
	self->count = 0;
	self->monsterinfo.currentmove = &arachnid_rail_move_rapid;
}

// enrage taunt/wind-up before the burst (the angry cry is a PSX-only asset, left commented)
mframe_t arachnid_rail_frames_taunt[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
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
mmove_t arachnid_rail_move_taunt = { FRAME_melee_pain1, FRAME_melee_pain16, arachnid_rail_frames_taunt, arachnid_rail_rapid_fire };

static void arachnid_rail_attack(edict_t *self)
{
	qboolean want_rapid;
	float chance;

	if (!G_ValidTarget(self, self->enemy, true, true))
		return;

	// up close it bites instead of sniping
	if (self->monsterinfo.melee_finished < level.time && entdist(self, self->enemy) < MELEE_DISTANCE)
	{
		self->monsterinfo.currentmove = &arachnid_move_melee;
		M_DelayNextAttack(self, 0, true);
		return;
	}

	// enrage rapid-fire: after enough whiffed rails it crouches into a taunt, then unloads a
	// fast burst. Lower threshold + its own cooldown (self->last_move_time) so it triggers
	// sooner and more often than the stalker summon below.
	want_rapid = (self->count >= ARACHNID_RAIL_RAPID_MISS_THRESHOLD &&
		level.time >= self->last_move_time &&
		M_MonsterHasClearShotFromFlash(self, MZ2_ARACHNID_RAIL1));
	if (want_rapid)
	{
		chance = self->count / 8.0f + 0.25f;
		if (chance > 0.9f)
			chance = 0.9f;
		want_rapid = (random() < chance);
	}

	if (want_rapid)
	{
		self->count = 0;
		self->monsterinfo.currentmove = &arachnid_rail_move_taunt;
		self->last_move_time = level.time + ARACHNID_RAIL_RAPID_COOLDOWN;
	//	gi.sound(self, CHAN_VOICE, sound_angry, 1, ATTN_NORM, 0); // enrage cry (PSX asset only)
	}
	// otherwise, once it has whiffed enough rails it summons a fresh pair of stalkers
	// (PSX behaviour), gated by self->wait so it can't flood the arena
	else if (self->count >= ARACHNID_RAIL_SUMMON_MISS_THRESHOLD && level.time >= self->wait)
	{
		self->count = 0;
		self->monsterinfo.currentmove = &arachnid_rail_move_summon;
		self->wait = level.time + ARACHNID_RAIL_SUMMON_COOLDOWN;
	}
	else
		self->monsterinfo.currentmove = &arachnid_rail_move_attack;

	M_DelayNextAttack(self, 0, true);
}

void init_drone_arachnid_plasma(edict_t *self)
{
	sound_step = gi.soundindex("insane/insane11.wav");
	sound_charge = gi.soundindex("weapons/plasshot.wav");
	sound_heat_fire = gi.soundindex("weapons/railgr1a.wav");
	sound_melee = gi.soundindex("gladiator/melee3.wav");
	sound_melee_hit = gi.soundindex("gladiator/melee2.wav");
	sound_pain = gi.soundindex("arachnid/pain.wav");
	sound_death = gi.soundindex("arachnid/death.wav");
	sound_sight = gi.soundindex("arachnid/sight.wav");
	sound_monster_spawn = gi.soundindex("medic_commander/monsterspawn1.wav");
//	sound_angry = gi.soundindex("arachnid/angry.wav");


	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/arachnid/tris.md2");
	if (invasion->value)
	{
		VectorSet(self->mins, -28, -28, -18);
		VectorSet(self->maxs, 28, 28, 34);
		self->s.scale = ARACHNID_INVASION_SCALE;
	}
	else
	{
		VectorSet(self->mins, -36, -36, -18);
		VectorSet(self->maxs, 36, 36, 42);
		self->s.scale = ARACHNID_DEFAULT_SCALE;
	}

	self->health = M_ARACHNID_INITIAL_HEALTH + M_ARACHNID_ADDON_HEALTH * self->monsterinfo.level;
	self->max_health = self->health;
	self->gib_health = -200;
	self->mass = 450;
	self->mtype = M_ARACHNID_PLASMA;
	self->monsterinfo.control_cost = M_GLADIATOR_CONTROL_COST;
	self->monsterinfo.cost = M_DEFAULT_COST;
	M_SetMonsterArmor(self, M_ARACHNID_INITIAL_ARMOR + M_ARACHNID_ADDON_ARMOR * self->monsterinfo.level);
	self->monsterinfo.jumpup = 64;
	self->monsterinfo.jumpdn = 512;
	self->monsterinfo.aiflags |= AI_NO_CIRCLE_STRAFE;
	self->monsterinfo.pain_chance = 0.2f;
	self->item = FindItemByClassname("ammo_slugs");

	self->pain = arachnid_pain;
	self->die = arachnid_die;
	self->monsterinfo.stand = arachnid_stand;
	self->monsterinfo.walk = arachnid_walk;
	self->monsterinfo.run = arachnid_run;
	self->monsterinfo.attack = arachnid_attack;
	self->monsterinfo.melee = arachnid_melee;
	self->monsterinfo.dodge = arachnid_dodge;
	self->prethink = arachnid_ceiling_prethink; // fire_plasma variant climbs to ceilings
	self->monsterinfo.sight = arachnid_sight;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &arachnid_move_stand;
	self->monsterinfo.scale = MODEL_SCALE * self->s.scale;
	self->nextthink = level.time + FRAMETIME;
}

void init_drone_arachnid_heat(edict_t *self)
{
	init_drone_arachnid_plasma(self);

	self->mtype = M_ARACHNID_HEAT;
	self->prethink = NULL; // heat variant stays grounded (no ceiling walking)
	self->monsterinfo.dodge = NULL; // only the plasma variant dodges + ceiling-walks
	self->s.scale = invasion->value ? ARACHNID_HEAT_INVASION_SCALE : ARACHNID_HEAT_DEFAULT_SCALE;
	self->health = M_ARACHNID_HEAT_INITIAL_HEALTH + M_ARACHNID_HEAT_ADDON_HEALTH * self->monsterinfo.level;
	self->max_health = self->health;
	M_SetMonsterArmor(self, M_ARACHNID_HEAT_INITIAL_ARMOR + M_ARACHNID_HEAT_ADDON_ARMOR * self->monsterinfo.level);
	self->monsterinfo.attack = arachnid_heat_attack;
	self->monsterinfo.currentmove = &arachnid_move_stand;
	self->monsterinfo.scale = MODEL_SCALE * self->s.scale;
	gi.linkentity(self);
}

void init_drone_arachnid(edict_t *self)
{
	init_drone_arachnid_plasma(self);

	self->mtype = M_ARACHNID;
	self->prethink = NULL; // canonical (ex-rail) variant stays grounded (no ceiling walking)
	self->monsterinfo.dodge = NULL; // only the plasma variant dodges + ceiling-walks
	self->health = (int)((M_ARACHNID_INITIAL_HEALTH + M_ARACHNID_ADDON_HEALTH * self->monsterinfo.level) * ARACHNID_RAIL_HEALTH_SCALE);
	self->max_health = self->health;
	self->monsterinfo.attack = arachnid_rail_attack;
	self->monsterinfo.currentmove = &arachnid_move_stand;
	self->monsterinfo.scale = MODEL_SCALE * self->s.scale;
	gi.linkentity(self);
}
