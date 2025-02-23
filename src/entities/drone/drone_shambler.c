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


#define MAX_LIGHTNING_FRAMES 4

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

void sham_swingl9(edict_t* self);
void sham_swingr9(edict_t* self);

//FROST NOVA Attack 

#define NOVA_RADIUS				150
#define NOVA_DEFAULT_DAMAGE		50
#define NOVA_ADDON_DAMAGE		30
#define NOVA_DELAY				0.3
#define FROSTNOVA_RADIUS		150
void NovaExplosionEffect(vec3_t org);

void shambler_frostnova(edict_t* self)
{
	if (!G_EntExists(self->enemy))
		return;

	edict_t* target = NULL;
	int damage = NOVA_DEFAULT_DAMAGE + NOVA_ADDON_DAMAGE * self->monsterinfo.level;

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

	float dist = entdist(self, self->enemy);

	if (dist <= 100) //MELEE_DISTANCE +20
	{
		float r = random();
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
	int frame_offset = self->s.frame - FRAME_magic1;
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
	gi.WriteByte(TE_MONSTER_HEATBEAM);
	gi.WriteShort(self - g_edicts);
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
	gi.WriteByte(TE_MONSTER_HEATBEAM);
	gi.WriteShort(self - g_edicts);
	gi.WritePosition(start);
	gi.WritePosition(tr.endpos);
	gi.multicast(start, MULTICAST_PVS);


	//apply dmg!



	if (tr.fraction < 1.0 && tr.ent)
	{
		//int damage = M_SHAMBLER_LIGHTNING_BASE_DMG + M_SHAMBLER_ADDON_LIGHTNING_DMG; //* self->monsterinfo.level; ?
		int damage = 4 + 3 * drone_damagelevel(self);

		//if (M_SHAMBLER_LIGHTNING_MAX_DMG && damage > M_SHAMBLER_LIGHTNING_MAX_DMG)
		//	damage = M_SHAMBLER_LIGHTNING_MAX_DMG;

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


//fiery skull attack

static void shambler_fieryskull_update(edict_t* self)
{
	int frame_offset = self->s.frame - FRAME_magic1;
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
	gi.WriteByte(TE_MONSTER_HEATBEAM);
	gi.WriteShort(self - g_edicts);
	gi.WritePosition(left_pos);
	gi.WritePosition(right_pos);
	gi.multicast(left_pos, MULTICAST_PVS);
}

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

void fire_icebolt(edict_t* self, vec3_t start, vec3_t aimdir, int damage, float damage_radius, int speed, int chillLevel, float chillDuration);
void ShamblerCastIcebolt(edict_t* self)
{
	vec3_t forward, right, up;
	vec3_t start_left, start_right;
	vec3_t aim_left, aim_right;
	float accuracy = 0.8; // Aumentamos la precisión base

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
	float slvl = drone_damagelevel(self);
	int damage = ICEBOLT_INITIAL_DAMAGE + ICEBOLT_ADDON_DAMAGE * slvl;
	float damage_radius = ICEBOLT_INITIAL_RADIUS + ICEBOLT_ADDON_RADIUS * slvl;
	int speed = ICEBOLT_INITIAL_SPEED + ICEBOLT_ADDON_SPEED * slvl;
	int chillLevel = 2 * slvl;
	float chillDuration = ICEBOLT_INITIAL_CHILL_DURATION + ICEBOLT_ADDON_CHILL_DURATION * slvl;

	// Use MonsterAim for both hands
	MonsterAim(self, accuracy, speed, false, -1, aim_left, start_left);
	MonsterAim(self, accuracy, speed, false, -1, aim_right, start_right);

	// Fire icebolt from left hand
	fire_icebolt(self, start_left, aim_left, damage, damage_radius, speed, chillLevel, chillDuration);

	// Fire icebolt from right hand
	fire_icebolt(self, start_right, aim_right, damage, damage_radius, speed, chillLevel, chillDuration);

	// Play sound effect
	gi.sound(self, CHAN_WEAPON, gi.soundindex("spells/coldcast.wav"), 1, ATTN_NORM, 0);
}


static void shambler_ice_update(edict_t* self)
{
	int frame_offset = self->s.frame - FRAME_magic1;
	if (frame_offset >= MAX_LIGHTNING_FRAMES)
	{
		return;
	}
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

	// create fire models on both hands
	edict_t* left_fire = G_Spawn();
	edict_t* right_fire = G_Spawn();

	VectorCopy(left_pos, left_fire->s.origin);
	VectorCopy(right_pos, right_fire->s.origin);

	left_fire->s.modelindex = gi.modelindex("models/fire/tris.md2");
	right_fire->s.modelindex = gi.modelindex("models/fire/tris.md2");	
	
	//left_fire->s.modelindex = gi.modelindex("models/objects/flball/tris.md2"); // ugly
	//right_fire->s.modelindex = gi.modelindex("models/objects/flball/tris.md2"); // ugly
	left_fire->s.effects |= EF_QUAD | RF_SHELL_CYAN;
	right_fire->s.effects |= EF_QUAD | RF_SHELL_CYAN;

	left_fire->s.renderfx |= RF_FULLBRIGHT;
	right_fire->s.renderfx |= RF_FULLBRIGHT;

	left_fire->think = G_FreeEdict;
	right_fire->think = G_FreeEdict;

	left_fire->nextthink = level.time + 0.1;
	right_fire->nextthink = level.time + 0.1;

	gi.linkentity(left_fire);
	gi.linkentity(right_fire);
}

void shambler_windupIce(edict_t* self) // lightning preparing
{
	shambler_ice_update(self);

	gi.sound(self, CHAN_WEAPON, sound_attack, 1, ATTN_NORM, 0);

	self->nextthink = level.time + FRAMETIME;
}


mframe_t shambler_frames_icebolt[] = {
	{ai_charge, 0, shambler_windupIce},
	{ai_charge, 0, ShamblerSaveLoc},
	{ai_charge, 0, shambler_ice_update},
	{ai_move, 0, ShamblerSaveLoc},
	{ai_move, 0, shambler_ice_update},
	{ai_move, 0, ShamblerSaveLoc},
	{ai_move, 0, NULL},
	{ai_move, 0, ShamblerSaveLoc},
	{ai_move, 0, ShamblerCastIcebolt},
	{ai_move, 0, ShamblerSaveLoc},
	{ai_move, 0, ShamblerCastIcebolt},
	{ai_charge, 0, NULL},
};
mmove_t shambler_move_icebolt = { FRAME_magic1, FRAME_magic12, shambler_frames_icebolt, shambler_run };


// FIERY ROCKET SKULLS

void shambler_windupFire(edict_t* self) // lightning preparing
{
	shambler_fieryskull_update(self);

	gi.sound(self, CHAN_WEAPON, gi.soundindex("sound_attack"), 1, ATTN_NORM, 0);

	self->nextthink = level.time + FRAMETIME;
}


//pre fire attack stuff
static void shambler_fire_update(edict_t* self)
{
	int frame_offset = self->s.frame - FRAME_magic1;
	if (frame_offset >= MAX_LIGHTNING_FRAMES)
	{
		return;
	}
	vec3_t f, r;
	AngleVectors(self->s.angles, f, r, NULL);

	// Proyectar las posiciones de las manos
	vec3_t left_pos, right_pos;
	VectorMA(self->s.origin, lightning_left_hand[frame_offset][0], f, left_pos);
	VectorMA(left_pos, lightning_left_hand[frame_offset][1], r, left_pos);
	left_pos[2] += lightning_left_hand[frame_offset][2];

	VectorMA(self->s.origin, lightning_right_hand[frame_offset][0], f, right_pos);
	VectorMA(right_pos, lightning_right_hand[frame_offset][1], r, right_pos);
	right_pos[2] += lightning_right_hand[frame_offset][2];

	// Crear efectos de cráneo en ambas manos
	edict_t* left_fire = G_Spawn();
	edict_t* right_fire = G_Spawn();

	VectorCopy(left_pos, left_fire->s.origin);
	VectorCopy(right_pos, right_fire->s.origin);

	left_fire->s.modelindex = gi.modelindex("models/fire/tris.md2");
	right_fire->s.modelindex = gi.modelindex("models/fire/tris.md2");

	left_fire->s.effects |= EF_GIB | EF_ROCKET;
	right_fire->s.effects |= EF_GIB | EF_ROCKET;

	left_fire->s.renderfx |= RF_FULLBRIGHT;
	right_fire->s.renderfx |= RF_FULLBRIGHT;

	left_fire->think = G_FreeEdict;
	right_fire->think = G_FreeEdict;

	left_fire->nextthink = level.time + 0.1;
	right_fire->nextthink = level.time + 0.1;

	gi.linkentity(left_fire);
	gi.linkentity(right_fire);
}

void bskull_touch(edict_t* self, edict_t* other, cplane_t* plane, csurface_t* surf);
//void magicbolt_touch(edict_t* self, edict_t* other, cplane_t* plane, csurface_t* surf);
void fire_shambler_skull(edict_t* self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius)
{
	edict_t* skull;
	skull = G_Spawn();
	VectorCopy(start, skull->s.origin);
	VectorCopy(dir, skull->movedir);
	vectoangles(dir, skull->s.angles);
	VectorScale(dir, speed, skull->velocity);
	skull->movetype = MOVETYPE_FLYMISSILE;
	skull->clipmask = MASK_SHOT;
	skull->solid = SOLID_BBOX;
	VectorClear(skull->mins);
	VectorClear(skull->maxs);
	skull->s.modelindex = gi.modelindex("models/objects/gibs/skull/tris.md2");
	skull->owner = self;
	skull->touch = bskull_touch;
	skull->dmg = damage;
	skull->radius_dmg = 120;
	skull->dmg_radius = damage_radius;
	skull->s.effects = EF_GIB | EF_ROCKET;
	skull->s.sound = gi.soundindex("weapons/rockfly.wav");
	skull->classname = "shambler_skull";
	skull->nextthink = level.time + 8000 / speed;
	skull->think = G_FreeEdict;
	gi.linkentity(skull);
}

void fire_skull(edict_t* self, vec3_t start, vec3_t dir, int damage, int speed, float damage_radius);

void ShamblerCastSkull(edict_t* self)
{
	vec3_t forward, right;
	vec3_t start_left, start_right;
	float accuracy = M_PROJECTILE_ACC;

	if (!G_EntIsAlive(self->enemy))
		return;

	// Get the current frame offset
	int frame_offset = self->s.frame - FRAME_magic1;
	if (frame_offset >= MAX_LIGHTNING_FRAMES)
	{
		frame_offset = MAX_LIGHTNING_FRAMES - 1;
	}

	// Calculate hand positions using the same method as for lightning
	AngleVectors(self->s.angles, forward, right, NULL);

	// Left hand
	VectorMA(self->s.origin, lightning_left_hand[frame_offset][0], forward, start_left);
	VectorMA(start_left, lightning_left_hand[frame_offset][1], right, start_left);
	start_left[2] = self->s.origin[2] + lightning_left_hand[frame_offset][2];

	// Right hand
	VectorMA(self->s.origin, lightning_right_hand[frame_offset][0], forward, start_right);
	VectorMA(start_right, lightning_right_hand[frame_offset][1], right, start_right);
	start_right[2] = self->s.origin[2] + lightning_right_hand[frame_offset][2];

	// Calculate damage
	int damage = 30 + 15 * self->monsterinfo.level;

	// Fire skull from left hand
	MonsterAim(self, accuracy, 1200, false, -1, forward, start_left);
	fire_shambler_skull(self, start_left, forward, damage, 1600, 50);

	// Fire skull from right hand
	MonsterAim(self, accuracy, 1600, false, -1, forward, start_right);
	fire_shambler_skull(self, start_right, forward, damage, 1300, 50);

	// Play sound effect
	gi.sound(self, CHAN_WEAPON, gi.soundindex("spells/circle1.wav"), 1, ATTN_NORM, 0);
}


mframe_t shambler_frames_skull[] = {
	{ai_charge, 0, shambler_windupFire},
	{ai_charge, 0, shambler_fire_update},
	{ai_charge, 0, shambler_fire_update},
	{ai_move, 0, shambler_fire_update},
	{ai_move, 0, shambler_fire_update},
	{ai_move, 0, ShamblerSaveLoc},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, ShamblerCastSkull},
	{ai_move, 0, ShamblerSaveLoc},
	{ai_move, 0, ShamblerCastSkull},
	{ai_charge, 0, NULL},
};
mmove_t shambler_move_skull = { FRAME_magic1, FRAME_magic12, shambler_frames_skull, shambler_run };


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


mframe_t shambler_frames_death[] =
{
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
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

	float r = random();
	float dist = entdist(self, self->enemy);

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
		// self->monsterinfo.currentmove = &shambler_move_skull;
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
	self->monsterinfo.currentmove = &shambler_move_pain;
}

void shambler_dead(edict_t* self)
{
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
	M_PrepBodyRemoval(self);
}

void shambler_die(edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, vec3_t point)
{
	int     n;

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
		if (vrx_spawn_nonessential_ent(self->s.origin))
		{
			for (n = 0; n < 2; n++)
				ThrowGib(self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
			for (n = 0; n < 4; n++)
				ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
			ThrowHead(self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
		}
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
	gi.soundindex("spells/coldcast.wav");

	//fire shambler ( unused )
	// 
	//gi.modelindex("models/objects/gibs/skull/tris.md2");
	//gi.modelindex("models/fire/tris.md2");
	//gi.soundindex("weapons/rockfly.wav");
	//gi.soundindex("spells/circle1.wav");

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