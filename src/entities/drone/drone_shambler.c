#include "g_local.h"
#include "quake2/monsterframes/m_shambler.h"

//KNOWN BUGS:
// 
// 
// Had problems adding, for example: M_SHAMBLER_INITIAL_HEALTH. until i used extern double for that, no LUA specifically for shambler INIT_COST/COST, using tank ones instead.
// Medics sometimes can't revive him unless it has a better distance from corpse, big bbox maybe, not 100% of the times
// Damage is nice for a normal monster, Melee is dangerous!, probably he's weak compared to other monsters // not bug lol, need maybe opinions?



//FIXED BUGS:
// 
// 
// Frost Nova is freezing every near monster independent of team //// FIXED added (OnSameTeam(self, target)), on both shambler and frostnova voids.
// Icebolt can only freeze Players, not monsters //FIXED creating Shambler_ValidIceboltTarget fixed it
// Sometimes if a summoned shambler dies, players would have to use monster remove to free the used slot // FIXED adding code to shambler_die void 
// Lightning only shines on one hand ( shambler_lightning_update(edict_t* self) ) // FIXED using other effects


static constexpr int MAX_LIGHTNING_FRAMES = 4;
static constexpr float SHAMBLER_ICE_CHARGE_MIN_SCALE = 0.1f;
static constexpr float SHAMBLER_ICE_CHARGE_MAX_SCALE = 1.0f;
#define SHAMBLER_ICE_CHARGE_GROW_TIME (7.0f * FRAMETIME)
static constexpr float SHAMBLER_ICE_CHARGE_TIMEOUT = 0.3f;
#define SHAMBLER_ICE_CHARGE_NAME "shambler_ice_charge"

static int sound_pain;
static int sound_idle;
static int sound_die;
static int sound_sight;
static int sound_attack;
static int sound_melee1;
static int sound_melee2;
static int sound_smack;

void shambler_stand(edict_t* self);
void shambler_run(edict_t* self);
void shambler_walk(edict_t* self);
void shambler_melee(edict_t* self);
void shambler_attack(edict_t* self);
void shambler_sight(edict_t* self, edict_t* other);
void shambler_idle(edict_t* self);
void shambler_pain(edict_t* self, edict_t* other, float kick, int damage);
void shambler_die(edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, vec3_t point);

static void FindShamblerOffset(edict_t* self, vec3_t offset);
static void shambler_free_ice_charges(edict_t* self);

void sham_swingl9(edict_t* self);
void sham_swingr9(edict_t* self);

//FROST NOVA Attack 

static constexpr int NOVA_RADIUS = 150;
static constexpr int NOVA_DEFAULT_DAMAGE = 50;
static constexpr int NOVA_ADDON_DAMAGE = 30;
static constexpr double NOVA_DELAY = 0.3;
static constexpr int FROSTNOVA_RADIUS = 150;
void NovaExplosionEffect(vec3_t org);

void shambler_frostnova(edict_t* self)
{
	if (!G_EntExists(self->enemy))
		return;

	edict_t* target = NULL;
	const int damage = NOVA_DEFAULT_DAMAGE + NOVA_ADDON_DAMAGE * self->monsterinfo.level;

	// nova dmg 
	T_RadiusDamage(self, self, damage, self, FROSTNOVA_RADIUS, MOD_NOVA);

	// freeze effect target
	while ((target = findradius(target, self->s.origin, FROSTNOVA_RADIUS)) != NULL)
	{
		if (target == self)
			continue;
		if (!target->takedamage)
			continue;
		if (!visible1(self, target))
			continue;
		if (OnSameTeam(self, target)) // added so shambler won't freeze friendly/owned entities
			continue;

		// apply freezing effect
		target->chill_level = 2 * self->monsterinfo.level;
		target->chill_time = level.time + 3.0;  // 3 sec freezing

		if (random() > 0.5)
			gi.sound(target, CHAN_ITEM, gi.soundindex("abilities/blue1.wav"), 1, ATTN_NORM, 0);
		else
			gi.sound(target, CHAN_ITEM, gi.soundindex("abilities/blue3.wav"), 1, ATTN_NORM, 0);
	}

	// effects 
	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_TELEPORT_EFFECT);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PVS);
	NovaExplosionEffect(self->s.origin);
	gi.sound(self, CHAN_WEAPON, gi.soundindex("abilities/novaelec.wav"), 1, ATTN_NORM, 0);
}

