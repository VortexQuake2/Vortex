// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

GLADIATOR

==============================================================================
*/

#include "g_local.h"
#include "m_gladiator.h"
#include "m_flash.h"
#include "shared.h"
#include "horde/g_horde_scaling.h"
#include "monster_constants.h"

static cached_soundindex sound_pain1;
static cached_soundindex sound_pain2;
static cached_soundindex sound_die;
static cached_soundindex sound_die2;
static cached_soundindex sound_gun;
static cached_soundindex sound_gunb;
static cached_soundindex sound_gunc;
static cached_soundindex sound_cleaver_swing;
static cached_soundindex sound_cleaver_hit;
static cached_soundindex sound_cleaver_miss;
static cached_soundindex sound_idle;
static cached_soundindex sound_search;
static cached_soundindex sound_sight;

MONSTERINFO_IDLE(gladiator_idle) (edict_t* self) -> void
{
	gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

MONSTERINFO_SIGHT(gladiator_sight) (edict_t* self, edict_t* other) -> void
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

MONSTERINFO_SEARCH(gladiator_search) (edict_t* self) -> void
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

void gladiator_cleaver_swing(edict_t* self)
{
	gi.sound(self, CHAN_WEAPON, sound_cleaver_swing, 1, ATTN_NORM, 0);
}

mframe_t gladiator_frames_stand[] = {
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand }
};
MMOVE_T(gladiator_move_stand) = { FRAME_stand1, FRAME_stand7, gladiator_frames_stand, nullptr };

MONSTERINFO_STAND(gladiator_stand) (edict_t* self) -> void
{
	M_SetAnimation(self, &gladiator_move_stand);
}

mframe_t gladiator_frames_walk[] = {
	{ ai_walk, 15 },
	{ ai_walk, 7 },
	{ ai_walk, 6 },
	{ ai_walk, 5 },
	{ ai_walk, 2, monster_footstep },
	{ ai_walk },
	{ ai_walk, 2 },
	{ ai_walk, 8 },
	{ ai_walk, 12 },
	{ ai_walk, 8 },
	{ ai_walk, 5 },
	{ ai_walk, 5 },
	{ ai_walk, 2, monster_footstep },
	{ ai_walk, 2 },
	{ ai_walk, 1 },
	{ ai_walk, 8 }
};
MMOVE_T(gladiator_move_walk) = { FRAME_walk1, FRAME_walk16, gladiator_frames_walk, nullptr };

MONSTERINFO_WALK(gladiator_walk) (edict_t* self) -> void
{
	M_SetAnimation(self, &gladiator_move_walk);
}

mframe_t gladiator_frames_run[] = {
	{ ai_run, 23 },
	{ ai_run, 14 },
	{ ai_run, 14, monster_footstep },
	{ ai_run, 21 },
	{ ai_run, 12 },
	{ ai_run, 13, monster_footstep }
};
MMOVE_T(gladiator_move_run) = { FRAME_run1, FRAME_run6, gladiator_frames_run, nullptr };

MONSTERINFO_RUN(gladiator_run) (edict_t* self) -> void
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		M_SetAnimation(self, &gladiator_move_stand);
	else
		M_SetAnimation(self, &gladiator_move_run);
}

void GladiatorMelee(edict_t* self)
{
	if (!M_HasValidTarget(self))
	{
		self->monsterinfo.melee_debounce_time = level.time + 1.5_sec;
		return;
	}

	vec3_t const aim = { MELEE_DISTANCE, self->mins[0], -4 };

	if (fire_hit(self, aim, irandom(30, 35), 300))
	{
		gi.sound(self, CHAN_AUTO, sound_cleaver_hit, 1, ATTN_NORM, 0);
	}
	else
	{
		gi.sound(self, CHAN_AUTO, sound_cleaver_miss, 1, ATTN_NORM, 0);
		self->monsterinfo.melee_debounce_time = level.time + 1.5_sec;
	}
}

mframe_t gladiator_frames_attack_melee[] = {
	{ ai_charge, 0, gladiator_cleaver_swing },
	{ ai_charge, 0, GladiatorMelee },
	{ ai_charge, 0, gladiator_cleaver_swing },
	{ ai_charge, 0, GladiatorMelee },
	{ ai_charge, 0, GladiatorMelee },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, gladiator_cleaver_swing },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, GladiatorMelee },
	{ ai_charge },
	{ ai_charge, 0, GladiatorMelee },
};
MMOVE_T(gladiator_move_attack_melee) = { FRAME_melee3, FRAME_melee16, gladiator_frames_attack_melee, gladiator_run };

