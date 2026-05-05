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
#define WIDOW2_FRAME_dthsrh01		104
#define WIDOW2_FRAME_dthsrh15		118
#define WIDOW2_FRAME_dthsrh16		119
#define WIDOW2_FRAME_dthsrh22		125

#define WIDOW2_SUMMON_COUNT			2
#define WIDOW2_SUMMON_COOLDOWN		10.0f
#define WIDOW2_MELEE_RANGE			256.0f
#define WIDOW2_INVASION_SCALE		0.60f
#define WIDOW2_INVASION_MOVE_SCALE	1.75f

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
qboolean drone_findtarget(edict_t *self, qboolean force);

static void widow2_ai_walk(edict_t *self, float dist)
{
	drone_ai_walk(self, invasion->value ? dist * WIDOW2_INVASION_MOVE_SCALE : dist);
}

static void widow2_ai_run(edict_t *self, float dist)
{
	drone_ai_run(self, invasion->value ? dist * WIDOW2_INVASION_MOVE_SCALE : dist);
}

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
static void widow2_explosion1(edict_t *self);
static void widow2_explosion2(edict_t *self);
static void widow2_explosion3(edict_t *self);
static void widow2_explosion4(edict_t *self);
static void widow2_explosion5(edict_t *self);
static void widow2_explosion6(edict_t *self);
static void widow2_explosion7(edict_t *self);
static void widow2_explosion_leg(edict_t *self);
static void widow2_dead(edict_t *self);
static void widow2_start_searching(edict_t *self);
static void widow2_keep_searching(edict_t *self);
static void widow2_finaldeath(edict_t *self);
static void widow2_step(edict_t *self);

static mframe_t widow2_frames_stand[] =
{
	drone_ai_stand, 0, NULL
};
static mmove_t widow2_move_stand = { WIDOW2_FRAME_blackwidow3, WIDOW2_FRAME_blackwidow3, widow2_frames_stand, widow2_stand };

static mframe_t widow2_frames_walk[] =
{
	widow2_ai_walk, 9, widow2_step,
	widow2_ai_walk, 8, NULL,
	widow2_ai_walk, 7, NULL,
	widow2_ai_walk, 7, NULL,
	widow2_ai_walk, 6, NULL,
	widow2_ai_walk, 6, widow2_step,
	widow2_ai_walk, 7, NULL,
	widow2_ai_walk, 8, NULL,
	widow2_ai_walk, 10, NULL
};
static mmove_t widow2_move_walk = { WIDOW2_FRAME_walk01, WIDOW2_FRAME_walk09, widow2_frames_walk, widow2_walk };

static mframe_t widow2_frames_run[] =
{
	widow2_ai_run, 9, widow2_step,
	widow2_ai_run, 8, NULL,
	widow2_ai_run, 7, NULL,
	widow2_ai_run, 7, NULL,
	widow2_ai_run, 6, NULL,
	widow2_ai_run, 6, widow2_step,
	widow2_ai_run, 7, NULL,
	widow2_ai_run, 8, NULL,
	widow2_ai_run, 10, NULL
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
	ai_move, 0, widow2_explosion1,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, widow2_explosion2,
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
	ai_move, 0, widow2_explosion3,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, widow2_explosion4,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, widow2_explosion5,
	ai_move, 0, widow2_explosion_leg,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, widow2_explosion6,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, widow2_explosion7,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, widow2_dead
};
static mmove_t widow2_move_death = { WIDOW2_FRAME_death01, WIDOW2_FRAME_death44, widow2_frames_death, NULL };

static mframe_t widow2_frames_dead[] =
{
	ai_move, 0, widow2_start_searching,
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
	ai_move, 0, widow2_keep_searching
};
static mmove_t widow2_move_dead = { WIDOW2_FRAME_dthsrh01, WIDOW2_FRAME_dthsrh15, widow2_frames_dead, NULL };

static mframe_t widow2_frames_really_dead[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, widow2_finaldeath
};
static mmove_t widow2_move_really_dead = { WIDOW2_FRAME_dthsrh16, WIDOW2_FRAME_dthsrh22, widow2_frames_really_dead, NULL };

static void widow2_start_searching(edict_t *self)
{
	self->count = 0;
}

static void widow2_keep_searching(edict_t *self)
{
	if (self->count <= 2)
	{
		self->monsterinfo.currentmove = &widow2_move_dead;
		self->s.frame = WIDOW2_FRAME_dthsrh01;
		self->count++;
		return;
	}

	self->monsterinfo.currentmove = &widow2_move_really_dead;
}

static void widow2_finaldeath(edict_t *self)
{
	// M_Remove ignores SOLID_NOT entities; the corpse is kept nonsolid during
	// the final death animation so it does not block map paths.
	self->solid = SOLID_BBOX;
	M_Remove(self, false, false);
}

static void widow2_step(edict_t *self)
{
	gi.sound(self, CHAN_BODY, sound_step, 1, ATTN_NORM, 0);
}