// MELEE STUFF
void shambler_melee2(edict_t* self)
{
	gi.sound(self, CHAN_WEAPON, sound_melee2, 1, ATTN_NORM, 0);
}

void shambler_meleesnd(edict_t* self)
{
	gi.sound(self, CHAN_WEAPON, sound_melee1, 1, ATTN_NORM, 0);
}

void sham_swingl9(edict_t* self);
void sham_swingr9(edict_t* self);

// smash melee
void sham_smash10(edict_t* self)
{
	if (!G_EntExists(self->enemy))
		return;

	int damage = M_MELEE_DMG_BASE + M_MELEE_DMG_ADDON * drone_damagelevel(self);
	if (M_MELEE_DMG_MAX && damage > M_MELEE_DMG_MAX)
		damage = M_MELEE_DMG_MAX;

	if (M_MeleeAttack(self, self->enemy, MELEE_DISTANCE, damage, 200))
	{
		gi.sound(self, CHAN_WEAPON, sound_smack, 1, ATTN_NORM, 0);
	}
}

// melee (claw)
void ShamClaw(edict_t* self)
{
	if (!G_EntExists(self->enemy))
		return;

	int damage = M_MELEE_DMG_BASE + M_MELEE_DMG_ADDON * drone_damagelevel(self);
	if (M_MELEE_DMG_MAX && damage > M_MELEE_DMG_MAX)
		damage = M_MELEE_DMG_MAX;

	if (M_MeleeAttack(self, self->enemy, MELEE_DISTANCE, damage, 150))
	{
		gi.sound(self, CHAN_WEAPON, sound_smack, 1, ATTN_NORM, 0);
	}
}

// Frames (smash)
mframe_t shambler_frames_smash[] = {
	{ ai_charge, 2, shambler_meleesnd },
	{ ai_charge, 6},
	{ ai_charge, 6, shambler_frostnova},
	{ ai_charge, 5 },
	{ ai_charge, 4 },
	{ ai_charge, 1 },
//   { ai_charge, 0 },
//   { ai_charge, 0 },
//   { ai_charge, 0 },
	 { ai_charge, 0, sham_smash10 },
	 { ai_charge, 5 },
	 { ai_charge, 4 },
};
mmove_t shambler_move_smash = { FRAME_smash4, FRAME_smash12, shambler_frames_smash, shambler_run };

// Frames (swingl)
mframe_t shambler_frames_swingl[] = {
	{ ai_charge, 5, shambler_meleesnd },
	{ ai_charge, 3 },
	{ ai_charge, 7, shambler_frostnova },
	{ ai_charge, 3 },
//  { ai_charge, 7 },
//  { ai_charge, 9 },
	{ ai_charge, 5, ShamClaw },
	{ ai_charge, 4 },
	{ ai_charge, 8, sham_swingl9 },
};
mmove_t shambler_move_swingl = { FRAME_swingl3, FRAME_swingl9, shambler_frames_swingl, shambler_run };

// Frames (swingr)
mframe_t shambler_frames_swingr[] = {
	{ ai_charge, 1, shambler_melee2 },
	{ ai_charge, 8 },
	{ ai_charge, 14, shambler_frostnova },
	{ ai_charge, 7 },
//   { ai_charge, 3 },
//   { ai_charge, 6 },
	{ ai_charge, 6, ShamClaw },
	{ ai_charge, 3 },
	{ ai_charge, 8, sham_swingr9 },
};
mmove_t shambler_move_swingr = { FRAME_swingr3, FRAME_swingr9, shambler_frames_swingr, shambler_run };

void sham_swingl9(edict_t* self)
{
	ai_charge(self, 8);
	if (G_EntExists(self->enemy) && random() < 0.5 && entdist(self, self->enemy) < MELEE_DISTANCE)
		self->monsterinfo.currentmove = &shambler_move_swingr;
}

void sham_swingr9(edict_t* self)
{
	ai_charge(self, 1);
	ai_charge(self, 10);
	if (G_EntExists(self->enemy) && random() < 0.5 && entdist(self, self->enemy) < MELEE_DISTANCE)
		self->monsterinfo.currentmove = &shambler_move_swingl;
}
void shambler_melee(edict_t* self)
{
	if (!G_EntExists(self->enemy))
		return;

	const float dist = entdist(self, self->enemy);

	if (dist <= 100) //MELEE_DISTANCE +20
	{
		const float r = random();
		if (r > 0.6 || self->health == 600)
			self->monsterinfo.currentmove = &shambler_move_smash;
		else if (r > 0.3)
			self->monsterinfo.currentmove = &shambler_move_swingl;
		else
			self->monsterinfo.currentmove = &shambler_move_swingr;
	}
	else if (dist >= MELEE_DISTANCE * 2)
	{
		shambler_attack(self);
	}
	else
	{
		shambler_run(self);
	}
}

