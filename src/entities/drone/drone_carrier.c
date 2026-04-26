/*
==============================================================================

carrier

==============================================================================
*/

#include "g_local.h"
#include "../../quake2/monsterframes/m_rogue_carrier.h"

#define CARRIER_SUMMON_COUNT		4
#define CARRIER_SUMMON_COOLDOWN		8.0f
#define CARRIER_DEFAULT_SCALE		0.75f
#define CARRIER_INVASION_SCALE		0.60f

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

static void carrier_stand(edict_t *self);
static void carrier_walk(edict_t *self);
static void carrier_run(edict_t *self);
static void carrier_attack(edict_t *self);

static void carrier_project_flash(edict_t *self, int flash, vec3_t forward, vec3_t start)
{
	vec3_t right, offset;

	AngleVectors(self->s.angles, forward, right, NULL);
	VectorCopy(monster_flash_offset[flash], offset);
	if (self->s.scale && self->s.scale != 1.0f)
		VectorScale(offset, self->s.scale, offset);
	G_ProjectSource(self->s.origin, offset, forward, right, start);
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

static void carrier_fire_rocket(edict_t *self)
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
		MonsterAim(self, M_PROJECTILE_ACC, speed, true, -1, forward, start);
		monster_fire_heat(self, start, forward, damage, speed, flashes[i], 0.06f);
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
	VectorMA(aim, crandom() * 0.15f, right, aim);
	VectorMA(aim, 0.1f, up, aim);
	VectorNormalize(aim);

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
	MonsterAim(self, 0.25f, 0, false, -1, forward, start);
	gi.sound(self, CHAN_WEAPON, sound_rail, 1, ATTN_NORM, 0);
	monster_fire_railgun(self, start, forward, damage, damage, MZ2_CARRIER_RAILGUN);
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

static qboolean carrier_find_spawn_spot(edict_t *self, vec3_t mins, vec3_t maxs, int index, vec3_t spot)
{
	vec3_t forward, right;
	float side;
	float dist;

	AngleVectors(self->s.angles, forward, right, NULL);
	side = (index & 1) ? 104.0f : -104.0f;
	dist = 128.0f + 48.0f * (float)(index / 2);

	VectorCopy(self->s.origin, spot);
	VectorMA(spot, dist, forward, spot);
	VectorMA(spot, side, right, spot);
	spot[2] -= 32;
	if (carrier_valid_spawn_spot(self, mins, maxs, spot))
		return true;

	VectorCopy(self->s.origin, spot);
	VectorMA(spot, -96, forward, spot);
	VectorMA(spot, side, right, spot);
	spot[2] -= 16;
	return carrier_valid_spawn_spot(self, mins, maxs, spot);
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

static qboolean carrier_spawn_monster(edict_t *self, int index)
{
	edict_t *owner;
	edict_t *spawned;
	vec3_t spot;

	owner = (self->activator && self->activator->inuse) ? self->activator : self;
	spawned = G_Spawn();
	spawned->mtype = carrier_summons[index % CARRIER_SUMMON_COUNT];
	spawned->activator = owner;
	spawned->monsterinfo.level = self->monsterinfo.level;

	if (!M_Initialize(owner, spawned, 0.0f))
	{
		G_FreeEdict(spawned);
		return false;
	}

	if (!carrier_find_spawn_spot(self, spawned->mins, spawned->maxs, index, spot))
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

static void carrier_spawngrows(edict_t *self)
{
	vec3_t mins, maxs, size, spot, effect_origin;
	float radius;

	VectorSet(mins, -24, -24, -24);
	VectorSet(maxs, 24, 24, 32);
	VectorSubtract(maxs, mins, size);
	radius = VectorLength(size) * 0.5f;

	for (int i = 0; i < CARRIER_SUMMON_COUNT; i++)
	{
		if (!carrier_find_spawn_spot(self, mins, maxs, i, spot))
			continue;

		VectorAdd(mins, maxs, effect_origin);
		VectorAdd(spot, effect_origin, effect_origin);
		SpawnGrow_Spawn(effect_origin, radius, radius * 2.0f);
	}
}

static void carrier_finish_spawn(edict_t *self)
{
	int spawned = 0;

	for (int i = 0; i < CARRIER_SUMMON_COUNT; i++)
	{
		if (carrier_spawn_monster(self, i))
			spawned++;
	}

	if (spawned)
		self->monsterinfo.melee_finished = level.time + CARRIER_SUMMON_COOLDOWN;
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

mframe_t carrier_frames_attack_rocket[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, carrier_fire_rocket,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t carrier_move_attack_rocket = { FRAME_fireb01, FRAME_fireb04, carrier_frames_attack_rocket, carrier_run };

mframe_t carrier_frames_attack_grenade[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, -15, carrier_fire_grenade,
	ai_charge, 0, NULL
};
mmove_t carrier_move_attack_grenade = { FRAME_fireb07, FRAME_fireb10, carrier_frames_attack_grenade, carrier_run };

mframe_t carrier_frames_attack_rail[] =
{
	ai_charge, 0, NULL,
	ai_charge, -10, NULL,
	ai_charge, -20, carrier_fire_rail,
	ai_charge, -10, NULL,
	ai_charge, 0, NULL
};
mmove_t carrier_move_attack_rail = { FRAME_search01, FRAME_search05, carrier_frames_attack_rail, carrier_run };

mframe_t carrier_frames_spawn[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, carrier_spawngrows,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, -2, carrier_finish_spawn,
	ai_charge, -6, NULL,
	ai_charge, -10, NULL,
	ai_charge, -6, NULL,
	ai_charge, -2, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t carrier_move_spawn = { FRAME_spawn01, FRAME_spawn18, carrier_frames_spawn, carrier_run };

static void carrier_start_spawn(edict_t *self)
{
	if (sound_spawn)
		gi.sound(self, CHAN_WEAPON, sound_spawn, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = &carrier_move_spawn;
}

static void carrier_attack(edict_t *self)
{
	float r;

	if (!G_EntExists(self->enemy))
		return;

	r = random();
	if (level.time >= self->monsterinfo.melee_finished && r < 0.20f)
		carrier_start_spawn(self);
	else if (r < 0.40f)
		self->monsterinfo.currentmove = &carrier_move_attack_rocket;
	else if (r < 0.60f)
		self->monsterinfo.currentmove = &carrier_move_attack_grenade;
	else if (r < 0.80f)
		self->monsterinfo.currentmove = &carrier_move_attack_rail;
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

	if (self->deadflag == DEAD_DEAD)
		return;

	DroneList_Remove(self);
	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
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
		VectorSet(self->mins, -40, -40, -24);
		VectorSet(self->maxs, 40, 40, 82);
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
