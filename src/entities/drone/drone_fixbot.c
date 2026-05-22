/*
==============================================================================

fixbot

==============================================================================
*/

#include "g_local.h"

#define FIXBOT_FRAME_charging_01	0
#define FIXBOT_FRAME_charging_27	26
#define FIXBOT_FRAME_charging_31	30
#define FIXBOT_FRAME_ambient_01		121
#define FIXBOT_FRAME_ambient_19		139
#define FIXBOT_FRAME_paina_01		140
#define FIXBOT_FRAME_paina_06		145
#define FIXBOT_FRAME_painb_01		146
#define FIXBOT_FRAME_painb_08		153
#define FIXBOT_FRAME_freeze_01		181
#define FIXBOT_FRAME_weldstart_01	188
#define FIXBOT_FRAME_weldstart_07	194

#define FIXBOT_BOSS_TURRET_MAX		6
#define FIXBOT_BOSS_SPAWN_COOLDOWN	8.0f
#define FIXBOT_BOSS_FAIL_COOLDOWN	2.0f
#define FIXBOT_BLASTER_FLASH			MZ2_HOVER_BLASTER_1
#define FIXBOT_BOSS_DEFAULT_SCALE		2.6f
#define FIXBOT_BOSS_INVASION_SCALE		2.0f
#define FIXBOT_BOSS_INVASION_MOVE_SCALE	1.5f
#define FIXBOT_SPAWN_YAW_SPEED			12.0f
#define FIXBOT_SPAWN_PITCH_SPEED		12.0f
#define FIXBOT_SPAWN_AIM_TIMEOUT		2.0f
#define FIXBOT_SPAWN_AIM_EPSILON		5.0f
#define FIXBOT_NO_SPAWN_YAW			-99999.0f

static int sound_pain;
static int sound_die;
static int sound_pew;
static int sound_ionripper;
static int sound_weld;
static int sound_spawn;

void drone_ai_stand(edict_t *self, float dist);
void drone_ai_walk(edict_t *self, float dist);
void drone_ai_run(edict_t *self, float dist);
void rogue_turret_force_ready(edict_t *self);

static void fixbot_stand(edict_t *self);
static void fixbot_walk(edict_t *self);
static void fixbot_run(edict_t *self);
static void fixbot_attack(edict_t *self);
static void fixbot_try_start_spawn(edict_t *self);
static mmove_t fixbot_move_spawn;

static qboolean fixbot_is_boss(edict_t *self)
{
	return self->mtype == M_FIXBOT_BOSS;
}

static float fixbot_move_scale(edict_t *self)
{
	return (fixbot_is_boss(self) && invasion->value) ? FIXBOT_BOSS_INVASION_MOVE_SCALE : 1.0f;
}

static void fixbot_ai_run(edict_t *self, float dist)
{
	drone_ai_run(self, dist * fixbot_move_scale(self));
}

static void fixbot_ai_walk(edict_t *self, float dist)
{
	drone_ai_walk(self, dist * fixbot_move_scale(self));
}

static void fixbot_set_fly_parameters(edict_t *self)
{
	float speed_scale = fixbot_move_scale(self);

	self->monsterinfo.fly_thrusters = false;
	self->monsterinfo.fly_acceleration = 20.0f * speed_scale;
	self->monsterinfo.fly_speed = 120.0f * speed_scale;
	if (fixbot_is_boss(self))
	{
		self->monsterinfo.fly_min_distance = 220.0f;
		self->monsterinfo.fly_max_distance = 650.0f;
	}
	else
	{
		self->monsterinfo.fly_min_distance = 250.0f;
		self->monsterinfo.fly_max_distance = 450.0f;
	}
}

static int fixbot_count_live_turrets(edict_t *self)
{
	int count = 0;
	edict_t *ent = DroneList_Iterate();

	while (ent)
	{
		if (G_EntExists(ent) && ent->owner == self && ent->mtype == M_ROGUE_TURRET)
			count++;
		ent = DroneList_Next(ent);
	}

	return count;
}

static void fixbot_remove_turrets(edict_t *self)
{
	edict_t *ent = DroneList_Iterate();

	while (ent)
	{
		edict_t *next = DroneList_Next(ent);

		if (G_EntExists(ent) && ent->owner == self && ent->mtype == M_ROGUE_TURRET)
		{
			if (ent->die)
				ent->die(ent, self, self, ent->health + 100, ent->s.origin);
			else
				M_Remove(ent, false, true);
		}

		ent = next;
	}
}

static qboolean fixbot_turret_position_clear(edict_t *self, vec3_t position)
{
	edict_t *ent = NULL;

	if (gi.pointcontents(position) & MASK_WATER)
		return false;

	while ((ent = findradius(ent, position, 144)) != NULL)
	{
		if (!ent->inuse || ent == self)
			continue;
		if (ent->mtype == M_ROGUE_TURRET)
			return false;
		if ((ent->client || (ent->svflags & SVF_MONSTER)) && ent->health > 0 && ent->solid != SOLID_NOT)
			return false;
		if (ent->solid == SOLID_BSP || (ent->solid == SOLID_BBOX && ent->takedamage))
			return false;
	}

	return true;
}

static float fixbot_angle_delta(float a, float b)
{
	float delta = anglemod(a - b);

	if (delta > 180.0f)
		delta = 360.0f - delta;
	return delta;
}

static qboolean fixbot_set_spawn_base_yaw(edict_t *self)
{
	vec3_t to_enemy;

	if (!G_EntExists(self->enemy))
		return false;

	VectorSubtract(self->enemy->s.origin, self->s.origin, to_enemy);
	self->move_angles[YAW] = vectoyaw(to_enemy);
	return true;
}

static void fixbot_set_spawn_yaw(edict_t *self)
{
	static const float yaw_offsets[] = { -35.0f, 35.0f, 0.0f, -70.0f, 70.0f, -110.0f, 110.0f, 180.0f };
	int index;

	index = fixbot_count_live_turrets(self) % (int)(sizeof(yaw_offsets) / sizeof(yaw_offsets[0]));
	self->angle = anglemod(self->move_angles[YAW] + yaw_offsets[index]);
	self->ideal_yaw = self->angle;
	self->teleport_time = level.time + FIXBOT_SPAWN_AIM_TIMEOUT;
}