// stand
mframe_t shambler_frames_stand[] =
{
	{drone_ai_stand, 0, NULL},
	{drone_ai_stand, 0, NULL},
	{drone_ai_stand, 0, NULL},
	{drone_ai_stand, 0, NULL},
	{drone_ai_stand, 0, NULL},
	{drone_ai_stand, 0, NULL},
	{drone_ai_stand, 0, NULL},
	{drone_ai_stand, 0, NULL},
	{drone_ai_stand, 0, NULL},
	{drone_ai_stand, 0, NULL},
	{drone_ai_stand, 0, NULL},
	{drone_ai_stand, 0, NULL},
	{drone_ai_stand, 0, NULL},
	{drone_ai_stand, 0, NULL},
	{drone_ai_stand, 0, NULL},
	{drone_ai_stand, 0, NULL},
	{drone_ai_stand, 0, NULL}
};
mmove_t shambler_move_stand = { FRAME_stand1, FRAME_stand17, shambler_frames_stand, NULL };

// walk
mframe_t shambler_frames_walk[] =
{
	{drone_ai_walk, 10, NULL},
	{drone_ai_walk, 9, NULL},
	{drone_ai_walk, 9, NULL},
	{drone_ai_walk, 5, NULL},
	{drone_ai_walk, 6, NULL},
	{drone_ai_walk, 12, NULL},
	{drone_ai_walk, 8, NULL},
	{drone_ai_walk, 3, NULL},
	{drone_ai_walk, 13, NULL},
	{drone_ai_walk, 9, NULL},
	{drone_ai_walk, 7, NULL},
	{drone_ai_walk, 5, NULL}
};
mmove_t shambler_move_walk = { FRAME_walk1, FRAME_walk12, shambler_frames_walk, NULL };

// run
mframe_t shambler_frames_run[] =
{
	{drone_ai_run, 20, NULL},
	{drone_ai_run, 24, NULL},
	{drone_ai_run, 20, NULL},
	{drone_ai_run, 20, NULL},
	{drone_ai_run, 24, NULL},
	{drone_ai_run, 20, NULL}
};
mmove_t shambler_move_run = { FRAME_run1, FRAME_run6, shambler_frames_run, NULL };

// pre-attack lightning effects
static const vec3_t lightning_left_hand[] = {
	{ 44, 36, 25 },
	{ 10, 44, 57 },
	{ -1, 40, 70 },
	{ -10, 34, 75 },
	{ 7.4f, 24, 89 }
};

// pre-attack lightning effects
static const vec3_t lightning_right_hand[] = {
	{ 28, -38, 25 },
	{ 31, -7, 70 },
	{ 20, 0, 80 },
	{ 16, 1.2f, 81 },
	{ 27, -11, 83 }
};

//attack lightning stuff
static void shambler_lightning_update(edict_t* self)
{
	const int frame_offset = self->s.frame - FRAME_magic1;
	if (frame_offset >= MAX_LIGHTNING_FRAMES)
	{
		return;
	}
	vec3_t f, r;
	AngleVectors(self->s.angles, f, r, NULL);

	vec3_t left_pos, right_pos;
	VectorMA(self->s.origin, lightning_left_hand[frame_offset][0], f, left_pos);
	VectorMA(left_pos, lightning_left_hand[frame_offset][1], r, left_pos);
	left_pos[2] += lightning_left_hand[frame_offset][2];
	VectorMA(self->s.origin, lightning_right_hand[frame_offset][0], f, right_pos);
	VectorMA(right_pos, lightning_right_hand[frame_offset][1], r, right_pos);
	right_pos[2] += lightning_right_hand[frame_offset][2];

	gi.WriteByte(svc_temp_entity);
#ifdef VRX_REPRO
	gi.WriteByte(TE_LIGHTNING);
	gi.WriteShort(self - g_edicts);
	gi.WriteShort(0);
#else
	gi.WriteByte(TE_MONSTER_HEATBEAM);
	gi.WriteShort(self - g_edicts);
#endif
	gi.WritePosition(left_pos);
	gi.WritePosition(right_pos);
	gi.multicast(left_pos, MULTICAST_PVS);


	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_WELDING_SPARKS);
	gi.WriteByte(15); 
	gi.WritePosition(left_pos);
	gi.WriteDir(vec3_origin);
	gi.WriteByte(0xE0); 
	gi.multicast(left_pos, MULTICAST_PVS);

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_WELDING_SPARKS);
	gi.WriteByte(15);  
	gi.WritePosition(right_pos);
	gi.WriteDir(vec3_origin);
	gi.WriteByte(0xE0); 
	gi.multicast(right_pos, MULTICAST_PVS);
}
void shambler_windup(edict_t* self)
{
    gi.sound(self, CHAN_WEAPON, sound_attack, 1, ATTN_NORM, 0);
    shambler_lightning_update(self);
    self->nextthink = level.time + FRAMETIME;
}

