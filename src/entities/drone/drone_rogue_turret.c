/*
==============================================================================

rogue rocket turret

==============================================================================
*/

#include "g_local.h"

#define TURRET_FRAME_stand01	0
#define TURRET_FRAME_stand02	1
#define TURRET_FRAME_active01	2
#define TURRET_FRAME_run01	8
#define TURRET_FRAME_run02	9
#define TURRET_FRAME_pow01	10
#define TURRET_FRAME_pow04	13

#define TURRET_ROCKET_DAMAGE	40
#define TURRET_ROCKET_SPEED		650

static int sound_moved;
static int sound_moving;

void drone_ai_stand(edict_t *self, float dist);

static void rogue_turret_stand(edict_t *self);
static void rogue_turret_run(edict_t *self);
static void rogue_turret_active(edict_t *self);

static void rogue_turret_laser_off(edict_t *self)
{
	if (self->target_ent && self->target_ent->inuse)
		G_FreeEdict(self->target_ent);
	self->target_ent = NULL;
}

static void rogue_turret_update_laser(edict_t *self)
{
	vec3_t forward;
	vec3_t end;
	trace_t tr;
	edict_t *laser;
	float scan_range;
	float phase;

	if (!G_ValidTarget(self, self->enemy, false, true))
	{
		rogue_turret_laser_off(self);
		return;
	}

	laser = self->target_ent;
	if (!laser || !laser->inuse)
	{
		laser = G_Spawn();
		if (!laser)
			return;
		self->target_ent = laser;
		laser->movetype = MOVETYPE_NONE;
		laser->solid = SOLID_NOT;
		laser->s.renderfx = RF_BEAM | RF_TRANSLUCENT;
		laser->s.modelindex = 1;
		laser->s.frame = 1;
		laser->s.skinnum = 0xf2f2f0f0;
		laser->classname = "turret_lasersight";
		laser->owner = self;
	}

	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorMA(self->s.origin, 8192, forward, end);
	tr = gi.trace(self->s.origin, NULL, NULL, end, self, MASK_SOLID);

	scan_range = visible(self, self->enemy) ? 12.0f : 64.0f;
	phase = level.time + (float)(self - g_edicts);
	tr.endpos[0] += sinf(phase) * scan_range;
	tr.endpos[1] += cosf(phase * 3.0f) * scan_range;
	tr.endpos[2] += sinf(phase * 2.5f) * scan_range;

	VectorSubtract(tr.endpos, self->s.origin, forward);
	if (VectorNormalize(forward))
	{
		VectorMA(self->s.origin, 8192, forward, end);
		tr = gi.trace(self->s.origin, NULL, NULL, end, self, MASK_SOLID);
	}

	VectorCopy(self->s.origin, laser->s.origin);
	VectorCopy(tr.endpos, laser->s.old_origin);
	VectorCopy(self->s.origin, laser->pos1);
	VectorCopy(tr.endpos, laser->pos2);
	laser->think = G_FreeEdict;
	laser->nextthink = level.time + 0.3f;
	gi.linkentity(laser);
}

static void rogue_turret_aim(edict_t *self)
{
	vec3_t dir;
	vec3_t angles;

	if (!G_ValidTarget(self, self->enemy, false, true))
	{
		rogue_turret_laser_off(self);
		return;
	}

	VectorSubtract(self->enemy->s.origin, self->s.origin, dir);
	if (VectorLength(dir) < 1)
	{
		rogue_turret_laser_off(self);
		return;
	}

	vectoangles(dir, angles);
	self->ideal_yaw = angles[YAW];
	M_ChangeYaw(self);
	self->s.angles[PITCH] = angles[PITCH];
	rogue_turret_update_laser(self);
}

static void rogue_turret_aim_stand_ai(edict_t *self, float dist)
{
	drone_ai_stand(self, dist);
	rogue_turret_aim(self);
}

static void rogue_turret_aim_move_ai(edict_t *self, float dist)
{
	ai_move(self, dist);
	rogue_turret_aim(self);
}