MONSTERINFO_MELEE(gladiator_melee) (edict_t* self) -> void
{
	M_SetAnimation(self, &gladiator_move_attack_melee);
}

void GladiatorGun(edict_t* self)
{
	if (!M_HasValidTarget(self))
	{
		return; // Stop immediately if the target is invalid.
	}

	// Railgun is hitscan, requires visibility (no blindfire)
	if (!visible(self, self->enemy))
		return;

	int damage = M_RAILGUN_DMG(self);

	vec3_t start;
	vec3_t dir;
	vec3_t forward, right;

	AngleVectors(self->s.angles, forward, right, nullptr);
	start = M_ProjectFlashSource(self, monster_flash_offset[MZ2_GLADIATOR_RAILGUN_1], forward, right);

	// calc direction to where we targted
	dir = self->pos1 - start;
	dir.normalize();

	monster_fire_railgun(self, start, dir, damage, 100, MZ2_GLADIATOR_RAILGUN_1);
}

void Gladiator_refire_chance(edict_t* self)
{
	if (M_HasValidTarget(self) && frandom() < 0.25f) 
		self->monsterinfo.nextframe = FRAME_attack1; // refire
}

mframe_t gladiator_frames_attack_gun[] = {
	{ ai_charge },
	{ ai_charge, 0, GladiatorGun },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge,0, Gladiator_refire_chance },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, monster_footstep },
	{ ai_charge }
};
MMOVE_T(gladiator_move_attack_gun) = { FRAME_attack1, FRAME_attack9, gladiator_frames_attack_gun, gladiator_run };

// RAFAEL
void gladbGun(edict_t* self)
{
	if (!M_HasValidTarget(self))
	{
		return; // Stop immediately if the target is invalid.
	}

	int damage = M_TRACKER_DMG(self);

	vec3_t start;
	vec3_t dir;
	vec3_t forward, right;
	float  len;

	AngleVectors(self->s.angles, forward, right, nullptr);
	start = G_ProjectSource(self->s.origin, monster_flash_offset[MZ2_GLADIATOR_RAILGUN_1], forward, right);

	// Blindfire support for tracker
	bool blindfire = (self->monsterinfo.aiflags & AI_MANUAL_STEERING);
	vec3_t target_pos;

	if (blindfire && self->monsterinfo.blind_fire_target)
	{
		target_pos = self->monsterinfo.blind_fire_target;
	}
	else
	{
		target_pos = self->pos1;  // Use saved position from attack start
	}

	dir = target_pos - self->enemy->s.origin;
	len = dir.length();

	if (len < 30)
	{
		// calc direction to where we targeted
		dir = target_pos - start;
		dir.normalize();

		monster_fire_tracker(self, start, dir, damage, M_TRACKER_SPEED(self), self->enemy, MZ2_GLADIATOR_RAILGUN_1);
	}
	else
	{
		if (blindfire)
		{
			// Blindfire: aim at blind_fire_target
			dir = (target_pos - start).normalized();
			monster_fire_tracker(self, start, dir, damage, M_TRACKER_SPEED(self), nullptr, MZ2_GLADIATOR_RAILGUN_1);
		}
		else
		{
			// Normal: use predictive aim
			PredictAim(self, self->enemy, start, 980, true, 0, &dir, nullptr);
			monster_fire_tracker(self, start, dir, damage, M_TRACKER_SPEED(self), nullptr, MZ2_GLADIATOR_RAILGUN_1);
		}
	}
}

void gladbGun_check(edict_t* self)
{
	if (skill->integer == 3)
		gladbGun(self);
}