void ShamblerSaveLoc(edict_t* self)
{
	if (!G_EntExists(self->enemy))
		return;

	VectorCopy(self->enemy->s.origin, self->pos1);
	self->pos1[2] += self->enemy->viewheight;
}

static void FindShamblerOffset(edict_t* self, vec3_t offset)
{
	vec3_t start, end;
	trace_t tr;
	float z_offset = 48.0f;

	VectorSet(offset, 0, 0, z_offset);

	for (int i = 0; i < 8; i++)
	{
		VectorAdd(self->s.origin, offset, start);
		VectorCopy(start, end);
		end[2] -= 512;

		tr = gi.trace(start, NULL, NULL, end, self, MASK_SOLID);
		if (tr.fraction < 1)
			return;

		z_offset -= 4.0f;
		offset[2] = z_offset;
	}
}

void ShamblerCastLightning(edict_t* self)
{
	vec3_t start, dir, end;
	vec3_t forward, right;
	vec3_t offset;
	trace_t tr;

	if (!G_EntIsAlive(self->enemy))
		return;

	// use shambler offset point
	AngleVectors(self->s.angles, forward, right, NULL);
	FindShamblerOffset(self, offset);
	G_ProjectSource(self->s.origin, offset, forward, right, start);

	// trace enemy
	VectorSubtract(self->enemy->s.origin, start, dir);
	VectorNormalize(dir);

	// calculate lightning distance
	VectorMA(start, 2048, dir, end);
	tr = gi.trace(start, NULL, NULL, end, self, MASK_SHOT);


	//unused TE_LIGHTNING, doesn't work here or any attack 
	gi.WriteByte(svc_temp_entity);
#ifdef VRX_REPRO
	gi.WriteByte(TE_LIGHTNING);
	gi.WriteShort(self - g_edicts);
	gi.WriteShort(0);
#else
	gi.WriteByte(TE_MONSTER_HEATBEAM);
	gi.WriteShort(self - g_edicts);
#endif
	gi.WritePosition(start);
	gi.WritePosition(tr.endpos);
	gi.multicast(start, MULTICAST_PVS);

	if (tr.fraction < 1.0f && tr.ent)
	{
		const int damage = 4 + 3 * drone_damagelevel(self);
		T_Damage(tr.ent, self, self, dir, tr.endpos, tr.plane.normal, damage, 0, DAMAGE_ENERGY, MOD_LIGHTNING);
	}
}

mframe_t shambler_frames_magic[] = {
	{ai_charge, 0, shambler_windup},
	{ai_charge, 0, shambler_lightning_update},
	{ai_charge, 0, shambler_lightning_update},
	{ai_move, 0, shambler_lightning_update},
	{ai_move, 0, shambler_lightning_update},
	{ai_move, 0, ShamblerSaveLoc},
	{ai_move, 0, NULL},
	{ai_move, 0, ShamblerCastLightning},
	{ai_move, 0, ShamblerCastLightning},
	{ai_move, 0, ShamblerCastLightning},
	{ai_move, 0, ShamblerCastLightning},
	{ai_move, 0, NULL},
};
mmove_t shambler_move_attack = { FRAME_magic1, FRAME_magic12, shambler_frames_magic, shambler_run };

// New function to calculate aim direction
void CalculateAimDirection(edict_t* self, vec3_t start, vec3_t aim)
{
	vec3_t target;

	// Use a mix of saved location and current enemy location
	VectorAdd(self->pos1, self->enemy->s.origin, target);
	VectorScale(target, 0.5, target);

	VectorSubtract(target, start, aim);
	VectorNormalize(aim);
}