static void widow2_search(edict_t *self)
{
	if (random() < 0.5f)
		gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
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

static void widow2_project_flash(edict_t *self, int flash, vec3_t forward, vec3_t start)
{
	vec3_t right, offset;

	AngleVectors(self->s.angles, forward, right, NULL);
	VectorCopy(monster_flash_offset[flash], offset);
	if (self->s.scale && self->s.scale != 1.0f)
		VectorScale(offset, self->s.scale, offset);
	G_ProjectSource(self->s.origin, offset, forward, right, start);
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

	widow2_project_flash(self, flash, forward, start);
	MonsterAim(self, M_HITSCAN_INSTANT_ACC, 0, false, -1, forward, start);
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

	widow2_project_flash(self, MZ2_WIDOW_DISRUPTOR, forward, start);
	MonsterAim(self, M_PROJECTILE_ACC, speed, true, -1, forward, start);
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
	if (self->s.scale && self->s.scale != 1.0f)
		VectorScale(offset, self->s.scale, offset);
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

static void widow2_prepare_pulled_enemy(edict_t *self)
{
	if (!G_EntExists(self->enemy))
		return;

	if (self->enemy->groundentity)
	{
		self->enemy->s.origin[2] += 1;
		self->enemy->groundentity = NULL;
	}
}

static int widow2_proboscis_damage(edict_t *self)
{
	int damage;

	damage = M_MELEE_DMG_BASE + M_MELEE_DMG_ADDON * drone_damagelevel(self);
	if (M_MELEE_DMG_MAX && damage > M_MELEE_DMG_MAX)
		damage = M_MELEE_DMG_MAX;

	damage = max(1, damage / 4);
	return vrx_increase_monster_damage_by_talent(self->activator, damage);
}

static int widow2_proboscis_pull(edict_t *self)
{
	int pull;

	pull = PARASITE_INITIAL_KNOCKBACK + PARASITE_ADDON_KNOCKBACK * drone_damagelevel(self);
	if (PARASITE_MAX_KNOCKBACK && pull < PARASITE_MAX_KNOCKBACK)
		pull = PARASITE_MAX_KNOCKBACK;
	if (G_EntExists(self->enemy) && self->enemy->groundentity)
		pull *= 2;

	return pull;
}

static void widow2_heal_from_proboscis(edict_t *self, int damage)
{
	if (self->health >= self->max_health)
		return;

	self->health += damage;
	if (self->health > self->max_health)
		self->health = self->max_health;
}

static void widow2_drain_proboscis(edict_t *self)
{
	int damage;
	int pull;
	vec3_t start, end, dir;

	if (!widow2_draw_proboscis(self, start, end))
		return;

	damage = widow2_proboscis_damage(self);
	pull = widow2_proboscis_pull(self);
	widow2_prepare_pulled_enemy(self);
	widow2_heal_from_proboscis(self, damage);

	if (self->s.frame == WIDOW2_FRAME_tongs01 + 3)
		gi.sound(self->enemy, CHAN_AUTO, sound_hit, 1, ATTN_NORM, 0);

	VectorSubtract(end, start, dir);
	T_Damage(self->enemy, self, self, dir, self->enemy->s.origin, end, damage, pull, DAMAGE_NO_ABILITIES, MOD_UNKNOWN);
}

static void widow2_show_proboscis(edict_t *self)
{
	vec3_t start, end;

	widow2_draw_proboscis(self, start, end);
}

static void widow2_pull_proboscis(edict_t *self)
{
	widow2_drain_proboscis(self);
}

static void widow2_melee_hit(edict_t *self)
{
	widow2_drain_proboscis(self);
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

static void widow2_setup_invasion_spawn(edict_t *spawned)
{
	if (!invasion->value)
		return;

	spawned->monsterinfo.aiflags &= ~AI_STAND_GROUND;
	spawned->monsterinfo.aiflags |= AI_FIND_NAVI;
	spawned->prev_navi = NULL;
	spawned->goalentity = NULL;
}

static void widow2_start_spawned_monster(edict_t *self, edict_t *spawned)
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
	widow2_setup_invasion_spawn(spawned);

	gi.linkentity(spawned);
	owner->num_monsters += spawned->monsterinfo.control_cost;
	owner->num_monsters_real++;

	widow2_start_spawned_monster(self, spawned);

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

static qboolean widow2_can_spawn_stalker(edict_t *self)
{
	vec3_t spot;

	for (int i = 0; i < WIDOW2_SUMMON_COUNT; i++)
	{
		if (widow2_find_spawn_spot(self, i, spot))
			return true;
	}

	return false;
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
	if (level.time >= self->monsterinfo.melee_finished && widow2_can_spawn_stalker(self))
		widow2_start_spawn(self);
	else if (widow2_can_melee(self) && r < 0.35f)
		widow2_melee(self);
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

	if (invasion->value != 2)
		self->monsterinfo.currentmove = &widow2_move_pain;
}

static void widow2_project_source2(vec3_t origin, vec3_t offset, vec3_t forward, vec3_t right, vec3_t up, vec3_t result)
{
	result[0] = origin[0] + forward[0] * offset[0] + right[0] * offset[1] + up[0] * offset[2];
	result[1] = origin[1] + forward[1] * offset[0] + right[1] * offset[1] + up[1] * offset[2];
	result[2] = origin[2] + forward[2] * offset[0] + right[2] * offset[1] + up[2] * offset[2];
}

static void widow2_throw_gib_at(edict_t *self, char *gibname, int damage, int type, vec3_t point)
{
	edict_t *gib;
	float scale;

	if (nolag->value || !vrx_spawn_nonessential_ent(point))
		return;

	scale = self->s.scale ? self->s.scale : 1.0f;
	gib = ThrowGibEx(self, gibname, damage, type, scale);
	if (!gib)
		return;

	VectorCopy(point, gib->s.origin);
	VectorCopy(point, gib->s.old_origin);
	gi.linkentity(gib);
}

static void widow2_explode_at(edict_t *self, vec3_t offset, int effect, vec3_t point)
{
	vec3_t forward, right, up;

	AngleVectors(self->s.angles, forward, right, up);
	widow2_project_source2(self->s.origin, offset, forward, right, up, point);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(effect);
	gi.WritePosition(point);
	gi.multicast(self->s.origin, MULTICAST_ALL);
}

static void widow2_throw_loose_gibs(edict_t *self, vec3_t point)
{
	widow2_throw_gib_at(self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_ORGANIC, point);
	widow2_throw_gib_at(self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, point);
	widow2_throw_gib_at(self, "models/objects/gibs/sm_metal/tris.md2", 300, GIB_METALLIC, point);
	widow2_throw_gib_at(self, "models/objects/gibs/sm_metal/tris.md2", 300, GIB_METALLIC, point);
}

static void widow2_explosion(edict_t *self, float x, float y, float z)
{
	vec3_t offset;
	vec3_t point;

	VectorSet(offset, x, y, z);
	widow2_explode_at(self, offset, TE_EXPLOSION1, point);
	widow2_throw_loose_gibs(self, point);
}

static void widow2_explosion1(edict_t *self)
{
	widow2_explosion(self, 23.74f, -37.67f, 76.96f);
}

static void widow2_explosion2(edict_t *self)
{
	widow2_explosion(self, -20.49f, 36.92f, 73.52f);
}

static void widow2_explosion3(edict_t *self)
{
	widow2_explosion(self, 2.11f, 0.05f, 92.20f);
}

static void widow2_explosion4(edict_t *self)
{
	widow2_explosion(self, -28.04f, -35.57f, -77.56f);
}

static void widow2_explosion5(edict_t *self)
{
	widow2_explosion(self, -20.11f, -1.11f, 40.76f);
}

static void widow2_explosion6(edict_t *self)
{
	widow2_explosion(self, -20.11f, -1.11f, 40.76f);
}

static void widow2_explosion7(edict_t *self)
{
	widow2_explosion(self, -20.11f, -1.11f, 40.76f);
}

static void widow2_explosion_leg(edict_t *self)
{
	vec3_t offset;
	vec3_t point;

	VectorSet(offset, -31.89f, -47.86f, 67.02f);
	widow2_explode_at(self, offset, TE_EXPLOSION1_BIG, point);
	widow2_throw_gib_at(self, "models/monsters/blackwidow2/gib2/tris.md2", 200, GIB_METALLIC | GIB_UPRIGHT, point);
	widow2_throw_gib_at(self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_ORGANIC, point);
	widow2_throw_gib_at(self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, point);

	VectorSet(offset, -44.9f, -82.14f, 54.72f);
	widow2_explode_at(self, offset, TE_EXPLOSION1, point);
	widow2_throw_gib_at(self, "models/monsters/blackwidow2/gib1/tris.md2", 300, GIB_METALLIC | GIB_UPRIGHT, point);
	widow2_throw_gib_at(self, "models/objects/gibs/sm_meat/tris.md2", 300, GIB_ORGANIC, point);
	widow2_throw_gib_at(self, "models/objects/gibs/sm_metal/tris.md2", 100, GIB_METALLIC, point);
}

static void widow2_dead(edict_t *self)
{
	vrx_throw_drone_gibs(self, 500);
	self->s.sound = 0;
	self->count = 0;
	self->solid = SOLID_NOT;
	self->takedamage = DAMAGE_NO;
	gi.linkentity(self);
	self->monsterinfo.currentmove = &widow2_move_dead;
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
	vrx_update_drone_death_skin(self);
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
	if (invasion->value)
	{
		self->s.scale = WIDOW2_INVASION_SCALE;
		VectorSet(self->mins, -40, -40, 0);
		VectorSet(self->maxs, 40, 40, 82);
	}

	gi.modelindex("models/items/spawngro3/tris.md2");
	gi.modelindex("models/monsters/stalker/tris.md2");
	gi.modelindex("models/objects/gibs/sm_metal/tris.md2");
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
	self->monsterinfo.idle = widow2_search;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &widow2_move_stand;
	self->monsterinfo.scale = 2.0f;
	self->nextthink = level.time + FRAMETIME;

	if (!invasion->value)
		G_PrintGreenText(va("A level %d widow2 has spawned!", self->monsterinfo.level));
}