mframe_t gladb_frames_attack_gun[] = {
	{ ai_charge },
	{ ai_charge, 0, gladbGun },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, gladbGun },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, Gladiator_refire_chance }

};
MMOVE_T(gladb_move_attack_gun) = { FRAME_attack1, FRAME_attack9, gladb_frames_attack_gun, gladiator_run };
// HORDE
// RAFAEL
void gladcGun(edict_t* self)
{
	if (!M_HasValidTarget(self))
	{
		return; // Stop immediately if the target is invalid.
	}

	int damage = M_GET_DMG_OR(self, PLASMA, 35);

	int radius_damage = 45;

	vec3_t start;
	vec3_t dir;
	vec3_t forward, right;

	AngleVectors(self->s.angles, forward, right, nullptr);
	start = M_ProjectFlashSource(self, monster_flash_offset[MZ2_GLADIATOR_RAILGUN_1], forward, right);

	// Blindfire support for plasma
	bool blindfire = (self->monsterinfo.aiflags & AI_MANUAL_STEERING);
	vec3_t target_pos;

	if (blindfire && self->monsterinfo.blind_fire_target)
	{
		target_pos = self->monsterinfo.blind_fire_target;
	}
	else
	{
		target_pos = self->pos1;  // Use saved position from attack start
	}

	// calc direction to where we targeted
	dir = target_pos - start;
	dir.normalize();

	if (self->s.frame > FRAME_attack3)
	{
		damage /= 2;
		radius_damage /= 2;
	}
	float const r = frandom();
	fire_plasma(self, start, dir, damage, M_PLASMA_SPEED(self), M_PLASMA_RADIUS(self), M_PLASMA_RADIUS(self));
	if (r < 0.5f && current_wave_level >= 18) {
		fire_plasma(self, start, dir, damage, 1225, M_PLASMA_RADIUS(self), M_PLASMA_RADIUS(self));
	}

	// FIX: Check if the enemy is still valid before updating the aim position.
	// The plasma shot we just fired might have killed it.
	if (self->enemy && self->enemy->inuse)
	{
		// save for aiming the shot
		self->pos1 = self->enemy->s.origin;
		self->pos1[2] += self->enemy->viewheight;
	}
}

void gladcGun_check(edict_t* self)
{
	if (skill->integer == 3)
		gladcGun(self);
}

mframe_t gladc_frames_attack_gun[] = {
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, gladcGun },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, gladcGun },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, gladcGun_check }
};
MMOVE_T(gladc_move_attack_gun) = { FRAME_attack1, FRAME_attack9, gladc_frames_attack_gun, gladiator_run };

// RAFAEL

MONSTERINFO_ATTACK(gladiator_attack) (edict_t* self) -> void
{
	monster_done_dodge(self);

	if (!M_HasValidTarget(self))
	{
		return; // Stop immediately if the target is invalid.
	}

	// Pre-calculate distance once for performance
	vec3_t const v = self->s.origin - self->enemy->s.origin;
	float const range = v.length();
	if (range <= (MELEE_DISTANCE + 32) && self->monsterinfo.melee_debounce_time <= level.time)
		return;
	else if (!M_CheckClearShot(self, monster_flash_offset[MZ2_GLADIATOR_RAILGUN_1]))
		return;

	// charge up the railgun
	self->pos1 = self->enemy->s.origin; // save for aiming the shot
	self->pos1[2] += self->enemy->viewheight;
	// RAFAEL
	if (self->style == 1)
	{
		gi.sound(self, CHAN_WEAPON, sound_gunb, 1, ATTN_NORM, 0);
		M_SetAnimation(self, &gladb_move_attack_gun);
	}
	else 	if (self->style == 3)
	{
		gi.sound(self, CHAN_WEAPON, sound_gun, 1, ATTN_NORM, 0);
		M_SetAnimation(self, &gladc_move_attack_gun);
	}
	else
	{
		// RAFAEL
		gi.sound(self, CHAN_WEAPON, sound_gun, 1, ATTN_NORM, 0);
		M_SetAnimation(self, &gladiator_move_attack_gun);
		// RAFAEL
	}
	// RAFAEL
}