//void fire_icebolt(edict_t* self, vec3_t start, vec3_t aimdir, int damage, float damage_radius, int speed, int chillLevel, float chillDuration, float freezeDuration);
void ShamblerCastIcebolt(edict_t* self)
{
	vec3_t forward, right, up;
	vec3_t start_left, start_right;
	vec3_t aim_left, aim_right;
	const float accuracy = 0.8; // Aumentamos la precisi�n base

	if (!G_EntIsAlive(self->enemy))
		return;

	// Get the current frame offset
	int frame_offset = self->s.frame - FRAME_magic1;
	if (frame_offset >= MAX_LIGHTNING_FRAMES)
	{
		frame_offset = MAX_LIGHTNING_FRAMES - 1;
	}

	// Calculate hand positions
	AngleVectors(self->s.angles, forward, right, up);

	// Left hand
	VectorMA(self->s.origin, lightning_left_hand[frame_offset][0], forward, start_left);
	VectorMA(start_left, lightning_left_hand[frame_offset][1], right, start_left);
	start_left[2] = self->s.origin[2] + lightning_left_hand[frame_offset][2];

	// Right hand
	VectorMA(self->s.origin, lightning_right_hand[frame_offset][0], forward, start_right);
	VectorMA(start_right, lightning_right_hand[frame_offset][1], right, start_right);
	start_right[2] = self->s.origin[2] + lightning_right_hand[frame_offset][2];

	// Calculate damage and other parameters
	const float slvl = drone_damagelevel(self);
	const int damage = ICEBOLT_INITIAL_DAMAGE + ICEBOLT_ADDON_DAMAGE * slvl;
	const float damage_radius = ICEBOLT_INITIAL_RADIUS + ICEBOLT_ADDON_RADIUS * slvl;
	const int speed = ICEBOLT_INITIAL_SPEED + ICEBOLT_ADDON_SPEED * slvl;
	const int chillLevel = 2 * slvl;
	const float chillDuration = ICEBOLT_INITIAL_CHILL_DURATION + ICEBOLT_ADDON_CHILL_DURATION * slvl;

	// Use MonsterAim for both hands
	MonsterAim(self, accuracy, speed, false, -1, aim_left, start_left);
	MonsterAim(self, accuracy, speed, false, -1, aim_right, start_right);

	// Fire icebolt from left hand
	fire_icebolt(self, start_left, aim_left, damage, damage_radius, speed, chillLevel, chillDuration, 0);

	// Fire icebolt from right hand
	fire_icebolt(self, start_right, aim_right, damage, damage_radius, speed, chillLevel, chillDuration, 0);

	// Play sound effect
	gi.sound(self, CHAN_WEAPON, gi.soundindex("spells/coldcast.wav"), 1, ATTN_NORM, 0);
}

static float shambler_clampf(float value, float min_value, float max_value)
{
	if (value < min_value)
		return min_value;
	if (value > max_value)
		return max_value;
	return value;
}

static float shambler_ice_charge_scale(int frame_offset)
{
	const float progress = shambler_clampf((float)frame_offset / 7.0f, 0.0f, 1.0f);

	return shambler_clampf(SHAMBLER_ICE_CHARGE_MIN_SCALE +
		(SHAMBLER_ICE_CHARGE_MAX_SCALE - SHAMBLER_ICE_CHARGE_MIN_SCALE) * progress,
		SHAMBLER_ICE_CHARGE_MIN_SCALE, SHAMBLER_ICE_CHARGE_MAX_SCALE);
}

static qboolean shambler_is_ice_charge(edict_t *charge)
{
	return charge && charge->inuse && charge->classname && !strcmp(charge->classname, SHAMBLER_ICE_CHARGE_NAME);
}

static void shambler_clear_ice_charge_owner(edict_t *charge)
{
	if (!charge || !charge->owner || !charge->owner->inuse)
		return;

	if (charge->owner->beam == charge)
		charge->owner->beam = NULL;
	if (charge->owner->beam2 == charge)
		charge->owner->beam2 = NULL;
}

static void shambler_ice_charge_think(edict_t *self)
{
	float progress;
	int i;

	if (!self->owner || !self->owner->inuse || self->owner->deadflag || level.time >= self->timestamp)
	{
		shambler_clear_ice_charge_owner(self);
		G_FreeEdict(self);
		return;
	}

	for (i = 0; i < 3; i++)
		self->s.angles[i] += self->avelocity[i] * FRAMETIME;

	progress = shambler_clampf((level.time - self->teleport_time) / self->wait, 0.0f, 1.0f);
	self->s.scale = shambler_clampf(self->accel + (self->decel - self->accel) * progress,
		SHAMBLER_ICE_CHARGE_MIN_SCALE, SHAMBLER_ICE_CHARGE_MAX_SCALE);

	gi.linkentity(self);
	self->nextthink = level.time + FRAMETIME;
}