static void fixbot_turn_to_spawn_yaw(edict_t *self)
{
	float old_yaw_speed;

	if (self->angle <= FIXBOT_NO_SPAWN_YAW + 1.0f)
		return;

	VectorClear(self->velocity);
	VectorClear(self->avelocity);
	self->ideal_yaw = self->angle;
	old_yaw_speed = self->yaw_speed;
	self->yaw_speed = FIXBOT_SPAWN_YAW_SPEED;
	M_ChangeYaw(self);
	self->yaw_speed = old_yaw_speed;
}

static qboolean fixbot_find_turret_spawn_position(edict_t *self, vec3_t position, vec3_t direction)
{
	vec3_t start;
	vec3_t base_angles;
	vec3_t initial_forward;
	vec3_t best_pos;
	vec3_t best_dir;
	float best_dist = 0;
	qboolean found = false;
	qboolean best_front = false;
	const float trace_distance = 1000.0f;
	vec3_t turret_mins = { -12, -12, -12 };
	vec3_t turret_maxs = { 12, 12, 12 };

	VectorCopy(self->s.origin, start);
	start[2] += 16;
	VectorCopy(self->s.angles, base_angles);
	if (self->angle > FIXBOT_NO_SPAWN_YAW + 1.0f)
		base_angles[YAW] = self->angle;
	base_angles[PITCH] = 0.0f;
	AngleVectors(base_angles, initial_forward, NULL, NULL);
	VectorClear(best_pos);
	VectorClear(best_dir);

	for (int attempt = 0; attempt < 12; attempt++)
	{
		vec3_t angles;
		vec3_t forward;
		vec3_t end;
		vec3_t candidate;
		vec3_t normal;
		vec3_t to_pos;
		trace_t tr;
		float dist;
		qboolean in_front;
		qboolean better = false;

		VectorCopy(base_angles, angles);
		if (attempt == 0)
		{
			VectorCopy(initial_forward, forward);
		}
		else
		{
			if (attempt <= 6)
			{
				angles[YAW] += (float)(attempt - 1) * 30.0f - 75.0f;
				angles[PITCH] = -15.0f;
			}
			else
			{
				angles[YAW] += (float)(attempt - 7) * 60.0f - 150.0f;
				angles[PITCH] += crandom() * 15.0f - 15.0f;
			}

			while (angles[YAW] < 0)
				angles[YAW] += 360;
			while (angles[YAW] >= 360)
				angles[YAW] -= 360;
			AngleVectors(angles, forward, NULL, NULL);
		}

		VectorMA(start, trace_distance, forward, end);
		tr = gi.trace(start, NULL, NULL, end, self, MASK_SOLID);
		if (tr.fraction >= 1.0)
			continue;
		if (tr.ent && tr.ent != world && tr.ent->solid == SOLID_BBOX)
			continue;

		VectorCopy(tr.plane.normal, normal);
		if (fabs(normal[2]) > 0.9f)
		{
			normal[2] = (normal[2] > 0) ? 0.7f : -0.7f;
			VectorNormalize(normal);
		}

		VectorMA(tr.endpos, 16.0f, normal, candidate);
		if (!G_IsValidLocation(self, candidate, turret_mins, turret_maxs))
			continue;
		if (!fixbot_turret_position_clear(self, candidate))
			continue;
		if (!G_IsClearPath(self, MASK_SOLID, start, candidate))
			continue;

		VectorSubtract(candidate, self->s.origin, to_pos);
		dist = VectorLength(to_pos);
		if (dist <= 56.0f)
			continue;
		VectorNormalize(to_pos);
		in_front = DotProduct(to_pos, initial_forward) > 0.1f;

		if (!found)
			better = true;
		else if (in_front && !best_front)
			better = true;
		else if (in_front == best_front && dist < best_dist)
			better = true;

		if (better)
		{
			VectorCopy(candidate, best_pos);
			VectorCopy(normal, best_dir);
			best_dist = dist;
			best_front = in_front;
			found = true;
		}
	}

	if (!found)
	{
		const float distances[] = { 120.0f, 180.0f, 240.0f, 320.0f };
		const float yaws[] = { 0.0f, -45.0f, 45.0f, -90.0f, 90.0f, -135.0f, 135.0f, 180.0f };

		for (int d = 0; d < (int)(sizeof(distances) / sizeof(distances[0])); d++)
		{
			for (int y = 0; y < (int)(sizeof(yaws) / sizeof(yaws[0])); y++)
			{
				vec3_t angles;
				vec3_t forward;
				vec3_t probe;
				vec3_t down;
				vec3_t candidate;
				vec3_t to_pos;
				trace_t tr;

				VectorCopy(base_angles, angles);
				angles[YAW] += yaws[y];
				while (angles[YAW] < 0)
					angles[YAW] += 360;
				while (angles[YAW] >= 360)
					angles[YAW] -= 360;
				angles[PITCH] = 0;
				AngleVectors(angles, forward, NULL, NULL);

				VectorMA(self->s.origin, distances[d], forward, probe);
				probe[2] += 96.0f;
				VectorCopy(probe, down);
				down[2] -= 384.0f;
				tr = gi.trace(probe, NULL, NULL, down, self, MASK_SOLID);
				if (tr.startsolid || tr.allsolid || tr.fraction >= 1.0f)
					continue;
				if (tr.surface && (tr.surface->flags & SURF_SKY))
					continue;
				if (tr.plane.normal[2] < 0.7f)
					continue;

				VectorCopy(tr.endpos, candidate);
				candidate[2] -= turret_mins[2];
				if (!G_IsValidLocation(self, candidate, turret_mins, turret_maxs))
					continue;
				if (!fixbot_turret_position_clear(self, candidate))
					continue;
				if (!G_IsClearPath(self, MASK_SOLID, start, candidate))
					continue;

				VectorSubtract(candidate, self->s.origin, to_pos);
				if (VectorLength(to_pos) <= 56.0f)
					continue;

				VectorCopy(candidate, position);
				VectorSet(direction, 0, 0, 1);
				return true;
			}
		}

		VectorClear(position);
		VectorClear(direction);
		return false;
	}

	VectorCopy(best_pos, position);
	VectorCopy(best_dir, direction);
	return true;
}