mframe_t gladiator_frames_pain[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(gladiator_move_pain) = { FRAME_pain2, FRAME_pain5, gladiator_frames_pain, gladiator_run };

mframe_t gladiator_frames_pain_air[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(gladiator_move_pain_air) = { FRAME_painup2, FRAME_painup6, gladiator_frames_pain_air, gladiator_run };

PAIN(gladiator_pain) (edict_t* self, edict_t* other, float kick, int damage, const mod_t& mod) -> void
{
	if (level.time < self->pain_debounce_time)
	{
		if ((self->velocity[2] > 100) && (self->monsterinfo.active_move == &gladiator_move_pain))
			M_SetAnimation(self, &gladiator_move_pain_air);
		return;
	}

	self->pain_debounce_time = level.time + 3_sec;

	if (frandom() < 0.5f)
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);

	if (!M_ShouldReactToPain(self, mod))
		return; // no pain anims in nightmare

	if (self->velocity[2] > 100)
		M_SetAnimation(self, &gladiator_move_pain_air);
	else
		M_SetAnimation(self, &gladiator_move_pain);
}

MONSTERINFO_SETSKIN(gladiator_setskin) (edict_t* self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum |= 1;
	else
		self->s.skinnum &= ~1;
}

void gladiator_dead(edict_t* self)
{
	self->mins = { -16, -16, -24 };
	self->maxs = { 16, 16, -8 };
	monster_dead(self);
}

static void gladiator_shrink(edict_t* self)
{
	self->maxs[2] = 0;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}

mframe_t gladiator_frames_death[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, gladiator_shrink },
	{ ai_move, 0, monster_footstep },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(gladiator_move_death) = { FRAME_death2, FRAME_death22, gladiator_frames_death, gladiator_dead };

DIE(gladiator_die) (edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, const vec3_t& point, const mod_t& mod) -> void
{
	//OnEntityDeath(self);
	// check for gib
	if (M_CheckGib(self, mod))
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

		self->s.skinnum /= 2;

		ThrowGibs(self, damage, {
			{ 2, "models/objects/gibs/bone/tris.md2" },
			{ 2, "models/objects/gibs/sm_meat/tris.md2" },
			{ 2, "models/monsters/gladiatr/gibs/thigh.md2", GIB_SKINNED },
			{ "models/monsters/gladiatr/gibs/larm.md2", GIB_SKINNED | GIB_UPRIGHT },
			{ "models/monsters/gladiatr/gibs/rarm.md2", GIB_SKINNED | GIB_UPRIGHT },
			{ "models/monsters/gladiatr/gibs/chest.md2", GIB_SKINNED },
			{ "models/monsters/gladiatr/gibs/head.md2", GIB_SKINNED | GIB_HEAD }
			});
		self->deadflag = true;
		return;
	}

	if (self->deadflag)
		return;

	// regular death
	gi.sound(self, CHAN_BODY, sound_die, 1, ATTN_NORM, 0);

	if (brandom())
		gi.sound(self, CHAN_VOICE, sound_die2, 1, ATTN_NORM, 0);

	self->deadflag = true;
	self->takedamage = true;

	M_SetAnimation(self, &gladiator_move_death);
}

//===========
// PGM
MONSTERINFO_BLOCKED(gladiator_blocked) (edict_t* self, float dist) -> bool
{
	if (blocked_checkplat(self, dist))
		return true;

	// Check if we can jump
	if (auto const result = blocked_checkjump(self, dist); result != blocked_jump_result_t::NO_JUMP)
	{
		if (result != blocked_jump_result_t::JUMP_TURN)
		{
			// Reduced forward velocity to prevent excessive jump distance
			M_MonsterJump(self, 180.0f, 250.0f);
		}
		return true;
	}

	return false;
}
// PGM
//===========