static edict_t *shambler_get_ice_charge(edict_t *self, qboolean right_hand)
{
	edict_t **slot = right_hand ? &self->beam2 : &self->beam;
	edict_t *charge = *slot;

	if (shambler_is_ice_charge(charge))
		return charge;

	if (charge && !charge->inuse)
		*slot = NULL;
	else if (charge)
		return NULL;

	charge = G_Spawn();

	if (!charge)
		return NULL;

	VectorCopy(self->s.angles, charge->s.angles);
	charge->s.modelindex = gi.modelindex("models/proj/proj_drole/tris.md2");
	charge->s.skinnum = 1;
	charge->s.frame = 4;
	charge->s.effects |= EF_HALF_DAMAGE | EF_FLAG2;
	VectorSet(charge->avelocity, GetRandom(300, 1000), 0, 0);
	charge->solid = SOLID_NOT;
	charge->movetype = MOVETYPE_NONE;
	charge->classname = SHAMBLER_ICE_CHARGE_NAME;
	charge->owner = self;
	charge->think = shambler_ice_charge_think;
	charge->accel = SHAMBLER_ICE_CHARGE_MIN_SCALE;
	charge->decel = SHAMBLER_ICE_CHARGE_MAX_SCALE;
	charge->s.scale = SHAMBLER_ICE_CHARGE_MIN_SCALE;
	charge->teleport_time = level.time;
	charge->wait = SHAMBLER_ICE_CHARGE_GROW_TIME;
	charge->timestamp = level.time + SHAMBLER_ICE_CHARGE_TIMEOUT;
	charge->nextthink = level.time + FRAMETIME;
	*slot = charge;

	return charge;
}

static void shambler_update_ice_charge(edict_t *self, vec3_t origin, float scale, qboolean right_hand)
{
	edict_t *charge = shambler_get_ice_charge(self, right_hand);

	if (!charge)
		return;

	if (VectorLength(charge->s.origin))
		VectorCopy(charge->s.origin, charge->s.old_origin);
	else
		VectorCopy(origin, charge->s.old_origin);
	VectorCopy(origin, charge->s.origin);
	charge->timestamp = level.time + SHAMBLER_ICE_CHARGE_TIMEOUT;
	if (charge->s.scale < scale)
		charge->s.scale = scale;
	gi.linkentity(charge);
}

static void shambler_free_ice_charge(edict_t **slot)
{
	edict_t *charge = *slot;

	if (charge && !charge->inuse)
	{
		*slot = NULL;
		return;
	}

	if (!shambler_is_ice_charge(charge))
		return;

	*slot = NULL;
	G_FreeEdict(charge);
}

static void shambler_free_ice_charges(edict_t* self)
{
	shambler_free_ice_charge(&self->beam);
	shambler_free_ice_charge(&self->beam2);
}

static void shambler_ice_charge_particles(vec3_t origin, float scale)
{
	const int blue_count = (int)shambler_clampf(8.0f + scale * 6.0f, 8.0f, 12.0f);
	vec3_t down = { 0, 0, -1 };

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_WELDING_SPARKS);
	gi.WriteByte(blue_count);
	gi.WritePosition(origin);
	gi.WriteDir(down);
	gi.WriteByte(113);
	gi.multicast(origin, MULTICAST_PVS);

	if (random() <= 0.33f)
	{
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_WELDING_SPARKS);
		gi.WriteByte(1);
		gi.WritePosition(origin);
		gi.WriteDir(down);
		gi.WriteByte(217);
		gi.multicast(origin, MULTICAST_PVS);
	}
}