static qboolean fixbot_plasma_valid_target(edict_t *self, edict_t *target)
{
	if (!G_EntExists(self->owner))
		return false;
	if (target == self->owner)
		return false;
	if (!G_ValidTargetEnt(self->owner, target, true))
		return false;
	if (OnSameTeam(self->owner, target))
		return false;
	if (!visible(self, target))
		return false;
	return true;
}

static void fixbot_plasma_turn_toward(edict_t *self, edict_t *target)
{
	vec3_t dir;
	float turn_fraction;

	if (!fixbot_plasma_valid_target(self, target))
		return;

	VectorSubtract(target->s.origin, self->s.origin, dir);
	if (!VectorNormalize(dir))
		return;

	turn_fraction = self->accel;
	if (turn_fraction < 0)
		turn_fraction = 0;
	else if (turn_fraction > 1)
		turn_fraction = 1;

	VectorScale(self->movedir, 1.0f - turn_fraction, self->movedir);
	VectorMA(self->movedir, turn_fraction, dir, self->movedir);
	VectorNormalize(self->movedir);
	vectoangles(self->movedir, self->s.angles);
	self->enemy = target;
}

static edict_t *fixbot_plasma_acquire_target(edict_t *self)
{
	edict_t *target = NULL;
	edict_t *acquire = NULL;
	vec3_t forward;
	vec3_t dir;
	float best_dot = -1.0f;
	float best_dist = 0.0f;

	if (fixbot_plasma_valid_target(self, self->enemy))
		return self->enemy;
	self->enemy = NULL;

	AngleVectors(self->s.angles, forward, NULL, NULL);

	while ((target = findradius(target, self->s.origin, 1024)) != NULL)
	{
		float dot;
		float dist;

		if (!fixbot_plasma_valid_target(self, target))
			continue;

		VectorSubtract(target->s.origin, self->s.origin, dir);
		dist = VectorNormalize(dir);
		dot = DotProduct(dir, forward);

		if (!acquire || dot > best_dot || (dot == best_dot && dist < best_dist))
		{
			acquire = target;
			best_dot = dot;
			best_dist = dist;
		}
	}

	return acquire;
}

static void fixbot_plasma_think(edict_t *self)
{
	edict_t *target;
	vec3_t upward;

	if (!G_EntExists(self->owner) || level.time >= self->delay)
	{
		BecomeExplosion1(self);
		return;
	}

	if (self->timestamp > level.time)
	{
		VectorSet(upward, 0, 0, self->movedir[2] > 0.3f ? 0.10f : 0.02f);
		VectorAdd(self->movedir, upward, self->movedir);
		VectorNormalize(self->movedir);
		vectoangles(self->movedir, self->s.angles);
		VectorScale(self->movedir, self->speed, self->velocity);
		if (random() > 0.5f)
		{
			gi.WriteByte(svc_temp_entity);
			gi.WriteByte(TE_BLASTER);
			gi.WritePosition(self->s.origin);
			gi.WriteDir(vec3_origin);
			gi.multicast(self->s.origin, MULTICAST_PVS);
		}
		self->nextthink = level.time + FRAMETIME;
		return;
	}

	target = fixbot_plasma_acquire_target(self);
	if (target)
		fixbot_plasma_turn_toward(self, target);

	VectorScale(self->movedir, self->speed, self->velocity);
	self->nextthink = level.time + FRAMETIME;
}

static void fixbot_plasma_touch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t origin;
	vec3_t normal;

	if (other == ent->owner)
		return;

	if (!G_EntExists(ent->owner))
	{
		G_FreeEdict(ent);
		return;
	}

	if (surf && (surf->flags & SURF_SKY))
	{
		ent->timestamp = 0;
		if (plane)
			VectorMA(ent->s.origin, 16.0f, plane->normal, ent->s.origin);
		if (ent->movedir[2] > 0)
		{
			ent->movedir[2] *= -0.35f;
			VectorNormalize(ent->movedir);
		}
		if (fixbot_plasma_valid_target(ent, ent->enemy))
			fixbot_plasma_turn_toward(ent, ent->enemy);
		ent->nextthink = level.time + FRAMETIME;
		return;
	}

	VectorMA(ent->s.origin, -0.02f, ent->velocity, origin);
	if (plane)
		VectorCopy(plane->normal, normal);
	else
		VectorClear(normal);

	if (other->takedamage)
		T_Damage(other, ent, ent->owner, ent->velocity, ent->s.origin, normal, ent->dmg, 0, DAMAGE_ENERGY, MOD_PHALANX);

	T_RadiusDamage(ent, ent->owner, ent->radius_dmg, other, ent->dmg_radius, MOD_PHALANX);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_PLASMA_EXPLOSION);
	gi.WritePosition(origin);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	G_FreeEdict(ent);
}

static void fixbot_fire_plasma_shot(edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float radius, int radius_damage, float turn_fraction)
{
	edict_t *plasma;

	damage = vrx_increase_monster_damage_by_talent(self->activator, damage);
	if (speed < 1)
		speed = 1;

	VectorNormalize(dir);
	self->lastsound = level.framenum;

	plasma = G_Spawn();
	VectorCopy(start, plasma->s.origin);
	VectorCopy(start, plasma->s.old_origin);
	VectorCopy(dir, plasma->movedir);
	vectoangles(dir, plasma->s.angles);
	VectorScale(dir, speed, plasma->velocity);
	plasma->movetype = MOVETYPE_FLYMISSILE;
	plasma->clipmask = MASK_SHOT;
	plasma->solid = SOLID_BBOX;
	VectorSet(plasma->mins, -5, -5, -5);
	VectorSet(plasma->maxs, 5, 5, 5);
	plasma->owner = self;
	plasma->touch = fixbot_plasma_touch;
	plasma->think = fixbot_plasma_think;
	plasma->nextthink = level.time + FRAMETIME;
	plasma->speed = speed;
	plasma->accel = turn_fraction;
	plasma->dmg = damage;
	plasma->radius_dmg = radius_damage;
	plasma->dmg_radius = radius;
	plasma->s.sound = gi.soundindex("weapons/rockfly.wav");
	plasma->s.modelindex = gi.modelindex("sprites/s_photon.sp2");
	plasma->s.effects |= EF_PLASMA | EF_ANIM_ALLFAST;
	plasma->s.scale = 0.75f;
	plasma->svflags |= SVF_PROJECTILE;
	plasma->timestamp = level.time + (fixbot_is_boss(self) ? 0.7f : 1.0f);
	plasma->delay = level.time + 8.0f;

	if (G_ValidTarget(self, self->enemy, true, true))
		plasma->enemy = self->enemy;

	gi.linkentity(plasma);
}