/*QUAKED monster_gladiator (1 .5 0) (-32 -32 -24) (32 32 64) Ambush Trigger_Spawn Sight
*/
void SP_monster_gladiator(edict_t* self)
{
	const spawn_temp_t& st = ED_GetSpawnTemp();

	// Early exit check
	if (!M_AllowSpawn(self)) {
		G_FreeEdict(self);
		return;
	}

	if (self->monsterinfo.monster_type_id == MONSTER_TYPE_UNKNOWN) { // Check if it hasn't been set yet
		self->monsterinfo.monster_type_id = static_cast<uint8_t>(horde::MonsterTypeID::GLADIATOR);
	}	if (g_horde->integer && current_wave_level <= 18)
	{
		const float randomsearch = frandom(); // Generar un número aleatorio entre 0 y 1

		if (randomsearch < 0.23f)
			gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
	}


	// Sound assignments
	sound_pain1.assign("gladiator/pain.wav");
	sound_pain2.assign("gladiator/gldpain2.wav");
	sound_die.assign("gladiator/glddeth2.wav");
	sound_die2.assign("gladiator/death.wav");
	sound_cleaver_swing.assign("gladiator/melee1.wav");
	sound_cleaver_hit.assign("gladiator/melee2.wav");
	sound_cleaver_miss.assign("gladiator/melee3.wav");
	sound_idle.assign("gladiator/gldidle1.wav");
	sound_search.assign("gladiator/gldsrch1.wav");
	sound_sight.assign("gladiator/sight.wav");

	// Model setup
	self->s.modelindex = gi.modelindex("models/monsters/gladiatr/tris.md2");
	gi.modelindex("models/monsters/gladiatr/gibs/chest.md2");
	gi.modelindex("models/monsters/gladiatr/gibs/head.md2");
	gi.modelindex("models/monsters/gladiatr/gibs/larm.md2");
	gi.modelindex("models/monsters/gladiatr/gibs/rarm.md2");
	gi.modelindex("models/monsters/gladiatr/gibs/thigh.md2");

	// Basic monster properties
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->gib_health = -175;
	self->mins = { -32, -32, -24 };
	self->maxs = { 32, 32, 42 };

	// Type-specific configuration based on style
	switch (self->style) {
	case 1: // gladb
		sound_gunb.assign("weapons/disrupt.wav");
		{
			if (g_horde && g_horde->integer && current_wave_level > 0) {
		self->health = M_GLADIATOR_ADDON_HEALTH(self);
	} else {
		self->health = static_cast<int>(M_GLADIATOR_INITIAL_HEALTH * st.health_multiplier);
	}
		}
		self->mass = 350;

		self->s.skinnum = 2;
		self->s.effects = EF_TRACKER;
		self->monsterinfo.weapon_sound = gi.soundindex("weapons/phaloop.wav");
		break;

	case 3: // gladc
		sound_gunc.assign("weapons/plasshot.wav");
		{
			if (g_horde && g_horde->integer && current_wave_level > 0) {
		self->health = M_GLADIATOR_ADDON_HEALTH(self);
	} else {
		self->health = static_cast<int>(M_GLADIATOR_INITIAL_HEALTH * st.health_multiplier);
	}
		}
		self->mass = 350;

		self->s.skinnum = 2;
		self->monsterinfo.weapon_sound = gi.soundindex("weapons/phaloop.wav");
		break;

	default: // normal gladiator
		sound_gun.assign("gladiator/railgun.wav");
		{
			if (g_horde && g_horde->integer && current_wave_level > 0) {
		self->health = M_GLADIATOR_ADDON_HEALTH(self);
	} else {
		self->health = static_cast<int>(M_GLADIATOR_INITIAL_HEALTH * st.health_multiplier);
	}
		}
		self->mass = 400;

		self->monsterinfo.weapon_sound = gi.soundindex("weapons/rg_hum.wav");
		break;
	}

	// Armor setup based on actual monster_type_id (not hardcoded GLADIATOR)
	// This ensures each variant gets the correct armor type from its JSON config
	uint8_t type_id = self->monsterinfo.monster_type_id;

	// Get armor configuration dynamically based on monster_type_id
	int base_armor = GetMonsterBaseArmor(type_id);
	item_id_t power_armor_type = static_cast<item_id_t>(GetMonsterPowerArmorType(type_id));

	// Set power armor if configured
	if (!st.was_key_specified("power_armor_type") && power_armor_type != IT_NULL) {
		self->monsterinfo.power_armor_type = power_armor_type;
		if (!st.was_key_specified("power_armor_power"))
			self->monsterinfo.power_armor_power = GetMonsterScaledPowerArmor(type_id, current_wave_level, self->monsterinfo.IS_BOSS);
	}

	// Set regular armor if configured
	if (!st.was_key_specified("armor_type") && base_armor > 0) {
		self->monsterinfo.armor_type = IT_ARMOR_COMBAT;
		if (!st.was_key_specified("armor_power"))
			self->monsterinfo.armor_power = GetMonsterScaledArmor(type_id, current_wave_level, self->monsterinfo.IS_BOSS);
	}

	// Monster AI/behavior functions
	self->pain = gladiator_pain;
	self->die = gladiator_die;
	self->monsterinfo.stand = gladiator_stand;
	self->monsterinfo.walk = gladiator_walk;
	self->monsterinfo.run = gladiator_run;
	self->monsterinfo.dodge = nullptr;
	self->monsterinfo.attack = gladiator_attack;
	self->monsterinfo.melee = gladiator_melee;
	self->monsterinfo.sight = gladiator_sight;
	self->monsterinfo.idle = gladiator_idle;
	self->monsterinfo.search = gladiator_search;
	self->monsterinfo.blocked = gladiator_blocked;
	self->monsterinfo.setskin = gladiator_setskin;

	// Horde mode specific
	if (g_horde->integer && current_wave_level <= 18) {
		if (frandom() < 0.23f)
			gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
	}

	// Final setup
	gi.linkentity(self);
	M_SetAnimation(self, &gladiator_move_stand);
	self->monsterinfo.scale = MODEL_SCALE;
	walkmonster_start(self);
	ApplyMonsterBonusFlags(self);
}

/*QUAKED monster_gladb (1 .5 0) (-32 -32 -24) (32 32 64) Ambush Trigger_Spawn Sight
*/
void SP_monster_gladb(edict_t* self)
{
	self->style = 1;
	self->monsterinfo.monster_type_id = static_cast<uint8_t>(horde::MonsterTypeID::GLADIATOR_B);
	SP_monster_gladiator(self);
	// All armor and health configuration is handled by SP_monster_gladiator's switch statement
}

void SP_monster_gladc(edict_t* self)
{
	self->style = 3;
	self->monsterinfo.monster_type_id = static_cast<uint8_t>(horde::MonsterTypeID::GLADIATOR_C);
	SP_monster_gladiator(self);
	// All armor and health configuration is handled by SP_monster_gladiator's switch statement
}