static void shambler_ice_update(edict_t* self)
{
	const int raw_frame_offset = self->s.frame - FRAME_magic1;
	int frame_offset = raw_frame_offset;
	const float scale = shambler_ice_charge_scale(raw_frame_offset);

	if (frame_offset < 0)
		return;
	if (frame_offset >= MAX_LIGHTNING_FRAMES)
		frame_offset = MAX_LIGHTNING_FRAMES - 1;

	vec3_t f, r;
	AngleVectors(self->s.angles, f, r, NULL);

	// use hands origin
	vec3_t left_pos, right_pos;
	VectorMA(self->s.origin, lightning_left_hand[frame_offset][0], f, left_pos);
	VectorMA(left_pos, lightning_left_hand[frame_offset][1], r, left_pos);
	left_pos[2] += lightning_left_hand[frame_offset][2];

	VectorMA(self->s.origin, lightning_right_hand[frame_offset][0], f, right_pos);
	VectorMA(right_pos, lightning_right_hand[frame_offset][1], r, right_pos);
	right_pos[2] += lightning_right_hand[frame_offset][2];

	shambler_update_ice_charge(self, left_pos, scale, false);
	shambler_update_ice_charge(self, right_pos, scale, true);
	shambler_ice_charge_particles(left_pos, scale);
	shambler_ice_charge_particles(right_pos, scale);
}

void shambler_windupIce(edict_t* self) // lightning preparing
{
	shambler_ice_update(self);

	gi.sound(self, CHAN_WEAPON, sound_attack, 1, ATTN_NORM, 0);

	self->nextthink = level.time + FRAMETIME;
}

static void ShamblerSaveLocAndIceUpdate(edict_t* self)
{
	ShamblerSaveLoc(self);
	shambler_ice_update(self);
}

static void ShamblerSaveLocIceUpdateAndCast(edict_t* self)
{
	ShamblerSaveLocAndIceUpdate(self);
	ShamblerCastIcebolt(self);
}

static void shambler_finish_icebolt(edict_t* self)
{
	shambler_free_ice_charges(self);
	shambler_run(self);
}


mframe_t shambler_frames_icebolt[] = {
	{ai_charge, 0, shambler_windupIce},
	{ai_charge, 0, ShamblerSaveLocAndIceUpdate},
	{ai_charge, 0, shambler_ice_update},
	{ai_move, 0, ShamblerSaveLocAndIceUpdate},
	{ai_move, 0, shambler_ice_update},
	{ai_move, 0, ShamblerSaveLocAndIceUpdate},
	{ai_move, 0, shambler_ice_update},
	{ai_move, 0, ShamblerSaveLocIceUpdateAndCast},
	{ai_move, 0, ShamblerCastIcebolt},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_charge, 0, NULL},
};
mmove_t shambler_move_icebolt = { FRAME_magic1, FRAME_magic12, shambler_frames_icebolt, shambler_finish_icebolt };

void shambler_meleehit(edict_t* self);

mframe_t shambler_frames_melee[] =
{
	{ai_charge, 2, shambler_meleesnd},
	{ai_charge, 6, NULL},
	{ai_charge, 6, NULL},
	{ai_charge, 5, NULL},
	{ai_charge, 4, NULL},
	{ai_charge, 1, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, shambler_meleehit},
	{ai_charge, 5, NULL},
	{ai_charge, 4, NULL}
};
mmove_t shambler_move_melee = { FRAME_smash1, FRAME_smash12, shambler_frames_melee, shambler_run };

mframe_t shambler_frames_pain[] =
{
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
mmove_t shambler_move_pain = { FRAME_pain1, FRAME_pain6, shambler_frames_pain, shambler_run };


void shambler_dead(edict_t* self);

static void shambler_shrink(edict_t *self)
{
	self->maxs[2] = 0;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}


mframe_t shambler_frames_death[] =
{
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, shambler_shrink},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
mmove_t shambler_move_death = { FRAME_death1, FRAME_death11, shambler_frames_death, shambler_dead };

void shambler_stand(edict_t* self)
{
	self->monsterinfo.currentmove = &shambler_move_stand;
}

void shambler_run(edict_t* self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &shambler_move_stand;
	else
		self->monsterinfo.currentmove = &shambler_move_run;
}

void shambler_walk(edict_t* self)
{
	if (!self->goalentity)
		self->goalentity = world;
	self->monsterinfo.currentmove = &shambler_move_walk;
}

void shambler_meleehit(edict_t* self)
{
	int damage;
	vec3_t	aim;

	damage = M_MELEE_DMG_BASE + M_MELEE_DMG_ADDON * drone_damagelevel(self); // dmg: mutant_hit_left_world
	if (M_MELEE_DMG_MAX && damage > M_MELEE_DMG_MAX)
		damage = M_MELEE_DMG_MAX;

	VectorSet(aim, 100, self->mins[0], 8);
	if (fire_hit(self, aim, damage, 100))
		gi.sound(self, CHAN_WEAPON, sound_melee1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_WEAPON, sound_melee2, 1, ATTN_NORM, 0);
}

void shambler_attack(edict_t* self)
{

	const float r = random();
	const float dist = entdist(self, self->enemy);

	if (dist <= MELEE_DISTANCE)
	{

		if (r > 0.6 || self->health == 600)
			self->monsterinfo.currentmove = &shambler_move_smash;
		else if (r > 0.3)
			self->monsterinfo.currentmove = &shambler_move_swingl;
		else
			self->monsterinfo.currentmove = &shambler_move_swingr;
	}
	else if (r < 0.3 && infront(self, self->enemy))  // 30% to use icebolt attack
	{
		self->monsterinfo.currentmove = &shambler_move_icebolt;
	}
	else
	{
		self->monsterinfo.currentmove = &shambler_move_attack;
	}
}
void shambler_sight(edict_t* self, edict_t* other)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void shambler_idle(edict_t* self)
{
	gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

void shambler_pain(edict_t* self, edict_t* other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))  
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3;
	gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
	shambler_free_ice_charges(self);
	self->monsterinfo.currentmove = &shambler_move_pain;
}