static qboolean fixbot_has_open_sky(edict_t *self)
{
	vec3_t start;
	vec3_t end;
	trace_t tr;

	VectorCopy(self->s.origin, start);
	start[2] += 20.0f;
	VectorCopy(start, end);
	end[2] += 2000.0f;

	tr = gi.trace(start, NULL, NULL, end, self, MASK_SOLID);
	return tr.surface && (tr.surface->flags & SURF_SKY);
}

static void fixbot_fire_ionripper_spread(edict_t *self, vec3_t start, vec3_t forward, vec3_t right)
{
	int damage;
	int speed;
	int shots = fixbot_is_boss(self) ? 5 : 3;
	vec3_t target;
	vec3_t dir;

	if (!G_ValidTarget(self, self->enemy, true, true))
		return;

	damage = M_IONRIPPER_DMG_BASE + M_IONRIPPER_DMG_ADDON * drone_damagelevel(self);
	if (M_IONRIPPER_DMG_MAX && damage > M_IONRIPPER_DMG_MAX)
		damage = M_IONRIPPER_DMG_MAX;
	if (damage <= 0)
		damage = fixbot_is_boss(self) ? 20 : 12;

	speed = M_IONRIPPER_SPEED_BASE + M_IONRIPPER_SPEED_ADDON * drone_damagelevel(self);
	if (M_IONRIPPER_SPEED_MAX && speed > M_IONRIPPER_SPEED_MAX)
		speed = M_IONRIPPER_SPEED_MAX;
	if (speed <= 0)
		speed = fixbot_is_boss(self) ? 750 : 650;

	G_EntMidPoint(self->enemy, target);
	VectorSubtract(target, start, dir);
	if (!VectorNormalize(dir))
		VectorCopy(forward, dir);

	if (shots == 5)
	{
		vec3_t dir1, dir2, dir3, dir4, dir5;

		VectorCopy(dir, dir1);
		VectorMA(dir1, 0.16f, right, dir1);
		VectorNormalize(dir1);
		VectorCopy(dir, dir2);
		VectorMA(dir2, 0.12f, right, dir2);
		VectorNormalize(dir2);
		VectorCopy(dir, dir3);
		VectorCopy(dir, dir4);
		VectorMA(dir4, -0.12f, right, dir4);
		VectorNormalize(dir4);
		VectorCopy(dir, dir5);
		VectorMA(dir5, -0.16f, right, dir5);
		VectorNormalize(dir5);

		monster_fire_ionripper(self, start, dir1, damage, speed, EF_IONRIPPER, -1);
		monster_fire_ionripper(self, start, dir2, damage, speed, EF_IONRIPPER, -1);
		monster_fire_ionripper(self, start, dir3, damage, speed, EF_IONRIPPER, -1);
		monster_fire_ionripper(self, start, dir4, damage, speed, EF_IONRIPPER, -1);
		monster_fire_ionripper(self, start, dir5, damage, speed, EF_IONRIPPER, -1);
	}
	else
	{
		vec3_t dir1, dir2, dir3;

		VectorCopy(dir, dir1);
		VectorMA(dir1, 0.12f, right, dir1);
		VectorNormalize(dir1);
		VectorCopy(dir, dir2);
		VectorCopy(dir, dir3);
		VectorMA(dir3, -0.12f, right, dir3);
		VectorNormalize(dir3);

		monster_fire_ionripper(self, start, dir1, damage, speed, EF_IONRIPPER, -1);
		monster_fire_ionripper(self, start, dir2, damage, speed, EF_IONRIPPER, -1);
		monster_fire_ionripper(self, start, dir3, damage, speed, EF_IONRIPPER, -1);
	}

	gi.sound(self, CHAN_WEAPON, sound_ionripper ? sound_ionripper : sound_pew, 1, ATTN_NORM, 0);
}

static void fixbot_fire_plasma(edict_t *self, float offset)
{
	int damage;
	int speed;
	int radius_damage;
	float turn_fraction;
	vec3_t forward;
	vec3_t right;
	vec3_t up;
	vec3_t start;
	vec3_t dir;

	if (!G_ValidTarget(self, self->enemy, true, true))
		return;

	AngleVectors(self->s.angles, forward, right, up);
	VectorCopy(self->s.origin, start);
	VectorMA(start, 25.0f, forward, start);
	VectorMA(start, offset, right, start);
	VectorMA(start, 50.0f, up, start);

	if (!fixbot_has_open_sky(self))
	{
		fixbot_fire_ionripper_spread(self, start, forward, right);
		return;
	}

	VectorCopy(forward, dir);
	VectorMA(dir, 0.55f, up, dir);
	VectorNormalize(dir);

	damage = (fixbot_is_boss(self) ? 24 : 20) + (fixbot_is_boss(self) ? 4 : 3) * drone_damagelevel(self);
	if (damage > (fixbot_is_boss(self) ? 120 : 90))
		damage = fixbot_is_boss(self) ? 120 : 90;

	speed = fixbot_is_boss(self) ? (520 + (int)(random() * 120.0f)) : (380 + (int)(random() * 100.0f));
	radius_damage = (int)(damage * 1.75f);
	turn_fraction = fixbot_is_boss(self) ? 0.085f : 0.065f;

	if (fixbot_is_boss(self))
	{
		vec3_t start1, start2, start3;
		vec3_t dir1, dir2, dir3;

		VectorCopy(start, start1);
		VectorMA(start1, 25.0f, right, start1);
		VectorMA(start1, 15.0f, up, start1);
		VectorCopy(start, start2);
		VectorCopy(start, start3);
		VectorMA(start3, -25.0f, right, start3);
		VectorMA(start3, 15.0f, up, start3);

		VectorCopy(dir, dir1);
		VectorMA(dir1, 0.10f, right, dir1);
		VectorMA(dir1, 0.05f, up, dir1);
		VectorNormalize(dir1);
		VectorCopy(dir, dir2);
		VectorCopy(dir, dir3);
		VectorMA(dir3, -0.10f, right, dir3);
		VectorMA(dir3, 0.05f, up, dir3);
		VectorNormalize(dir3);

		fixbot_fire_plasma_shot(self, start1, dir1, damage, speed, 150.0f, radius_damage, turn_fraction);
		fixbot_fire_plasma_shot(self, start2, dir2, damage, speed, 150.0f, radius_damage, turn_fraction);
		fixbot_fire_plasma_shot(self, start3, dir3, damage, speed, 150.0f, radius_damage, turn_fraction);
	}
	else
	{
		fixbot_fire_plasma_shot(self, start, dir, damage, speed, 150.0f, radius_damage, turn_fraction);
	}

	gi.sound(self, CHAN_WEAPON, sound_pew, 1, ATTN_NORM, 0);
}