static void rogue_turret_aim_charge_ai(edict_t *self, float dist)
{
	ai_charge(self, dist);
	rogue_turret_aim(self);
}

static void rogue_turret_fire(edict_t *self)
{
	vec3_t forward;
	vec3_t aim;
	vec3_t right;
	vec3_t start;
	vec3_t end;
	trace_t tr;

	if (!G_ValidTarget(self, self->enemy, false, true))
	{
		rogue_turret_laser_off(self);
		return;
	}

	rogue_turret_aim(self);

	if (entdist(self, self->enemy) <= 72)
		return;

	G_EntMidPoint(self->enemy, end);
	AngleVectors(self->s.angles, forward, right, NULL);
	G_ProjectSource(self->s.origin, monster_flash_offset[MZ2_TURRET_ROCKET], forward, right, start);
	VectorSubtract(end, start, aim);
	if (!VectorNormalize(aim))
		return;

	if (DotProduct(aim, forward) < 0.90f)
		return;

	tr = gi.trace(start, NULL, NULL, end, self, MASK_SHOT);
	if (tr.fraction < 1.0 && tr.ent != self->enemy && !G_ValidTarget(self, tr.ent, false, true))
		return;

	monster_fire_rocket(self, start, aim, TURRET_ROCKET_DAMAGE, TURRET_ROCKET_SPEED, MZ2_TURRET_ROCKET);
}

static mframe_t rogue_turret_frames_stand[] =
{
	rogue_turret_aim_stand_ai, 0, NULL,
	rogue_turret_aim_stand_ai, 0, NULL
};
static mmove_t rogue_turret_move_stand = { TURRET_FRAME_stand01, TURRET_FRAME_stand02, rogue_turret_frames_stand, NULL };

static void rogue_turret_stand(edict_t *self)
{
	if (!G_ValidTarget(self, self->enemy, false, true))
		rogue_turret_laser_off(self);
	self->monsterinfo.currentmove = &rogue_turret_move_stand;
}

static mframe_t rogue_turret_frames_ready[] =
{
	rogue_turret_aim_move_ai, 0, NULL,
	rogue_turret_aim_move_ai, 0, NULL,
	rogue_turret_aim_move_ai, 0, NULL,
	rogue_turret_aim_move_ai, 0, NULL,
	rogue_turret_aim_move_ai, 0, NULL,
	rogue_turret_aim_move_ai, 0, NULL,
	rogue_turret_aim_move_ai, 0, NULL
};
static mmove_t rogue_turret_move_ready = { TURRET_FRAME_active01, TURRET_FRAME_run01, rogue_turret_frames_ready, rogue_turret_run };

static void rogue_turret_ready(edict_t *self)
{
	if (self->monsterinfo.currentmove != &rogue_turret_move_ready)
	{
		self->monsterinfo.currentmove = &rogue_turret_move_ready;
		self->random = 1;
		gi.sound(self, CHAN_WEAPON, sound_moving, 1, ATTN_NORM, 0);
	}
}

static mframe_t rogue_turret_frames_run[] =
{
	rogue_turret_aim_stand_ai, 0, rogue_turret_active,
	rogue_turret_aim_stand_ai, 0, rogue_turret_active
};
static mmove_t rogue_turret_move_run = { TURRET_FRAME_run01, TURRET_FRAME_run02, rogue_turret_frames_run, rogue_turret_run };

static void rogue_turret_run(edict_t *self)
{
	if (self->s.frame < TURRET_FRAME_run01)
	{
		rogue_turret_ready(self);
		return;
	}

	if (self->random)
	{
		self->random = 0;
		gi.sound(self, CHAN_WEAPON, sound_moved, 1, ATTN_NORM, 0);
	}

	self->monsterinfo.currentmove = &rogue_turret_move_run;
}

static mframe_t rogue_turret_frames_fire[] =
{
	rogue_turret_aim_charge_ai, 0, NULL,
	rogue_turret_aim_charge_ai, 0, rogue_turret_fire,
	rogue_turret_aim_charge_ai, 0, NULL,
	rogue_turret_aim_charge_ai, 0, NULL
};
static mmove_t rogue_turret_move_fire = { TURRET_FRAME_pow01, TURRET_FRAME_pow04, rogue_turret_frames_fire, rogue_turret_run };