void shambler_dead(edict_t* self)
{
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, 0);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
	M_PrepBodyRemoval(self);
}

void shambler_die(edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, vec3_t point)
{
	shambler_free_ice_charges(self);

	// notify the owner that the monster is dead
	M_Notify(self);

#ifdef OLD_NOLAG_STYLE
	// reduce lag by removing the entity right away
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
		//self->deadflag = DEAD_DEAD;
		//return;//FIXME: this will cause DroneList_Next to enter an infinite loop

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

	gi.sound(self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	vrx_update_drone_death_skin(self);
	self->monsterinfo.currentmove = &shambler_move_death;

	if (self->activator && !self->activator->client)
	{
		self->activator->num_monsters_real--;
		// gi.bprintf(PRINT_HIGH, "releasing %p (%d)\n", self, self->activator->num_monsters_real);
	}
}

void init_drone_shambler(edict_t* self)
{
	self->s.modelindex = gi.modelindex("models/monsters/shambler/tris.md2");
	VectorSet(self->mins, -32, -32, -24);
	VectorSet(self->maxs, 32, 32, 64);
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	//nova
	gi.modelindex("models/objects/nova/tris.md2");
	gi.soundindex("abilities/novaelec.wav");
	gi.soundindex("abilities/blue3.wav");
	gi.soundindex("abilities/blue1.wav");

	//icebolt shambler
	gi.modelindex("models/proj/proj_drole/tris.md2");
	gi.soundindex("spells/coldcast.wav");

	//shambler sounds
	sound_pain = gi.soundindex("shambler/shurt2.wav");
	sound_idle = gi.soundindex("shambler/sidle.wav");
	sound_die = gi.soundindex("shambler/sdeath.wav");
	sound_attack = gi.soundindex("shambler/sattck1.wav");
	sound_melee1 = gi.soundindex("shambler/melee1.wav");
	sound_melee2 = gi.soundindex("shambler/melee2.wav");
	sound_sight = gi.soundindex("shambler/ssight.wav");
	sound_smack = gi.soundindex("shambler/smack.wav");

	self->gib_health = -2 * BASE_GIB_HEALTH;
	self->health = M_SHAMBLER_INITIAL_HEALTH + M_SHAMBLER_ADDON_HEALTH * self->monsterinfo.level; // hlt: tank
	self->max_health = self->health;
	self->mass = 300;

	self->monsterinfo.control_cost = M_TANK_CONTROL_COST; //using tank atm
	self->monsterinfo.cost = M_TANK_COST; //using tank atm

	self->pain = shambler_pain;
	self->die = shambler_die;

	self->monsterinfo.stand = shambler_stand;
	self->monsterinfo.walk = shambler_walk;
	self->monsterinfo.run = shambler_run;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = shambler_attack;
	self->monsterinfo.melee = shambler_melee;
	self->monsterinfo.sight = shambler_sight;
	self->monsterinfo.idle = shambler_idle;
	self->monsterinfo.jumpup = 64;
	self->monsterinfo.jumpdn = 512;

	self->monsterinfo.aiflags |= AI_NO_CIRCLE_STRAFE;
	self->mtype = M_SHAMBLER;
	gi.linkentity(self);

	self->monsterinfo.currentmove = &shambler_move_stand;
	self->monsterinfo.scale = MODEL_SCALE;

	self->nextthink = level.time + FRAMETIME;
}