static void fixbot_fire_blaster(edict_t *self)
{
	int damage;
	int speed = 1000;
	int effect;
	vec3_t forward;
	vec3_t base_forward;
	vec3_t right;
	vec3_t start;

	if (!G_ValidTarget(self, self->enemy, true, true))
		return;

	damage = (fixbot_is_boss(self) ? 10 : 7) + (fixbot_is_boss(self) ? 3 : 2) * self->monsterinfo.level;
	if (damage > (fixbot_is_boss(self) ? 80 : 50))
		damage = fixbot_is_boss(self) ? 80 : 50;

	effect = (self->s.frame & 3) ? 0 : EF_BLASTER;
	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, monster_flash_offset[FIXBOT_BLASTER_FLASH], forward, right, start);
	MonsterAim(self, M_PROJECTILE_ACC, speed, false, -1, forward, start);
	if (!M_MonsterHasClearShotFrom(self, start))
	{
		M_MonsterBlockedShot(self, 0.35f);
		return;
	}
	VectorCopy(forward, base_forward);
	monster_fire_blaster(self, start, forward, damage, speed, effect, BLASTER_PROJ_BOLT, 2.0f, false, FIXBOT_BLASTER_FLASH);

	if (fixbot_is_boss(self) && (self->s.frame & 1))
	{
		VectorCopy(base_forward, forward);
		VectorMA(forward, 0.10f, right, forward);
		VectorNormalize(forward);
		monster_fire_blaster(self, start, forward, damage, speed, 0, BLASTER_PROJ_BOLT, 2.0f, false, FIXBOT_BLASTER_FLASH);

		VectorCopy(base_forward, forward);
		VectorMA(forward, -0.10f, right, forward);
		VectorNormalize(forward);
		monster_fire_blaster(self, start, forward, damage, speed, 0, BLASTER_PROJ_BOLT, 2.0f, false, FIXBOT_BLASTER_FLASH);
	}

	gi.sound(self, CHAN_WEAPON, sound_pew, 1, ATTN_NORM, 0);
}

static void fixbot_reattack(edict_t *self)
{
	if (G_ValidTarget(self, self->enemy, true, true))
	{
		if (random() < (fixbot_is_boss(self) ? 0.16f : 0.12f))
		{
			fixbot_fire_plasma(self, 0.0f);
			self->monsterinfo.nextframe = FIXBOT_FRAME_charging_27;
			return;
		}

		if (random() < (fixbot_is_boss(self) ? 0.80f : 0.55f))
		{
			self->monsterinfo.nextframe = FIXBOT_FRAME_charging_27;
			return;
		}
	}

	M_DelayNextAttack(self, fixbot_is_boss(self) ? 0.4f : 0.8f, true);
}

static void fixbot_update_spawn_probe(edict_t *self)
{
	static const float scan_yaws[] = { 0.0f, -35.0f, 35.0f, -70.0f, 70.0f, -110.0f, 110.0f, -150.0f, 150.0f, 180.0f };
	vec3_t start;
	vec3_t angles;
	vec3_t forward;
	vec3_t end;
	trace_t tr;
	int index;

	VectorCopy(self->s.origin, start);
	start[2] += 16.0f;

	index = (self->s.frame - FIXBOT_FRAME_weldstart_01) % (int)(sizeof(scan_yaws) / sizeof(scan_yaws[0]));
	if (index < 0)
		index = 0;

	VectorCopy(self->s.angles, angles);
	if (self->angle > FIXBOT_NO_SPAWN_YAW + 1.0f)
		angles[YAW] = self->angle;
	angles[YAW] += scan_yaws[index];
	angles[PITCH] = -10.0f;
	while (angles[YAW] < 0)
		angles[YAW] += 360;
	while (angles[YAW] >= 360)
		angles[YAW] -= 360;

	AngleVectors(angles, forward, NULL, NULL);
	VectorMA(start, 1000.0f, forward, end);
	tr = gi.trace(start, NULL, NULL, end, self, MASK_SOLID);

	if (tr.fraction < 1.0f)
		VectorCopy(tr.endpos, self->pos2);
	else
		VectorCopy(end, self->pos2);
}

static qboolean fixbot_try_select_turret_position(edict_t *self)
{
	if (fixbot_find_turret_spawn_position(self, self->pos1, self->pos2))
		return true;

	VectorClear(self->pos1);
	fixbot_update_spawn_probe(self);
	return false;
}

static qboolean fixbot_get_spawn_target(edict_t *self, vec3_t target)
{
	if (VectorLength(self->pos1) >= 1)
		VectorCopy(self->pos1, target);
	else
		VectorCopy(self->pos2, target);

	return VectorLength(target) >= 1;
}

static float fixbot_turn_angle(float current, float ideal, float speed)
{
	float move;
	float step;

	current = anglemod(current);
	ideal = anglemod(ideal);
	if (current == ideal)
		return current;

	move = ideal - current;
	if (ideal > current)
	{
		if (move >= 180.0f)
			move -= 360.0f;
	}
	else
	{
		if (move <= -180.0f)
			move += 360.0f;
	}

	step = speed * FRAMETIME * 10.0f;
	if (move > step)
		move = step;
	else if (move < -step)
		move = -step;

	return anglemod(current + move);
}