static void rogue_turret_active(edict_t *self)
{
	rogue_turret_aim(self);

	if (!G_ValidTarget(self, self->enemy, false, true))
		return;
	if (level.time < self->monsterinfo.attack_finished)
		return;
	if (entdist(self, self->enemy) <= 72)
	{
		M_DelayNextAttack(self, 0.3f, false);
		return;
	}

	self->monsterinfo.currentmove = &rogue_turret_move_fire;
	M_DelayNextAttack(self, 0.8f + random() * 0.4f, true);
}

static void rogue_turret_attack(edict_t *self)
{
	if (!G_ValidTarget(self, self->enemy, false, true))
	{
		rogue_turret_laser_off(self);
		return;
	}

	rogue_turret_aim(self);
	if (self->s.frame < TURRET_FRAME_run01)
		rogue_turret_ready(self);
	else
		self->monsterinfo.currentmove = &rogue_turret_move_fire;

	M_DelayNextAttack(self, 1.0f + random() * 0.5f, true);
}

static void rogue_turret_pain(edict_t *self, edict_t *other, float kick, int damage)
{
}

void rogue_turret_force_ready(edict_t *self)
{
	self->s.frame = TURRET_FRAME_run01;
	self->monsterinfo.currentmove = &rogue_turret_move_run;
	self->monsterinfo.attack_finished = level.time;
	self->monsterinfo.pausetime = 0;
	self->nextthink = level.time + FRAMETIME;

	if (G_ValidTarget(self, self->enemy, false, true))
		rogue_turret_aim(self);
	else
		rogue_turret_laser_off(self);
}

static void rogue_turret_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	rogue_turret_laser_off(self);

	vrx_throw_drone_gibs(self, damage ? damage : 150);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	M_Remove(self, false, false);
}

void init_drone_rogue_turret(edict_t *self)
{
	sound_moved = gi.soundindex("turret/moved.wav");
	sound_moving = gi.soundindex("turret/moving.wav");
	gi.soundindex("weapons/rockfly.wav");
	gi.soundindex("chick/chkatck2.wav");
	gi.modelindex("models/objects/rocket/tris.md2");
	gi.modelindex("models/objects/debris1/tris.md2");

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/turret/tris.md2");
	self->s.skinnum = 2;
	VectorSet(self->mins, -12, -12, -12);
	VectorSet(self->maxs, 12, 12, 12);

	self->health = M_ROGUE_TURRET_INITIAL_HEALTH + M_ROGUE_TURRET_ADDON_HEALTH * self->monsterinfo.level;
	self->gib_health = -100;
	self->mass = 250;
	self->mtype = M_ROGUE_TURRET;
	self->flags |= FL_NO_KNOCKBACK;
	self->max_health = self->health;
	self->monsterinfo.power_armor_power = M_ROGUE_TURRET_INITIAL_ARMOR + M_ROGUE_TURRET_ADDON_ARMOR * self->monsterinfo.level;
	self->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;
	self->monsterinfo.max_armor = self->monsterinfo.power_armor_power;
	self->monsterinfo.control_cost = M_DEFAULT_CONTROL_COST;
	self->monsterinfo.cost = M_DEFAULT_COST;
	self->monsterinfo.sight_range = 1024;
	self->monsterinfo.aiflags |= AI_STAND_GROUND;
	self->monsterinfo.pain_chance = 0.0f;
	self->gravity = 0;
	self->yaw_speed = 40;

	self->pain = rogue_turret_pain;
	self->die = rogue_turret_die;

	self->monsterinfo.stand = rogue_turret_stand;
	self->monsterinfo.walk = rogue_turret_run;
	self->monsterinfo.run = rogue_turret_run;
	self->monsterinfo.attack = rogue_turret_attack;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &rogue_turret_move_stand;
	self->monsterinfo.scale = 3.5f;
}