static qboolean fixbot_facing_spawn_target(edict_t *self)
{
	vec3_t dir;
	vec3_t angles;
	vec3_t target;

	if (!fixbot_get_spawn_target(self, target))
		return false;

	VectorSubtract(target, self->s.origin, dir);
	if (VectorLength(dir) <= 1)
		return false;

	vectoangles(dir, angles);
	return fixbot_angle_delta(self->s.angles[YAW], angles[YAW]) <= FIXBOT_SPAWN_AIM_EPSILON;
}

static qboolean fixbot_aim_at_spawn_target(edict_t *self)
{
	vec3_t dir;
	vec3_t angles;
	vec3_t target;
	float old_yaw_speed;
	qboolean building;

	if (!fixbot_get_spawn_target(self, target))
		return false;

	VectorSubtract(target, self->s.origin, dir);
	if (VectorLength(dir) <= 1)
		return false;

	vectoangles(dir, angles);
	building = self->monsterinfo.currentmove == &fixbot_move_spawn;
	self->ideal_yaw = angles[YAW];
	if (building)
		self->s.angles[PITCH] = fixbot_turn_angle(self->s.angles[PITCH], angles[PITCH], FIXBOT_SPAWN_PITCH_SPEED);
	else
		self->s.angles[PITCH] = angles[PITCH];
	old_yaw_speed = self->yaw_speed;
	if (building)
		self->yaw_speed = FIXBOT_SPAWN_YAW_SPEED;
	M_ChangeYaw(self);
	self->yaw_speed = old_yaw_speed;
	return true;
}

static void fixbot_prep_spawn(edict_t *self)
{
	VectorClear(self->pos1);
	VectorClear(self->pos2);
	self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	self->angle = FIXBOT_NO_SPAWN_YAW;
	self->teleport_time = level.time;

	if (!fixbot_is_boss(self) || level.time < self->monsterinfo.melee_finished)
	{
		self->monsterinfo.currentmove = NULL;
		fixbot_run(self);
		return;
	}

	if (fixbot_count_live_turrets(self) >= FIXBOT_BOSS_TURRET_MAX)
	{
		self->monsterinfo.melee_finished = level.time + FIXBOT_BOSS_FAIL_COOLDOWN;
		self->monsterinfo.currentmove = NULL;
		fixbot_run(self);
		return;
	}

	if (fixbot_set_spawn_base_yaw(self))
		fixbot_set_spawn_yaw(self);
	fixbot_try_select_turret_position(self);
	fixbot_aim_at_spawn_target(self);
	self->s.effects |= EF_HYPERBLASTER | EF_PLASMA;
	gi.sound(self, CHAN_WEAPON, sound_weld, 1, ATTN_NORM, 0);
}

static void fixbot_spawn_laser_off(edict_t *self)
{
	if (self->beam && self->beam->inuse)
		G_FreeEdict(self->beam);
	self->beam = NULL;
}

static void fixbot_fire_spawn_laser(edict_t *self)
{
	edict_t *laser;
	vec3_t forward;
	vec3_t start;
	vec3_t target;

	if (!fixbot_get_spawn_target(self, target))
		return;

	laser = self->beam;
	if (!laser || !laser->inuse)
	{
		laser = G_Spawn();
		if (!laser)
			return;
		self->beam = laser;
		laser->movetype = MOVETYPE_NONE;
		laser->solid = SOLID_NOT;
		laser->s.renderfx = RF_BEAM | RF_TRANSLUCENT;
		laser->s.modelindex = 1;
		laser->s.frame = 2;
		laser->owner = self;
		laser->classname = "fixbot_spawn_laser";
		laser->s.sound = gi.soundindex("misc/lasfly.wav");
	}

	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorMA(self->s.origin, 16.0f, forward, start);
	VectorCopy(start, laser->s.origin);
	VectorCopy(target, laser->s.old_origin);
	VectorCopy(start, laser->pos1);
	VectorCopy(target, laser->pos2);
	laser->s.skinnum = fixbot_is_boss(self) ? 0xf0f0f0f0 : 0xf2f2f0f0;
	laser->think = G_FreeEdict;
	laser->nextthink = level.time + 0.2f;
	gi.linkentity(laser);
}

static void fixbot_spawn_effect(edict_t *self)
{
	vec3_t target;
	qboolean has_position;

	has_position = VectorLength(self->pos1) >= 1;
	if (!has_position)
		has_position = fixbot_try_select_turret_position(self);

	if (!fixbot_get_spawn_target(self, target))
		return;

	fixbot_fire_spawn_laser(self);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_WELDING_SPARKS);
	gi.WriteByte(has_position ? (fixbot_is_boss(self) ? 18 : 10) : 5);
	gi.WritePosition(target);
	gi.WriteDir(vec3_origin);
	gi.WriteByte(has_position ? (fixbot_is_boss(self) ? 0xf0 : 0xe0) : 0xd0);
	gi.multicast(target, MULTICAST_PVS);
}

static void fixbot_spawn_aim_ai(edict_t *self, float dist)
{
	ai_move(self, dist);
	if (VectorLength(self->pos1) < 1)
		fixbot_try_select_turret_position(self);
	if (!fixbot_aim_at_spawn_target(self))
		fixbot_turn_to_spawn_yaw(self);
	fixbot_fire_spawn_laser(self);
}

static qboolean fixbot_spawn_turret(edict_t *self)
{
	edict_t *spawned;
	edict_t *summoner;
	vec3_t dir;
	vec3_t angles;

	if (VectorLength(self->pos1) < 1)
		return false;
	if (fixbot_count_live_turrets(self) >= FIXBOT_BOSS_TURRET_MAX)
		return false;

	spawned = G_Spawn();
	spawned->mtype = M_ROGUE_TURRET;
	spawned->activator = self;
	spawned->owner = self;
	spawned->monsterinfo.level = self->monsterinfo.level;

	summoner = G_GetSummoner(self);
	if (summoner)
		spawned->creator = summoner;

	if (!M_Initialize(self, spawned, 0.0f))
	{
		G_FreeEdict(spawned);
		return false;
	}

	spawned->activator = self;
	spawned->owner = self;
	if (summoner)
		spawned->creator = summoner;
	spawned->monsterinfo.control_cost = 0;
	spawned->monsterinfo.cost = 0;
	spawned->movetype = MOVETYPE_NONE;
	spawned->gravity = 0;
	spawned->health = spawned->max_health;
	M_SetMonsterArmorCurrent(spawned, M_MonsterArmorMax(spawned));
	spawned->monsterinfo.pausetime = 0;
	spawned->monsterinfo.attack_finished = level.time;
	spawned->monsterinfo.melee_finished = level.time;
	VectorCopy(self->pos1, spawned->s.origin);
	VectorCopy(self->pos1, spawned->s.old_origin);

	if (G_ValidTarget(self, self->enemy, false, true))
		VectorSubtract(self->enemy->s.origin, self->pos1, dir);
	else
		VectorSubtract(self->s.origin, self->pos1, dir);
	if (VectorLength(dir) < 1)
		VectorSet(dir, 1, 0, 0);
	vectoangles(dir, angles);
	VectorCopy(angles, spawned->s.angles);

	if (G_ValidTarget(spawned, self->enemy, false, true))
		spawned->enemy = self->enemy;

	gi.linkentity(spawned);
	rogue_turret_force_ready(spawned);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_TELEPORT_EFFECT);
	gi.WritePosition(self->pos1);
	gi.multicast(self->pos1, MULTICAST_PVS);
	if (sound_spawn)
		gi.sound(self, CHAN_AUTO, sound_spawn, 1, ATTN_NORM, 0);

	return true;
}

static void fixbot_finish_spawn(edict_t *self)
{
	if (VectorLength(self->pos1) < 1)
		fixbot_try_select_turret_position(self);

	fixbot_aim_at_spawn_target(self);
	fixbot_fire_spawn_laser(self);
	if (!fixbot_facing_spawn_target(self) && level.time < self->teleport_time)
	{
		self->monsterinfo.aiflags |= AI_HOLD_FRAME;
		return;
	}

	self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
	if (fixbot_spawn_turret(self))
		self->monsterinfo.melee_finished = level.time + FIXBOT_BOSS_SPAWN_COOLDOWN;
	else
		self->monsterinfo.melee_finished = level.time + FIXBOT_BOSS_FAIL_COOLDOWN;

	VectorClear(self->pos1);
	VectorClear(self->pos2);
	self->angle = FIXBOT_NO_SPAWN_YAW;
	fixbot_spawn_laser_off(self);
	self->s.effects &= ~(EF_HYPERBLASTER | EF_PLASMA);
}

static mframe_t fixbot_frames_stand[] =
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
	drone_ai_stand, 0, NULL
};
static mmove_t fixbot_move_stand = { FIXBOT_FRAME_ambient_01, FIXBOT_FRAME_ambient_19, fixbot_frames_stand, fixbot_run };

static void fixbot_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &fixbot_move_stand;
}

static mframe_t fixbot_frames_run[] =
{
	fixbot_ai_run, 10, fixbot_try_start_spawn
};
static mmove_t fixbot_move_run = { FIXBOT_FRAME_freeze_01, FIXBOT_FRAME_freeze_01, fixbot_frames_run, NULL };

static void fixbot_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &fixbot_move_stand;
	else
		self->monsterinfo.currentmove = &fixbot_move_run;
}

static mframe_t fixbot_frames_walk[] =
{
	fixbot_ai_walk, 5, NULL
};
static mmove_t fixbot_move_walk = { FIXBOT_FRAME_freeze_01, FIXBOT_FRAME_freeze_01, fixbot_frames_walk, NULL };

static void fixbot_walk(edict_t *self)
{
	if (!self->goalentity)
		self->goalentity = world;
	self->monsterinfo.currentmove = &fixbot_move_walk;
}

static mframe_t fixbot_frames_attack[] =
{
	ai_charge, 0, fixbot_fire_blaster,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, fixbot_fire_blaster,
	ai_charge, 0, NULL,
	ai_charge, 0, fixbot_fire_blaster,
	ai_charge, 0, NULL,
	ai_charge, 0, fixbot_fire_blaster,
	ai_charge, 0, fixbot_fire_blaster,
	ai_charge, -10, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, fixbot_fire_blaster,
	ai_charge, -10, NULL,
	ai_charge, 0, fixbot_fire_blaster,
	ai_charge, 0, NULL,
	ai_charge, -10, NULL,
	ai_charge, 0, fixbot_fire_blaster,
	ai_charge, 0, NULL,
	ai_charge, 0, fixbot_fire_blaster,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, fixbot_fire_blaster,
	ai_charge, 0, NULL,
	ai_charge, 0, fixbot_fire_blaster,
	ai_charge, 0, fixbot_fire_blaster,
	ai_charge, 0, fixbot_fire_blaster,
	ai_charge, 0, fixbot_fire_blaster,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, fixbot_reattack
};
static mmove_t fixbot_move_attack = { FIXBOT_FRAME_charging_01, FIXBOT_FRAME_charging_31, fixbot_frames_attack, fixbot_run };

static mframe_t fixbot_frames_spawn[] =
{
	ai_move, 0, fixbot_prep_spawn,
	fixbot_spawn_aim_ai, 0, fixbot_spawn_effect,
	fixbot_spawn_aim_ai, 0, fixbot_spawn_effect,
	fixbot_spawn_aim_ai, 0, fixbot_spawn_effect,
	fixbot_spawn_aim_ai, 0, fixbot_spawn_effect,
	ai_move, 0, fixbot_finish_spawn,
	ai_move, 0, NULL
};
static mmove_t fixbot_move_spawn = { FIXBOT_FRAME_weldstart_01, FIXBOT_FRAME_weldstart_07, fixbot_frames_spawn, fixbot_run };

static void fixbot_try_start_spawn(edict_t *self)
{
	if (!fixbot_is_boss(self))
		return;
	if (level.time < self->monsterinfo.melee_finished)
		return;
	if (fixbot_count_live_turrets(self) >= FIXBOT_BOSS_TURRET_MAX)
		return;
	if (!G_ValidTarget(self, self->enemy, false, true))
		return;

	self->monsterinfo.currentmove = &fixbot_move_spawn;
}

static void fixbot_attack(edict_t *self)
{
	if (!G_ValidTarget(self, self->enemy, false, true))
		return;

	if (fixbot_is_boss(self) && level.time >= self->monsterinfo.melee_finished
		&& fixbot_count_live_turrets(self) < FIXBOT_BOSS_TURRET_MAX)
	{
		self->monsterinfo.currentmove = &fixbot_move_spawn;
		return;
	}

	self->monsterinfo.lefty = random() <= 0.5f;
	self->monsterinfo.currentmove = &fixbot_move_attack;
}

static mframe_t fixbot_frames_paina[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
static mmove_t fixbot_move_paina = { FIXBOT_FRAME_paina_01, FIXBOT_FRAME_paina_06, fixbot_frames_paina, fixbot_run };

static mframe_t fixbot_frames_painb[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
static mmove_t fixbot_move_painb = { FIXBOT_FRAME_painb_01, FIXBOT_FRAME_painb_08, fixbot_frames_painb, fixbot_run };

static mframe_t fixbot_frames_pain3[] =
{
	ai_move, -1, NULL
};
static mmove_t fixbot_move_pain3 = { FIXBOT_FRAME_freeze_01, FIXBOT_FRAME_freeze_01, fixbot_frames_pain3, fixbot_run };

static void fixbot_dead(edict_t *self);

static mframe_t fixbot_frames_death1[] =
{
	ai_move, 0, NULL
};
static mmove_t fixbot_move_death1 = { FIXBOT_FRAME_freeze_01, FIXBOT_FRAME_freeze_01, fixbot_frames_death1, fixbot_dead };

static void fixbot_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3.0f;
	gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

	if (fixbot_is_boss(self) && self->monsterinfo.currentmove == &fixbot_move_spawn)
		return;

	if (damage <= 10)
		self->monsterinfo.currentmove = &fixbot_move_pain3;
	else if (damage <= 25)
		self->monsterinfo.currentmove = &fixbot_move_painb;
	else
		self->monsterinfo.currentmove = &fixbot_move_paina;
}

static void fixbot_dead(edict_t *self)
{
	fixbot_remove_turrets(self);
	fixbot_spawn_laser_off(self);

	vrx_throw_drone_gibs(self, self->dmg ? self->dmg : 120);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	M_Remove(self, false, false);
}

static void fixbot_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	M_Notify(self);

	if (self->deadflag == DEAD_DEAD)
		return;

	gi.sound(self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_NO;
	self->s.sound = 0;
	self->dmg = damage;
	self->monsterinfo.currentmove = &fixbot_move_death1;
}

static void init_drone_fixbot_common(edict_t *self, qboolean boss)
{
	sound_pain = gi.soundindex("daedalus/daedpain1.wav");
	sound_die = gi.soundindex("daedalus/daeddeth1.wav");
	sound_pew = gi.soundindex("makron/blaster.wav");
	sound_ionripper = gi.soundindex("weapons/rippfire.wav");
	sound_weld = gi.soundindex("misc/welder1.wav");
	sound_spawn = gi.soundindex("makron/popup.wav");
	gi.soundindex("misc/welder2.wav");
	gi.soundindex("misc/welder3.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/fixbot/tris.md2");

	if (boss)
	{
		if (invasion->value)
		{
			VectorSet(self->mins, -30, -30, -24);
			VectorSet(self->maxs, 30, 30, 24);
			self->s.scale = FIXBOT_BOSS_INVASION_SCALE;
		}
		else
		{
			VectorSet(self->mins, -36, -36, -28);
			VectorSet(self->maxs, 36, 36, 28);
			self->s.scale = FIXBOT_BOSS_DEFAULT_SCALE;
		}
		self->health = M_FIXBOT_BOSS_INITIAL_HEALTH + M_FIXBOT_BOSS_ADDON_HEALTH * self->monsterinfo.level;
		M_SetMonsterArmor(self, M_FIXBOT_BOSS_INITIAL_ARMOR + M_FIXBOT_BOSS_ADDON_ARMOR * self->monsterinfo.level);
		self->mass = 400;
		self->mtype = M_FIXBOT_BOSS;
		self->monsterinfo.control_cost = M_JORG_CONTROL_COST;
		self->monsterinfo.cost = M_COMMANDER_COST;
	}
	else
	{
		VectorSet(self->mins, -24, -24, -18);
		VectorSet(self->maxs, 24, 24, 24);
		self->health = M_FIXBOT_INITIAL_HEALTH + M_FIXBOT_ADDON_HEALTH * self->monsterinfo.level;
		M_SetMonsterArmor(self, M_FIXBOT_INITIAL_ARMOR + M_FIXBOT_ADDON_ARMOR * self->monsterinfo.level);
		self->mass = 150;
		self->s.scale = 1.55f;
		self->mtype = M_FIXBOT;
		self->monsterinfo.control_cost = M_HOVER_CONTROL_COST;
		self->monsterinfo.cost = M_HOVER_COST;
	}

	self->gib_health = -100;
	self->max_health = self->health;
	self->flags |= FL_FLY | FL_NO_KNOCKBACK;
	self->monsterinfo.aiflags |= AI_ALTERNATE_FLY;
	fixbot_set_fly_parameters(self);
	self->monsterinfo.sight_range = boss ? 1400 : 1024;
	self->monsterinfo.pain_chance = boss ? 0.08f : 0.18f;
	self->yaw_speed = boss ? 25 : 30;

	self->pain = fixbot_pain;
	self->die = fixbot_die;

	self->monsterinfo.stand = fixbot_stand;
	self->monsterinfo.walk = fixbot_walk;
	self->monsterinfo.run = fixbot_run;
	self->monsterinfo.attack = fixbot_attack;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &fixbot_move_stand;
	self->monsterinfo.scale = 1.0f;

	qboolean isBoss = (self->mtype == M_FIXBOT_BOSS);
	if (isBoss && !invasion->value)
		G_PrintGreenText(va("A level %d fixer has spawned!", self->monsterinfo.level));
}

void init_drone_fixbot(edict_t *self)
{
	init_drone_fixbot_common(self, false);
}

void init_drone_fixbot_boss(edict_t *self)
{
	init_drone_fixbot_common(self, true);
}
