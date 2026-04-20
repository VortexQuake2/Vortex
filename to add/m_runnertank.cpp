// Copyright (c) ZeniMax Media Inc.
// Licensed under the GNU General Public License 2.0.
/*
==============================================================================

runnertank

==============================================================================
*/

#include "g_local.h"

#include "m_runnertank.h"

#include "m_flash.h"
#include "shared.h"
#include "horde/g_horde_scaling.h"
#include "monster_constants.h"
void runnertankStrike(edict_t* self);
void runnertank_refire_rocket(edict_t* self);
//void runnetank_doattack_rocket(edict_t* self);
void runnertank_reattack_blaster(edict_t* self);
void runnertank_attack_finished(edict_t* self);
//bool runnertank_check_wall(edict_t* self, float dist);

static cached_soundindex sound_thud;
static cached_soundindex sound_pain, sound_pain2;
static cached_soundindex sound_idle;
static cached_soundindex sound_die;
static cached_soundindex sound_step;
static cached_soundindex sound_sight;
static cached_soundindex sound_windup;
static cached_soundindex sound_strike;

constexpr spawnflags_t SPAWNFLAG_runnertank_COMMANDER_GUARDIAN = 8_spawnflag;
constexpr spawnflags_t SPAWNFLAG_runnertank_COMMANDER_HEAT_SEEKING = 16_spawnflag;

//
// misc
//

MONSTERINFO_SIGHT(runnertank_sight) (edict_t* self, edict_t* other) -> void
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void runnertank_footstep(edict_t* self)
{
brandom() ?	gi.sound(self, CHAN_BODY, sound_step, 1.f, ATTN_NORM, 0)
		:	gi.sound(self, CHAN_BODY, sound_step, 0.75f, ATTN_NORM, 0);
}

void runnertank_thud(edict_t* self)
{
	gi.sound(self, CHAN_BODY, sound_thud, 1, ATTN_NORM, 0);
}

void runnertank_windup(edict_t* self)
{
	gi.sound(self, CHAN_WEAPON, sound_windup, 1, ATTN_NORM, 0);
}

MONSTERINFO_IDLE(runnertank_idle) (edict_t* self) -> void
{
	gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

//
// stand
//

mframe_t runnertank_frames_stand[] = {
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand },
	{ ai_stand }
};
MMOVE_T(runnertank_move_stand) = { FRAME_stand01, FRAME_stand30, runnertank_frames_stand, nullptr };

MONSTERINFO_STAND(runnertank_stand) (edict_t* self) -> void
{
	M_SetAnimation(self, &runnertank_move_stand);
}

//
// walk
//

void runnertank_walk(edict_t* self);

// Animación de caminata corregida
mframe_t runnertank_frames_walk[] = {
	{ ai_walk, 4 }, { ai_walk, 5 }, { ai_walk, 3 }, { ai_walk, 2 },
	{ ai_walk, 5 }, { ai_walk, 5 }, { ai_walk, 4 }, { ai_walk, 4, runnertank_footstep },
	{ ai_walk, 3 }, { ai_walk, 5 }, { ai_walk, 4 }, { ai_walk, 5 },
	{ ai_walk, 7 }, { ai_walk, 7 }, { ai_walk, 6 }, { ai_walk, 6, runnertank_footstep },
	{ ai_walk, 4 }, { ai_walk, 5 }, { ai_walk, 3 }, { ai_walk, 2 },
	{ ai_walk, 5 }, { ai_walk, 5 }, { ai_walk, 4 }, { ai_walk, 4, runnertank_footstep },
	{ ai_walk, 3 }, { ai_walk, 5 }, { ai_walk, 4 }, { ai_walk, 5 },
	{ ai_walk, 7 }, { ai_walk, 7 }, { ai_walk, 6 }, { ai_walk, 6, runnertank_footstep },
	{ ai_walk, 4 }, { ai_walk, 5 }, { ai_walk, 3 }, { ai_walk, 2 },
	{ ai_walk, 4 }, { ai_walk, 5 }
};
MMOVE_T(runnertank_move_walk) = { FRAME_walk01, FRAME_walk38, runnertank_frames_walk, runnertank_walk };


mframe_t runnertank_frames_start_walk[] = {
	{ ai_walk },
	{ ai_walk, 6 },
	{ ai_walk, 6 },
	{ ai_walk, 11, runnertank_footstep },
		{ ai_walk },
	{ ai_walk, 6 },
	{ ai_walk, 6 },
	{ ai_walk, 11, runnertank_footstep }
	,	{ ai_walk },
	{ ai_walk, 6 },
	{ ai_walk, 6 },
	{ ai_walk, 11, runnertank_footstep }
	,	{ ai_walk },
	{ ai_walk, 6 },
	{ ai_walk, 6 },
	{ ai_walk, 11, runnertank_footstep },
		{ ai_walk },
	{ ai_walk, 6 },
	{ ai_walk, 6 },
	{ ai_walk, 11, runnertank_footstep }
		{ ai_walk },
	{ ai_walk, 6 }
};
MMOVE_T(runnertank_move_start_walk) = { FRAME_walk15, FRAME_walk22, runnertank_frames_start_walk, runnertank_walk };

mframe_t runnertank_frames_stop_walk[] = {
	{ ai_walk, 3 },
	{ ai_walk, 3 },
	{ ai_walk, 2 },
	{ ai_walk, 2 },
	{ ai_walk, 4, runnertank_footstep }
};
MMOVE_T(runnertank_move_stop_walk) = { FRAME_walk21, FRAME_walk25, runnertank_frames_stop_walk, runnertank_stand };

void runnertank_run(edict_t* self);

// mframe_t runnertank_frames_start_walk[] = {
// 	{ ai_walk, 0 }, { ai_walk, 3 }, { ai_walk, 3 }, { ai_walk, 3 }
// };
// MMOVE_T(runnertank_move_start_walk) = { FRAME_walk01, FRAME_walk04, runnertank_frames_start_walk, runnertank_walk };

// mframe_t runnertank_frames_stop_walk[] = {
// 	{ ai_walk, 3 }, { ai_walk, 3 }, { ai_walk, 2 }, { ai_walk, 2 },
// 	{ ai_walk, 0, runnertank_footstep }
// };
// MMOVE_T(runnertank_move_stop_walk) = { FRAME_walk34, FRAME_walk38, runnertank_frames_stop_walk, runnertank_stand };

void runnertank_walk_to_run(edict_t* self);
void runnertank_stop_run_to_attack(edict_t* self);

//mframe_t runnertank_frames_start_run[] = {
//	{ ai_run },
//	{ ai_run, 6 },
//	{ ai_run, 6 },
//	{ ai_run, 11, runnertank_footstep }
//};
//MMOVE_T(runnertank_move_start_run) = { FRAME_walk01, FRAME_walk04, runnertank_frames_start_run, runnertank_run };

mframe_t runnertank_frames_start_run[] = {
	{ ai_run, 6 }, { ai_run, 5 }, { ai_run, 6 }, { ai_run, 7, runnertank_footstep }
};
MMOVE_T(runnertank_move_start_run) = { FRAME_walk35, FRAME_walk38, runnertank_frames_start_run, runnertank_walk_to_run };


// Ajustar la función de caminata para una transición más suave
MONSTERINFO_WALK(runnertank_walk) (edict_t* self) -> void
{
	if (self->monsterinfo.active_move != &runnertank_move_walk)
	{
		M_SetAnimation(self, &runnertank_move_start_walk);
	}
	else
	{
		M_SetAnimation(self, &runnertank_move_walk);
	}
}

//
// run
//
//
// Actualizar la animación de carrera
mframe_t runnertank_frames_run[] = {
	{ ai_run, 14, runnertank_footstep }, 
	{ ai_run, 18, nullptr },
	{ ai_run, 15, nullptr },
	{ ai_run, 15, nullptr },
	{ ai_run, 15, nullptr },
	{ ai_run, 19, runnertank_footstep },
	{ ai_run, 15, nullptr },
	{ ai_run, 13, nullptr },
	{ ai_run, 18, nullptr },
	{ ai_run, 17, nullptr }
};
MMOVE_T(runnertank_move_run) = { FRAME_run01, FRAME_run10, runnertank_frames_run, nullptr };


mframe_t runnertank_frames_stop_run[] = {
	{ ai_run, 3 },
	{ ai_run, 3 },
	{ ai_run, 2 },
	{ ai_run, 2 },
	{ ai_run, 4, runnertank_footstep }
};
MMOVE_T(runnertank_move_stop_run) = { FRAME_walk21, FRAME_walk25, runnertank_frames_stop_run, runnertank_stop_run_to_attack };

// Función para manejar la transición de caminata a carrera
void runnertank_walk_to_run(edict_t* self)
{
	if (!M_HasValidTarget(self))
	{
		return;
	}

	if (range_to(self, self->enemy) > RANGE_NEAR)
	{
		M_SetAnimation(self, &runnertank_move_run);
		self->monsterinfo.aiflags |= AI_CHARGING;
	}
	else
	{
		M_SetAnimation(self, &runnertank_move_walk);
	}
}



bool runnertank_enemy_visible(edict_t* self)
{
	return self->enemy && visible(self, self->enemy);
}

void runnertank_attack(edict_t* self);
void runnertank_consider_strafe(edict_t* self);


// Forward declarations for jump attack
void runnertank_jump_attack_takeoff(edict_t* self);
void runnertank_high_gravity(edict_t* self);
void runnertank_check_jump_landing(edict_t* self);

mframe_t tank_frames_punch_attack[] =
{
	{ai_charge, 0, nullptr},
	{ai_charge, 0, nullptr},
	{ai_charge, 0, nullptr},
	{ai_charge, 0, nullptr},
	{ai_charge, 0, runnertankStrike},  // FRAME_attak225 - Añadir footstep aquí
	{ai_charge, 0, nullptr},  // FRAME_attak226 - Engendrar monstruo aquí
	{ai_charge, -1, nullptr},
	{ai_charge, -1, nullptr},
	{ai_charge, -1, nullptr},
	{ai_charge, -1, nullptr},
	{ai_charge, -1, nullptr},
	{ai_charge, -1, nullptr},
	{ai_charge, -1, nullptr},
	{ai_charge, -2, nullptr}   // FRAME_attak229
};
MMOVE_T(tank_move_punch_attack) = { FRAME_attak222, FRAME_attak235, tank_frames_punch_attack, runnertank_run };

// Jump attack animation - using frames that make sense for jumping
mframe_t runnertank_frames_jump_attack[] =
{
	{ai_charge, 15, runnertank_jump_attack_takeoff}, // Launch immediately!
	{ai_move, 0, runnertank_high_gravity}, // In air 1
	{ai_move, 0, runnertank_check_jump_landing}, // Check landing (loops here until landed)
	{ai_move, 0, nullptr}, // Landing recovery 1
	{ai_move, 0, nullptr} // Recovery 2
};
MMOVE_T(runnertank_move_jump_attack) = { FRAME_run01, FRAME_run05, runnertank_frames_jump_attack, runnertank_attack_finished };

MONSTERINFO_MELEE(runnertank_melee) (edict_t* self) -> void
{
	if (!M_HasValidTarget(self))
	{
		return;
	}

	float const range = range_to(self, self->enemy);
	if (!visible(self, self->enemy))
		return;

	// Melee is only for close range punch
	if (range <= MELEE_DISTANCE * 2.4f)
	{
		M_SetAnimation(self, &tank_move_punch_attack);
		self->monsterinfo.attack_finished = level.time + 1.5_sec;
	}
}

MONSTERINFO_RUN(runnertank_run) (edict_t* self) -> void
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
	{
		M_SetAnimation(self, &runnertank_move_stand);
		return;
	}
	else
	{
		M_SetAnimation(self, &runnertank_move_run);
		return;
	}

	// // ALWAYS set animation first to prevent getting stuck
	// if (self->monsterinfo.active_move == &runnertank_move_walk ||
	// 	self->monsterinfo.active_move == &runnertank_move_start_walk)
	// {
	// 	M_SetAnimation(self, &runnertank_move_run);
	// }
	// else if (self->monsterinfo.active_move != &runnertank_move_run &&
	// 		 self->monsterinfo.active_move != &runnertank_move_start_run)
	// {
	// 	M_SetAnimation(self, &runnertank_move_start_run);
	// }

	// 	M_SetAnimation(self, &runnertank_move_run)
	// // Try to attack if we have a valid target and attack cooldown has expired
	// // Attack will override the run animation if successful
	// if (M_HasValidTarget(self) && level.time >= self->monsterinfo.attack_finished)
	// {
	// 		runnertank_attack(self);
	// }
}
//
// pain
//

mframe_t runnertank_frames_pain1[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(runnertank_move_pain1) = { FRAME_pain201, FRAME_pain204, runnertank_frames_pain1, runnertank_run };

mframe_t runnertank_frames_pain3[] = {
	{ ai_move, -7 },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 2 },
	{ ai_move },
	{ ai_move },
	{ ai_move, 3 },
	{ ai_move },
	{ ai_move, 2 },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, runnertank_footstep }
};
MMOVE_T(runnertank_move_pain3) = { FRAME_pain301, FRAME_pain316, runnertank_frames_pain3, runnertank_run };

PAIN(runnertank_pain) (edict_t* self, edict_t* other, float kick, int damage, const mod_t& mod) -> void
{
	if (mod.id != MOD_CHAINFIST && damage <= 10)
		return;

	if (level.time < self->pain_debounce_time)
		return;

	if (mod.id != MOD_CHAINFIST)
	{
		if (damage <= 30)
			if (frandom() > 0.2f)
				return;

		// don't go into pain while attacking
		if ((self->s.frame >= FRAME_attak301) && (self->s.frame <= FRAME_attak330))
			return;
		if ((self->s.frame >= FRAME_attak101) && (self->s.frame <= FRAME_attak116))
			return;
	}

	self->pain_debounce_time = level.time + 3_sec;

	if (self->count)
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

	if (!M_ShouldReactToPain(self, mod))
		return; // no pain anims in nightmare

	// PMM - blindfire cleanup
	self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
	// pmm

	if (damage <= 50)
		M_SetAnimation(self, &runnertank_move_pain1);
	else
		M_SetAnimation(self, &runnertank_move_pain3);
}

MONSTERINFO_SETSKIN(runnertank_setskin) (edict_t* self) -> void
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum |= self->s.skinnum = gi.imageindex("models/monsters/tank/pain.pcx");;
	//else
	//	self->s.skinnum &= ~1;
}

// [Paril-KEX]
bool M_AdjustBlindfireTarget(edict_t* self, const vec3_t& start, const vec3_t& target, const vec3_t& right, vec3_t& out_dir);

//
// attacks
//
// Definimos los offsets específicos para cada frame
struct RailOffset {
	float x;
	float y;
	float z;
};

const RailOffset RAIL_OFFSETS[] = {
	{28.7f, -18.5f, 28.7f},  // FRAME_attak110
	{24.6f, -21.5f, 30.1f},  // FRAME_attak113
	{19.8f, -23.9f, 32.1f}   // FRAME_attak116
};

void runnertankRail(edict_t* self)
{
	if (!M_HasEnemy(self))
	{
		return; // Stop immediately if the enemy is invalid.
	}

	int damage = M_GET_DMG_OR(self, RAILGUN, 45);

	vec3_t forward, right;
	vec3_t start;
	vec3_t dir;
	monster_muzzleflash_id_t flash_number;
	const RailOffset* current_offset;

	// Allow blindfire for rail gun
	bool const blindfire = self->monsterinfo.aiflags & AI_MANUAL_STEERING;
	if (!blindfire)
	{
		if (!M_HasValidTarget(self))
			return;

		if (!infront(self, self->enemy) || !visible(self, self->enemy))
			return;
	}

	// Seleccionamos el offset basado en el frame actual
	if (self->s.frame == FRAME_attak110) {
		flash_number = MZ2_ARACHNID_RAIL2;
		current_offset = &RAIL_OFFSETS[0];
	}
	else if (self->s.frame == FRAME_attak113) {
		flash_number = MZ2_ARACHNID_RAIL2;
		current_offset = &RAIL_OFFSETS[1];
	}
	else { // FRAME_attak116
		flash_number = MZ2_ARACHNID_RAIL2;
		current_offset = &RAIL_OFFSETS[2];
	}

	AngleVectors(self->s.angles, forward, right, nullptr);

	// Creamos un vector temporal para el offset actual
	vec3_t const custom_offset = {
		current_offset->x,
		current_offset->y,
		current_offset->z
	};

	// Usamos el offset personalizado en lugar del monster_flash_offset
	start = M_ProjectFlashSource(self, custom_offset, forward, right);

	vec3_t target;
	if (blindfire) {
		target = self->monsterinfo.blind_fire_target;
		if (!M_AdjustBlindfireTarget(self, start, target, right, dir))
			return;
	}
	else {
		// Check if muzzle origin can see enemy before firing
		vec3_t target_pos = self->enemy->s.origin;
		target_pos[2] += self->enemy->viewheight;
		trace_t trace = gi.traceline(start, target_pos, self, MASK_PROJECTILE);

		if (!(trace.fraction > 0.5f || trace.ent->solid != SOLID_BSP))
			return;

		PredictAim(self, self->enemy, start, 0, false, 0.2f, &dir, nullptr);
	}

	monster_fire_railgun(self, start, dir, damage, 100, flash_number);
}
void runnertankStrike(edict_t* self)
{
	gi.sound(self, CHAN_WEAPON, sound_strike, 1, ATTN_NORM, 0);

	{
		gi.sound(self, CHAN_WEAPON, sound_strike, 1, ATTN_NORM, 0);


		// Efecto visual similar al berserker
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_BERSERK_SLAM);

		vec3_t f, r, start;
		AngleVectors(self->s.angles, f, r, nullptr);
		start = M_ProjectFlashSource(self, { 20.f, -14.3f, -21.f }, f, r);
		trace_t const tr = gi.traceline(self->s.origin, start, self, MASK_SOLID);

		gi.WritePosition(tr.endpos);
		gi.WriteDir({ 0.f, 0.f, 1.f });
		gi.multicast(tr.endpos, MULTICAST_PHS, false);
		int damage = M_GET_DMG_OR(self, SLAM, 45);
		void T_SlamRadiusDamage(vec3_t point, edict_t * inflictor, edict_t * attacker, float damage, float kick, edict_t * ignore, float radius, mod_t mod);
		// Daño radial
		T_SlamRadiusDamage(tr.endpos, self, self, damage, 450.f, self, 165, MOD_TANK_PUNCH);

	}
}



void runnertankRocket(edict_t* self) {
	if (!M_HasEnemy(self))
	{
		return; // Stop immediately if the enemy is invalid.
	}

	int damage = M_GET_DMG_OR(self, ROCKET, 50);

	// Determinar flash number basado en el frame actual
	monster_muzzleflash_id_t const flash_number = static_cast<monster_muzzleflash_id_t>(
		self->s.frame == FRAME_attak324 ? MZ2_TANK_ROCKET_1 :
		self->s.frame == FRAME_attak327 ? MZ2_TANK_ROCKET_2 :
		MZ2_TANK_ROCKET_3);

	// Obtener vectores de dirección usando destructuring
	auto [forward, right, up] = AngleVectors(self->s.angles);

	// Calcular posición de inicio usando M_ProjectFlashSource
	vec3_t const start = M_ProjectFlashSource(self, monster_flash_offset[flash_number], forward, right);

	// Determinar velocidad del cohete
	int config_speed = self->spawnflags.has(SPAWNFLAG_runnertank_COMMANDER_HEAT_SEEKING) ? M_HEAT_SPEED(self) : M_ROCKET_SPEED(self);
	int32_t const rocket_speed = config_speed > 0 ? config_speed :
		(self->speed ? self->speed :
			(self->spawnflags.has(SPAWNFLAG_runnertank_COMMANDER_HEAT_SEEKING) ? 500 : 650));

	// Calcular punto objetivo
	vec3_t target;
	const bool blindfire = self->monsterinfo.aiflags & AI_MANUAL_STEERING;

	if (blindfire) {
		target = self->monsterinfo.blind_fire_target;
		vec3_t dir = target - start;
		if (!M_AdjustBlindfireTarget(self, start, target, right, dir))
			return;
	}
	else {
		if (!M_HasValidTarget(self))
			return;
		// Decidir punto de objetivo basado en posición del enemigo
		if (frandom() < 0.66f || start.z < self->enemy->absmin.z) {
			// Apuntar al centro del cuerpo
			target = self->enemy->s.origin;
			target.z += self->enemy->viewheight;
		}
		else {
			// Apuntar a los pies
			target = self->enemy->s.origin;
			target.z = self->enemy->absmin.z + 1;
		}
	}

	// Calcular dirección base
	vec3_t dir = (target - start).normalized();

	// Predicción de objetivo para disparos no ciegos
	if (!blindfire && frandom() < (0.2f + ((3 - skill->integer) * 0.15f))) {
		PredictAim(self, self->enemy, start, rocket_speed, false, 0, &dir, &target);
	}

	// Verificar línea de visión y disparar
	if (blindfire) {
		if (M_AdjustBlindfireTarget(self, start, target, right, dir)) {
			if (self->spawnflags.has(SPAWNFLAG_runnertank_COMMANDER_HEAT_SEEKING))
				monster_fire_heat(self, start, dir, damage, rocket_speed, flash_number, self->accel);
			else
				monster_fire_rocket(self, start, dir, damage, rocket_speed, flash_number);
		}
	}
	else {
		trace_t const trace = gi.traceline(start, target, self, MASK_PROJECTILE);
		if (trace.fraction > 0.5f || trace.ent->solid != SOLID_BSP) {
			if (self->spawnflags.has(SPAWNFLAG_runnertank_COMMANDER_HEAT_SEEKING))
				monster_fire_heat(self, start, dir, damage, rocket_speed, flash_number, self->accel);
			else
				monster_fire_rocket(self, start, dir, damage, rocket_speed, flash_number);
		}
	}
}

void runnertankPlasmaGun(edict_t* self) {

	if (!M_HasValidTarget(self))
	{
		return; // Stop immediately if the target is invalid.
	}

	int damage = M_GET_DMG_OR(self, PLASMA, 35);

	// Blindfire support for plasma (like gunner ionripper)
	bool blindfire = (self->monsterinfo.aiflags & AI_MANUAL_STEERING);
	vec3_t target;

	if (blindfire)
	{
		// Blindfire mode: use blind_fire_target
		if (!self->monsterinfo.blind_fire_target)
			return;
		target = self->monsterinfo.blind_fire_target;
	}
	else
	{
		// Normal mode: require visibility
		if (!visible(self, self->enemy) || !infront(self, self->enemy))
			return;
		target = self->enemy->s.origin;
	}

	// Constantes del arma
	constexpr float SPREAD = 0.08f;
	constexpr float PREDICTION_TIME = 0.2f;
	constexpr float PROJECTILE_SPEED = 700.0f;
	constexpr float PLASMA_RADIUS = 40.0f;

	// Calcular flash number basado en el frame
	monster_muzzleflash_id_t const flash_number = static_cast<monster_muzzleflash_id_t>
		(MZ2_TANK_MACHINEGUN_1 + (self->s.frame - FRAME_attak406));

	// Use current angles for muzzle position - don't force rotation before firing
	vec3_t forward, right, up;
	AngleVectors(self->s.angles, &forward, &right, &up);

	// Calcular posición de inicio
	vec3_t const start = M_ProjectFlashSource(self, monster_flash_offset[flash_number], forward, right);

	// Check if muzzle origin can see enemy before firing (skip for blindfire)
	if (!blindfire)
	{
		vec3_t check_pos = self->enemy->s.origin;
		check_pos[2] += self->enemy->viewheight;
		trace_t trace = gi.traceline(start, check_pos, self, MASK_PROJECTILE);

		if (!(trace.fraction > 0.5f || trace.ent->solid != SOLID_BSP))
			return;
	}

	// Calcular dirección base al objetivo
	target = self->enemy->s.origin;
	target.z += self->enemy->viewheight;
	vec3_t dir = (target - start).normalized();

	// Añadir dispersión a la dirección
	dir += vec3_t{
		crandom() * SPREAD,
		crandom() * SPREAD,
		crandom() * SPREAD
	};
	dir.normalize();

	// Predicción de movimiento del objetivo
	PredictAim(self, self->enemy, start, PROJECTILE_SPEED, false,
		PREDICTION_TIME, &dir, nullptr);

	// Disparar el proyectil de plasma
	fire_plasma(self, start, dir, damage, PROJECTILE_SPEED,
		PLASMA_RADIUS, PLASMA_RADIUS);

	// Actualizar posición del último disparo
	self->pos1 = target;
}

//static void runnertank_blind_check(edict_t* self)
//{
//	if (self->monsterinfo.aiflags & AI_MANUAL_STEERING)
//	{
//		const vec3_t aim = self->monsterinfo.blind_fire_target - self->s.origin;
//		self->ideal_yaw = vectoyaw(aim);
//	}
//}

mframe_t runnertank_frames_attack_blast[] = {
    { ai_charge }, { ai_charge }, { ai_charge }, { ai_charge },
    { ai_charge }, { ai_charge }, { ai_charge }, { ai_charge },
    { ai_charge }, { ai_charge, 0, runnertankRail },
    { ai_charge }, { ai_charge }, { ai_charge, 0, runnertankRail },
    { ai_charge }, { ai_charge }, { ai_charge, 0, runnertankRail },
    { ai_charge }, { ai_charge }, { ai_charge }, { ai_charge },
    { ai_charge }, { ai_charge }
};
MMOVE_T(runnertank_move_attack_blast) = { FRAME_attak101, FRAME_attak122, runnertank_frames_attack_blast, runnertank_reattack_blaster };

mframe_t runnertank_frames_reattack_blast[] = {
	{ ai_charge },
	{ ai_charge, 0, runnertankRail },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, runnertankRail },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge, 0, runnertankRail } // 16
};
MMOVE_T(runnertank_move_reattack_blast) = { FRAME_attak111, FRAME_attak122, runnertank_frames_reattack_blast, runnertank_reattack_blaster };

mframe_t runnertank_frames_attack_post_blast[] = {
	{ ai_move }, // 17
	{ ai_move },
	{ ai_move, 2 },
	{ ai_move, 3 },
	{ ai_move, 2 },
	{ ai_move, -2, runnertank_footstep } // 22
};
MMOVE_T(runnertank_move_attack_post_blast) = { FRAME_attak117, FRAME_attak122, runnertank_frames_attack_post_blast, runnertank_attack_finished };


// Called when attack animations finish - adds cooldown before next attack
void runnertank_attack_finished(edict_t* self)
{
	// Add random cooldown after attack ends
	self->monsterinfo.attack_finished = level.time + random_time(0.4_sec, 2.3_sec);
	runnertank_run(self);
}

void runnertank_reattack_blaster(edict_t* self)
{
	if (!M_HasValidTarget(self))
	{
		M_SetAnimation(self, &runnertank_move_attack_post_blast);
		return;
	}

	if (self->monsterinfo.aiflags & AI_MANUAL_STEERING)
	{
		self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
		M_SetAnimation(self, &runnertank_move_attack_post_blast);
		return;
	}

	// Check if we've been attacking too long
	if (level.time >= self->monsterinfo.attack_finished)
	{
		M_SetAnimation(self, &runnertank_move_attack_post_blast);
		return;
	}

	// Reduce refire chance to prevent constant shooting
	if (visible(self, self->enemy))
		if (self->enemy->health > 0)
			if (frandom() <= 0.35f) // Reduced from 0.6f
			{
				M_SetAnimation(self, &runnertank_move_reattack_blast);
				return;
			}
	M_SetAnimation(self, &runnertank_move_attack_post_blast);
}

void runnertank_doattack_rocket(edict_t* self);

void runnertank_poststrike(edict_t* self)
{
	self->enemy = nullptr;
	// [Paril-KEX]
	self->monsterinfo.pausetime = HOLD_FOREVER;
	self->monsterinfo.stand(self);
}

mframe_t runnertank_frames_attack_strike[] = {
	{ ai_charge }, { ai_charge }, { ai_charge }, { ai_charge },
	{ ai_charge }, { ai_charge }, { ai_charge }, { ai_charge },
	{ ai_charge }, { ai_charge }, { ai_charge }, { ai_charge },
	{ ai_charge }, { ai_charge }, { ai_charge }, { ai_charge },
	{ ai_charge }, { ai_charge }, { ai_charge }, { ai_charge },
	{ ai_charge, 0, runnertankStrike }, { ai_charge }, { ai_charge },
	{ ai_charge }, { ai_charge }, { ai_charge }, { ai_charge },
	{ ai_charge }, { ai_charge }, { ai_charge }, { ai_charge },
	{ ai_charge }, { ai_charge }, { ai_charge }, { ai_charge },
	{ ai_charge }, { ai_charge }, { ai_charge }
};
MMOVE_T(runnertank_move_attack_strike) = { FRAME_attak201, FRAME_attak238, runnertank_frames_attack_strike, runnertank_poststrike };

mframe_t runnertank_frames_attack_pre_rocket[] = {
	{ ai_charge },	{ ai_charge },
	{ ai_charge }, { ai_charge }, { ai_charge }, { ai_charge },
	{ ai_charge, 0, ai_charge }, { ai_charge }, { ai_charge, 0, runnertankRocket }, { ai_charge },
	{ ai_charge, 0, runnertankRocket }, { ai_charge }, { ai_charge }, { runnertankRocket, 0, ai_charge },
	{ ai_charge }, { ai_charge }, { ai_charge, 0, ai_charge }, { ai_charge, 0, ai_charge },
	{ ai_charge }
};
MMOVE_T(runnertank_move_attack_pre_rocket) = { FRAME_attak303, FRAME_attak321, runnertank_frames_attack_pre_rocket, runnertank_doattack_rocket };

mframe_t runnertank_frames_attack_fire_rocket[] = {
	{ ai_charge }, { runnertankRocket },
	{ ai_charge, 0, ai_charge },
	 { runnertankRocket, 0, ai_charge },
	 { ai_charge, 0, runnertankRocket },
	 	 { ai_charge, 0, ai_charge }
};
MMOVE_T(runnertank_move_attack_fire_rocket) = { FRAME_attak312, FRAME_attak322, runnertank_frames_attack_fire_rocket, runnertank_refire_rocket };

mframe_t runnertank_frames_attack_post_rocket[] = {

	{ ai_charge, -9 },
	{ ai_charge, -8 },
	{ ai_charge, -7 },
	{ ai_charge, -1 },
	{ ai_charge, -1, runnertank_footstep },
	{ ai_charge },
	{ ai_charge },
	{ ai_charge }, // 50
	{ ai_charge },
	{ ai_charge }
};
MMOVE_T(runnertank_move_attack_post_rocket) = { FRAME_attak326, FRAME_attak335, runnertank_frames_attack_post_rocket, runnertank_attack_finished };


void runnertank_refire_rocket(edict_t* self)
{
	// If the enemy is gone, the tank should finish its attack animation.
	if (!M_HasValidTarget(self))
	{
		M_SetAnimation(self, &runnertank_move_attack_post_rocket);
		return;
	}

	// PMM - blindfire cleanup
	if (self->monsterinfo.aiflags & AI_MANUAL_STEERING)
	{
		self->monsterinfo.aiflags &= ~AI_MANUAL_STEERING;
		M_SetAnimation(self, &runnertank_move_attack_post_rocket);
		return;
	}
	// pmm

	// Check if we've been attacking too long
	if (level.time >= self->monsterinfo.attack_finished)
	{
		M_SetAnimation(self, &runnertank_move_attack_post_rocket);
		return;
	}

	if (self->enemy->health > 0)
		if (visible(self, self->enemy))
			if (frandom() <= 0.3f) // Reduced from 0.4f
			{
				M_SetAnimation(self, &runnertank_move_attack_fire_rocket);
				return;
			}
	M_SetAnimation(self, &runnertank_move_attack_post_rocket);
}

void runnertank_doattack_rocket(edict_t* self)
{
	M_SetAnimation(self, &runnertank_move_attack_fire_rocket);
}


mframe_t runnertank_frames_attack_chain[] = {
	{ ai_charge },
	{ ai_charge }, 
	{ ai_charge, 0, runnertankPlasmaGun },
	{ ai_charge, 0, runnertankPlasmaGun },
	{ ai_charge, 0, runnertankPlasmaGun },
	{ ai_charge, 0, runnertankPlasmaGun },
	{ ai_charge, 0, runnertankPlasmaGun },
	{ ai_charge, 0, runnertankPlasmaGun },
	{ ai_charge, 0, runnertankPlasmaGun },
	{ ai_charge, 0, runnertankPlasmaGun },
	{ ai_charge, 0, runnertankPlasmaGun },
	{ ai_charge }
};
MMOVE_T(runnertank_move_attack_chain) = { FRAME_attak404, FRAME_attak415, runnertank_frames_attack_chain, runnertank_attack_finished };

void runnertank_stop_run_to_attack(edict_t* self)
{
	if (!M_HasValidTarget(self))
	{
		M_SetAnimation(self, &runnertank_move_run);
		return;
	}

	// Don't attack if we're still in cooldown
	if (level.time < self->monsterinfo.attack_finished)
	{
		M_SetAnimation(self, &runnertank_move_run);
		return;
	}

	if (range_to(self, self->enemy) <= RANGE_NEAR && visible(self, self->enemy))
	{
		M_SetAnimation(self, &runnertank_move_attack_pre_rocket);
		self->monsterinfo.attack_finished = level.time + 3_sec;
	}
	else
	{
		M_SetAnimation(self, &runnertank_move_run);
	}
}

void runnertank_consider_strafe(edict_t* self)
{
	// No strafear si estamos en medio de un ataque
	if (self->monsterinfo.active_move == &runnertank_move_attack_blast ||
		self->monsterinfo.active_move == &runnertank_move_attack_pre_rocket ||
		self->monsterinfo.active_move == &runnertank_move_attack_fire_rocket ||
		self->monsterinfo.active_move == &tank_move_punch_attack)
		return;

	// Use the comprehensive check
	if (!M_HasValidTarget(self))
		return;

	// Don't strafe if we're still in strafe pause
	if (level.time < self->monsterinfo.pausetime)
		return;

	float strafe_chance = 0.4f; // Increased base chance for more responsive movement

	// Increase probability in critical situations
	if (self->enemy && self->enemy->client && (self->enemy->client->buttons & BUTTON_ATTACK))
		strafe_chance += 0.3f;
	if (self->health < self->max_health * 0.6f)
		strafe_chance += 0.25f;

	// Distance-based strafing: strafe more when close to enemy
	if (self->enemy) {
		float dist = (self->s.origin - self->enemy->s.origin).length();
		if (dist < 512.0f) // Close range
			strafe_chance += 0.2f;
		else if (dist > 1024.0f) // Long range - less strafing
			strafe_chance -= 0.1f;
	}

	// Clamp the chance
	strafe_chance = std::clamp(strafe_chance, 0.1f, 0.85f);

	if (frandom() < strafe_chance)
	{
		// Decide direction - consistent integer usage
		self->monsterinfo.lefty = (frandom() < 0.5f) ? 1 : 0;

		// Calculate strafe direction
		vec3_t right;
		AngleVectors(self->s.angles, nullptr, right, nullptr);

		// Calculate strafe speed based on health and situation
		float strafe_speed = 200.0f;

		// Boost speed when in danger
		if (self->health < self->max_health * 0.5f)
			strafe_speed *= 1.4f;

		// Add some randomness to make movement less predictable
		strafe_speed += frandom() * 100.0f;

		// Apply strafe velocity - replace current lateral movement
		vec3_t strafe_velocity = right * (self->monsterinfo.lefty ? -strafe_speed : strafe_speed);

		// Preserve forward/backward movement but replace lateral movement
		vec3_t forward;
		AngleVectors(self->s.angles, forward, nullptr, nullptr);
		float forward_speed = self->velocity.dot(forward);

		// Set new velocity: keep forward momentum, add strafe
		self->velocity = forward * forward_speed + strafe_velocity;

		// Moderate strafe duration
		self->monsterinfo.pausetime = level.time + random_time(0.8_sec, 1.5_sec);
	}
}


// Custom checkattack with higher attack chances than default
// Default is: stand=0.7, melee=0.4, near=0.25, mid=0.06, far=0.0
// Runnertank is aggressive and should attack more often
MONSTERINFO_CHECKATTACK(runnertank_checkattack) (edict_t* self) -> bool
{
	return M_CheckAttack_Base(self, 0.8f, 0.5f, 0.4f, 0.25f, 0.15f, 1.0f);
}

MONSTERINFO_ATTACK(runnertank_attack) (edict_t* self) -> void
{
	monster_done_dodge(self);

	if (!M_HasValidTarget(self))
	{
		return; // Can't attack a non-existent or dead target.
	}

	if (level.time < self->monsterinfo.attack_finished)
		return;

	// Check for blindfire conditions
	if (self->monsterinfo.attack_state == AS_BLIND)
	{
		// Blindfire logic - attack without visibility requirement
		float chance;
		if (self->monsterinfo.blind_fire_delay < 1_sec)
			chance = 0.8f;
		else if (self->monsterinfo.blind_fire_delay < 7.5_sec)
			chance = 0.4f;
		else
			chance = 0.1f;

		if (frandom() > chance)
			return;

		self->monsterinfo.blind_fire_delay += 5.2_sec + random_time(3_sec);
		self->monsterinfo.aiflags |= AI_MANUAL_STEERING;
	}
	else if (!visible(self, self->enemy))
	{
		// Regular attack needs visibility
		return;
	}

	//const float range = range_to(self, self->enemy);
	const float r = frandom();

	// Verificar líneas de visión para cada tipo de ataque
	const bool can_blast = M_CheckClearShot(self, monster_flash_offset[MZ2_TANK_BLASTER_1]);
	const bool can_rocket = M_CheckClearShot(self, monster_flash_offset[MZ2_TANK_ROCKET_1]);
	const bool can_chain = M_CheckClearShot(self, monster_flash_offset[MZ2_TANK_MACHINEGUN_1]);

	float const range = range_to(self, self->enemy);

	// Jump attack - check first for medium range leap attack
	// Range between 150-400 units, visible enemy, on ground
	if (range > 150.0f && range <= 400.0f &&
		self->groundentity && visible(self, self->enemy) &&
		frandom() < 0.25f) // 25% chance
	{
		// Check if enemy is at similar height or slightly above
		float height_diff = self->enemy->s.origin[2] - self->s.origin[2];
		if (height_diff > -50.0f && height_diff < 150.0f)
		{
			M_SetAnimation(self, &runnertank_move_jump_attack);
			self->monsterinfo.attack_finished = level.time + 3_sec;
			return;
		}
	}

	// Close range attack selection
	if (range <= RANGE_MELEE * 1.5f)
	{
		M_SetAnimation(self, &tank_move_punch_attack);
		self->monsterinfo.attack_finished = level.time + 1.5_sec;
	}
	else if (range <= RANGE_NEAR)
	{
		if (can_chain && r < 0.5f)
		{
			M_SetAnimation(self, &runnertank_move_attack_chain);
			self->monsterinfo.attack_finished = level.time + 3_sec;
		}
		else if (can_rocket && r < 0.7f)
		{
			M_SetAnimation(self, &runnertank_move_attack_pre_rocket);
			self->monsterinfo.attack_finished = level.time + 4_sec;
		}
		else if (can_blast)
		{
			M_SetAnimation(self, &runnertank_move_attack_blast);
			self->monsterinfo.attack_finished = level.time + 3_sec;
		}
		else
		{
			// If no clear shot, let normal AI handle movement
			return;
		}
	}
	// Medium range attack selection
	else if (range <= RANGE_MID)
	{
		if (can_blast && r < 0.4f)
		{
			M_SetAnimation(self, &runnertank_move_attack_blast);
			self->monsterinfo.attack_finished = level.time + 3_sec;
		}
		else if (can_rocket && r < 0.7f)
		{
			M_SetAnimation(self, &runnertank_move_attack_pre_rocket);
			self->monsterinfo.attack_finished = level.time + 4_sec;
		}
		else if (can_chain)
		{
			M_SetAnimation(self, &runnertank_move_attack_chain);
			self->monsterinfo.attack_finished = level.time + 3_sec;
		}
		else
		{
			// If no clear shot, let normal AI handle movement
			return;
		}
	}
	// Long range attack selection
	else
	{
		if (can_blast && r < 0.6f)
		{
			M_SetAnimation(self, &runnertank_move_attack_blast);
			self->monsterinfo.attack_finished = level.time + 3.5_sec;
		}
		else if (can_rocket)
		{
			M_SetAnimation(self, &runnertank_move_attack_pre_rocket);
			self->monsterinfo.attack_finished = level.time + 4_sec;
		}
		else
		{
			// If no attack is possible, let normal AI handle movement
			return;
		}
	}
}
//
// death
//

void runnertank_dead(edict_t* self)
{
	self->mins = { -16, -16, -16 };
	self->maxs = { 16, 16, -0 };
	monster_dead(self);
}

static void runnertank_shrink(edict_t* self)
{
	self->maxs[2] = 0;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}

mframe_t runnertank_frames_death1[] = {
	{ ai_move, -7 },
	{ ai_move, -2 },
	{ ai_move, -2 },
	{ ai_move, 1 },
	{ ai_move, 3 },
	{ ai_move, 6 },
	{ ai_move, 1 },
	{ ai_move, 1 },
	{ ai_move, 2 },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, -2 },
	{ ai_move },
	{ ai_move },
	{ ai_move, -3 },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, -4 },
	{ ai_move, -6 },
	{ ai_move, -4 },
	{ ai_move, -5 },
	{ ai_move, -7, runnertank_shrink },
	{ ai_move, -15, runnertank_thud },
	{ ai_move, -5 },
	{ ai_move },
	{ ai_move },
	{ ai_move }
};
MMOVE_T(runnertank_move_death) = { FRAME_death01, FRAME_death32, runnertank_frames_death1, runnertank_dead };

DIE(runnertank_die) (edict_t* self, edict_t* inflictor, edict_t* attacker, int damage, const vec3_t& point, const mod_t& mod) -> void
{
	// check for gib
	if (M_CheckGib(self, mod))
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);

		self->s.skinnum /= 2;

		ThrowGibs(self, damage, {
			{ "models/objects/gibs/sm_meat/tris.md2" },
			{ 3, "models/objects/gibs/sm_metal/tris.md2", GIB_METALLIC },
			{ "models/objects/gibs/gear/tris.md2", GIB_METALLIC },
			{ 2, "models/monsters/tank/gibs/foot.md2", GIB_SKINNED | GIB_METALLIC },
			{ 2, "models/monsters/tank/gibs/thigh.md2", GIB_SKINNED | GIB_METALLIC },
			{ "models/monsters/tank/gibs/chest.md2", GIB_SKINNED },
			{ "models/monsters/tank/gibs/head.md2", GIB_HEAD | GIB_SKINNED }
			});

		if (!self->style)
			ThrowGib(self, "models/monsters/tank/gibs/barm.md2", damage, GIB_SKINNED | GIB_UPRIGHT, self->s.scale);

		self->deadflag = true;
		return;
	}

	if (self->deadflag)
		return;

	// [Paril-KEX] dropped arm
	if (!self->style)
	{
		self->style = 1;

		auto [fwd, rgt, up] = AngleVectors(self->s.angles);

		edict_t* arm_gib = ThrowGib(self, "models/monsters/tank/gibs/barm.md2", damage, GIB_SKINNED | GIB_UPRIGHT, self->s.scale);

		if (!arm_gib) {
			return;
		}

		arm_gib->s.origin = self->s.origin + (rgt * -16.f) + (up * 23.f);
		arm_gib->s.old_origin = arm_gib->s.origin;
		arm_gib->avelocity = { crandom() * 15.f, crandom() * 15.f, 180.f };
		arm_gib->velocity = (up * 100.f) + (rgt * -120.f);
		arm_gib->s.angles = self->s.angles;
		arm_gib->s.angles[2] = -90.f;
		arm_gib->s.skinnum /= 2;
		gi.linkentity(arm_gib);
	}

	// regular death
	gi.sound(self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
	self->deadflag = true;
	self->takedamage = true;

	M_SetAnimation(self, &runnertank_move_death);
}
void runnertank_jump_now(edict_t* self)
{
    auto [forward, right, up] = AngleVectors(self->s.angles);
    
    // Usar operadores vec3_t para impulso
    self->velocity += forward * 130.0f + up * 300.0f;
}

void runnertank_jump2_now(edict_t* self)
{
	auto [forward, right, up] = AngleVectors(self->s.angles);

	// Usar operadores vec3_t para impulso
	self->velocity += forward * 250.0f + up * 400.0f;
}

void runnertank_jump_wait_land(edict_t* self)
{
	if (self->groundentity == nullptr)
	{
		self->monsterinfo.nextframe = self->s.frame;

		if (monster_jump_finished(self))
			self->monsterinfo.nextframe = self->s.frame + 1;
	}
	else
		self->monsterinfo.nextframe = self->s.frame + 1;
}

mframe_t runnertank_frames_jump[] = {
	{ ai_move },
	{ ai_move },
	{ ai_move },
	{ ai_move, 0, runnertank_jump_now },
	{ ai_move },
	{ ai_move, 0, runnertank_jump_wait_land },
	{ ai_move }
};
MMOVE_T(runnertank_move_jump) = { FRAME_run01, FRAME_run07, runnertank_frames_jump, runnertank_run };

mframe_t runnertank_frames_jump2[] = {
	{ ai_move, -6 },
	{ ai_move, -4 },
	{ ai_move, -5 },
	{ ai_move, 0, runnertank_jump2_now },
	{ ai_move },
	{ ai_move, 0, runnertank_jump_wait_land },
	{ ai_move }
};
MMOVE_T(runnertank_move_jump2) = { FRAME_run01, FRAME_run07, runnertank_frames_jump2, runnertank_run };

// Jump attack functions implementation
void runnertank_jump_attack_takeoff(edict_t* self)
{
	if (!M_HasValidTarget(self))
		return;

	// Calculate jump trajectory to enemy
	vec3_t enemy_pos = self->enemy->s.origin;
	vec3_t dir_to_enemy = enemy_pos - self->s.origin;
	float const length = dir_to_enemy.length();

	// Adjust for enemy movement prediction
	float const jump_time = length / 400.0f; // Estimate time to reach enemy
	if (self->enemy->velocity.length() > 50.0f)
	{
		enemy_pos += self->enemy->velocity * jump_time * 0.5f; // Partial prediction
		dir_to_enemy = enemy_pos - self->s.origin;
	}

	// Calculate horizontal and vertical speeds
	float const horizontal_speed = length * 1.5f; // Adjust multiplier as needed
	float const vertical_speed = 200.0f + (length * 0.3f); // Higher for longer jumps

	// Face the target
	self->s.angles[1] = vectoyaw(dir_to_enemy);
	vec3_t forward;
	AngleVectors(self->s.angles, forward, nullptr, nullptr);

	// Launch!
	self->s.origin[2] += 1;
	self->velocity = forward * horizontal_speed;
	self->velocity[2] = vertical_speed;
	self->groundentity = nullptr;
	self->monsterinfo.aiflags |= AI_DUCKED;

	// Play a sound effect if we have one
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

void runnertank_high_gravity(edict_t* self)
{
	float const gravity_scale = (800.f / level.gravity);
	if (self->velocity[2] < 0)
		self->gravity = 2.0f;
	else
		self->gravity = 4.5f;
	self->gravity *= gravity_scale;
}

void runnertank_check_jump_landing(edict_t* self)
{
	runnertank_high_gravity(self);

	if (self->groundentity)
	{
		// Landed - do slam attack immediately
		self->monsterinfo.aiflags &= ~AI_DUCKED;
		self->gravity = 1.0f;
		self->velocity = {};
		self->flags &= ~FL_KILL_VELOCITY;

		// Always do the slam on landing - this is a jump attack!
		runnertankStrike(self);
		
		// Continue with the animation
		return;
	}

	// Still in air - keep checking
	self->monsterinfo.nextframe = self->s.frame;
}

void runnertank_jump(edict_t* self, blocked_jump_result_t result)
{
	if (!M_HasValidTarget(self))
		return; // Can't jump at a non-existent or dead target.

	monster_done_dodge(self);

	if (result == blocked_jump_result_t::JUMP_JUMP_UP)
		M_SetAnimation(self, &runnertank_move_jump2);
	else
		M_SetAnimation(self, &runnertank_move_jump);
}
//===========
// PGM

//bool runnertank_check_wall(edict_t* self, float dist)
//{
//	auto [forward, right, up] = AngleVectors(self->s.angles);
//
//	// Usar operador + de vec3_t para punto de verificación
//	vec3_t const check_point = self->s.origin + (forward * (dist + 10.0f));
//
//	trace_t const tr = gi.trace(self->s.origin, self->mins, self->maxs,
//		check_point, self, MASK_MONSTERSOLID);
//
//	if (tr.fraction < 1.0f) {
//		// Usar dot() de vec3_t
//		float const dot = forward.dot(tr.plane.normal);
//		float const turn_angle = 90.0f * (1.0f - std::abs(dot));
//
//		// Actualizar orientación usando el resultado
//		float new_yaw = self->s.angles[YAW] + (dot < 0 ? turn_angle : -turn_angle);
//		float const yaw_diff = AngleDifference(new_yaw, self->ideal_yaw);
//
//		if (std::abs(yaw_diff) > 30.0f)
//			new_yaw = self->s.angles[YAW] + (yaw_diff > 0 ? 30.0f : -30.0f);
//
//		self->ideal_yaw = anglemod(new_yaw);
//		M_ChangeYaw(self);
//
//		// Ajustar velocidad usando operador * de vec3_t
//		self->velocity = self->velocity * (tr.fraction * 0.8f);
//
//		return true;
//	}
//	return false;
//}

MONSTERINFO_BLOCKED(runnertank_blocked) (edict_t* self, float dist) -> bool
{
	if (self->monsterinfo.can_jump)
	{
		if (const auto result = blocked_checkjump(self, dist); result != blocked_jump_result_t::NO_JUMP)
		{
			runnertank_jump(self, result);
			return true;
		}
	}

	if (blocked_checkplat(self, dist))
		return true;

	// Eliminar la llamada a runnertank_check_wall aquí
	// if (runnertank_check_wall(self, dist))
	// {
	//     ai_turn(self, 0);
	//     return true;
	// }

	return false;
}

// PGM
// 
//===========

//
// monster_runnertank
//

MONSTERINFO_SIDESTEP(runnertank_sidestep) (edict_t* self) -> bool
{
	// No hacer sidestep si estamos en medio de un ataque
	if ((self->monsterinfo.active_move == &runnertank_move_attack_blast) ||
		(self->monsterinfo.active_move == &runnertank_move_attack_pre_rocket) ||
		(self->monsterinfo.active_move == &runnertank_move_attack_fire_rocket) ||
		(self->monsterinfo.active_move == &tank_move_punch_attack) ||
		(self->monsterinfo.active_move == &runnertank_move_jump_attack))
	{
		return false;
	}

	// Don't sidestep if we just did one recently
	if (level.time < self->monsterinfo.pausetime)
		return false;

	// Si no estamos corriendo, cambiar a la animación de carrera
	if (self->monsterinfo.active_move != &runnertank_move_run)
		M_SetAnimation(self, &runnertank_move_run);

	// Consistent integer usage for lefty
	self->monsterinfo.lefty = (frandom() < 0.5f) ? 1 : 0;

	// Calculate strafe direction
	auto [forward, right, up] = AngleVectors(self->s.angles);

	// More dynamic speed calculation
	float base_speed = 250.0f;
	float speed_variation = frandom() * 150.0f;
	float const strafe_speed = base_speed + speed_variation;

	// Enhanced evasion: check if enemy is aiming/attacking
	float evasion_multiplier = 1.0f;
	if (self->enemy && self->enemy->client) {
		// Boost evasion if enemy is attacking
		if (self->enemy->client->buttons & BUTTON_ATTACK)
			evasion_multiplier = 1.6f;

		// Also consider enemy's facing direction for better dodging
		vec3_t enemy_forward;
		AngleVectors(self->enemy->s.angles, enemy_forward, nullptr, nullptr);
		vec3_t to_self = (self->s.origin - self->enemy->s.origin).normalized();

		// If enemy is facing us, dodge more aggressively
		if (enemy_forward.dot(to_self) > 0.7f)
			evasion_multiplier *= 1.3f;
	}

	// Apply enhanced movement
	vec3_t dodge_velocity = right * (self->monsterinfo.lefty ? -strafe_speed : strafe_speed) * evasion_multiplier;

	// Add slight forward/backward component for more unpredictable movement
	float fb_component = (frandom() - 0.5f) * 100.0f;
	dodge_velocity += forward * fb_component;

	// Replace lateral velocity but preserve some vertical momentum
	vec3_t preserved_velocity = {0, 0, self->velocity.z};
	self->velocity = dodge_velocity + preserved_velocity;

	// Variable pause time based on evasion intensity
	self->monsterinfo.pausetime = level.time + random_time(0.8_sec, 1.5_sec);

	return true;
}

/*QUAKED monster_runnertank (1 .5 0) (-32 -32 -16) (32 32 72) Ambush Trigger_Spawn Sight
model="models/monsters/runnertank/tris.md2"
*/
/*QUAKED monster_runnertank_commander (1 .5 0) (-32 -32 -16) (32 32 72) Ambush Trigger_Spawn Sight Guardian HeatSeeking
 */
MONSTERINFO_DODGE(runnertank_dodge) (edict_t* self, edict_t* attacker, gtime_t eta, trace_t* tr, bool gravity) -> void
{
	// Basic checks
	if (!self->groundentity || self->health <= 0)
		return;

	// Set enemy if we don't have one
	if (!self->enemy && attacker)
	{
		self->enemy = attacker;
		FoundTarget(self);
		return;
	}

	// Don't dodge if we're attacking melee or jump attacking
	if (self->monsterinfo.active_move == &tank_move_punch_attack ||
		self->monsterinfo.active_move == &runnertank_move_jump_attack)
		return;

	// Check dodge cooldown using timestamp
	if (self->timestamp > level.time)
		return;

	// Don't dodge if projectile impact is too soon or too far away
	if (eta < FRAME_TIME_MS || eta > 3_sec)
		return;

	// Don't dodge if attacker is invalid
	if (!attacker)
		return;

	// Calculate dodge direction based on attacker position
	vec3_t dodge_dir;

	// Get our right vector for lateral dodge
	vec3_t right;
	AngleVectors(self->s.angles, nullptr, right, nullptr);

	// Decide dodge direction - prefer moving away from attacker
	vec3_t to_attacker = (attacker->s.origin - self->s.origin).normalized();
	float side_dot = to_attacker.dot(right);

	// Dodge perpendicular to attack direction, away from attacker
	if (side_dot > 0)
		dodge_dir = right * -1.0f; // Dodge left
	else
		dodge_dir = right; // Dodge right

	// Add some forward/backward component based on distance
	vec3_t forward;
	AngleVectors(self->s.angles, forward, nullptr, nullptr);
	float dist = (self->s.origin - attacker->s.origin).length();

	if (dist < 400.0f) {
		// Close range - dodge backward
		dodge_dir += forward * -0.3f;
	} else if (dist > 800.0f) {
		// Long range - dodge forward to close distance
		dodge_dir += forward * 0.2f;
	}

	dodge_dir = dodge_dir.normalized();

	// Calculate dodge speed based on urgency (eta)
	float base_dodge_speed = 300.0f;
	float eta_seconds = eta.seconds();
	float urgency_multiplier = std::clamp(2.0f - eta_seconds, 1.0f, 2.5f);
	float dodge_speed = base_dodge_speed * urgency_multiplier;

	// Apply dodge velocity
	vec3_t dodge_velocity = dodge_dir * dodge_speed;
	
	// Preserve some vertical momentum but replace horizontal
	dodge_velocity.z = self->velocity.z * 0.5f;
	self->velocity = dodge_velocity;

	// Set animation to running for dodge
	if (self->monsterinfo.active_move != &runnertank_move_run)
		M_SetAnimation(self, &runnertank_move_run);

	// Set cooldown using timestamp (like spider)
	self->timestamp = level.time + random_time(0.5_sec, 1.5_sec);

	// Also set pausetime for movement consistency
	self->monsterinfo.pausetime = level.time + random_time(0.3_sec, 0.7_sec);

	// Update lefty for consistency with sidestep
	self->monsterinfo.lefty = (side_dot > 0) ? 1 : 0;

	// Mark that we're dodging
	monster_done_dodge(self);
}

void SP_monster_runnertank(edict_t* self)
{
	const spawn_temp_t& st = ED_GetSpawnTemp();

	    if (self->monsterinfo.monster_type_id == MONSTER_TYPE_UNKNOWN) { // Check if it hasn't been set yet
        self->monsterinfo.monster_type_id = static_cast<uint8_t>(horde::MonsterTypeID::RUNNERTANK);
}	if (!M_AllowSpawn(self)) {
		G_FreeEdict(self);
		return;
	}

	self->s.modelindex = gi.modelindex("models/vault/monsters/tank/tris.md2");
	self->mins = { -28, -28, -14 };
	self->maxs = { 28, 28, 56 };
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	gi.modelindex("models/monsters/tank/gibs/barm.md2");
	gi.modelindex("models/monsters/tank/gibs/head.md2");
	gi.modelindex("models/monsters/tank/gibs/chest.md2");
	gi.modelindex("models/monsters/tank/gibs/foot.md2");
	gi.modelindex("models/monsters/tank/gibs/thigh.md2");

	sound_thud.assign("tank/tnkdeth2.wav");
	sound_idle.assign("tank/tnkidle1.wav");
	sound_die.assign("tank/death.wav");
	sound_step.assign("tank/step.wav");
	sound_windup.assign("tank/tnkatck4.wav");
	sound_strike.assign("tank/tnkatck5.wav");
	sound_sight.assign("tank/sight1.wav");

	gi.soundindex("tank/tnkatck1.wav");
	gi.soundindex("tank/tnkatk2a.wav");
	gi.soundindex("tank/tnkatk2b.wav");
	gi.soundindex("tank/tnkatk2c.wav");
	gi.soundindex("tank/tnkatk2d.wav");
	gi.soundindex("tank/tnkatk2e.wav");
	gi.soundindex("tank/tnkatck3.wav");

	// if (strcmp(self->classname, "monster_runnertank_commander") == 0)
	// {
	// 	self->health = (config ? config->health : 1000) * st.health_multiplier;
	// 	self->gib_health = -225;
	// 	self->count = 1;
	// 	sound_pain2.assign("tank/pain.wav");
	// }
	// else
	{
		int base_health = M_RUNNERTANK_INITIAL_HEALTH;
		if (g_horde && g_horde->integer && current_wave_level > 0) {
			self->health = ScaleMonsterHealth(base_health, current_wave_level, false);
		} else {
			self->health = base_health * st.health_multiplier;
		}
		self->gib_health = -200;
		sound_pain.assign("tank/tnkpain2.wav");
	}

	self->monsterinfo.scale = MODEL_SCALE;

	// [Paril-KEX] N64 runnertank commander is a chonky boy
	if (self->spawnflags.has(SPAWNFLAG_runnertank_COMMANDER_GUARDIAN))
	{
		if (!self->s.scale)
			self->s.scale = 1.5f;
		int base_health = M_RUNNERTANK_INITIAL_HEALTH;
		if (g_horde && g_horde->integer && current_wave_level > 0) {
			self->health = ScaleMonsterHealth(base_health, current_wave_level, false);
		} else {
			self->health = base_health * st.health_multiplier;
		}
	}

	// heat seekingness
	if (!self->accel)
		self->accel = 0.075f;

	self->mass = 500;

	self->pain = runnertank_pain;
	self->die = runnertank_die;
	self->monsterinfo.stand = runnertank_stand;
	self->monsterinfo.walk = runnertank_walk;
	self->monsterinfo.run = runnertank_run;
	self->monsterinfo.sidestep = runnertank_sidestep;
	self->monsterinfo.dodge = runnertank_dodge;
	self->monsterinfo.attack = runnertank_attack;
	self->monsterinfo.melee = runnertank_melee;
	self->monsterinfo.sight = runnertank_sight;
	self->monsterinfo.idle = runnertank_idle;
	self->monsterinfo.blocked = runnertank_blocked; // PGM
	self->monsterinfo.setskin = runnertank_setskin;
	self->monsterinfo.checkattack = runnertank_checkattack;
	self->yaw_speed = 20; // Better tracking but not too fast

	self->s.renderfx |= RF_CUSTOMSKIN;
	self->s.skinnum = gi.imageindex("models/vault/monsters/tank/skin.pcx");

	gi.linkentity(self);

	M_SetAnimation(self, &runnertank_move_stand);

	walkmonster_start(self);

	// PMM
	self->monsterinfo.aiflags |= AI_IGNORE_SHOTS;
	self->monsterinfo.blindfire = true;
	// pmm
// 	if (strcmp(self->classname, "monster_runnertank_commander") == 0)
// 		self->s.skinnum = 2;

	self->monsterinfo.can_jump = true;
	self->monsterinfo.drop_height = 256;
	// HORDE MOD: Increased jump height from 68 to 88 (30% increase) for better obstacle navigation
	self->monsterinfo.jump_height = 88;
	ApplyMonsterBonusFlags(self);
}

void Use_Boss3(edict_t* ent, edict_t* other, edict_t* activator);

THINK(Think_runnertankStand) (edict_t* ent) -> void
{
	if (ent->s.frame == FRAME_stand30)
		ent->s.frame = FRAME_stand01;
	else
		ent->s.frame++;
	ent->nextthink = level.time + 10_hz;
}

/*QUAKED monster_runnertank_stand (1 .5 0) (-32 -32 0) (32 32 90)

Just stands and cycles in one place until targeted, then teleports away.
N64 edition!
*/
void SP_monster_runnertank_stand(edict_t* self)
{
	if (!M_AllowSpawn(self)) {
		G_FreeEdict(self);
		return;
	}

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->model = "models/vault/monsters/tank/tris.md2";
	self->s.modelindex = gi.modelindex(self->model);
	self->s.frame = FRAME_stand01;
	self->s.skinnum = 2;

	gi.soundindex("misc/bigtele.wav");

	self->mins = { -32, -32, -16 };
	self->maxs = { 32, 32, 64 };

	if (!self->s.scale)
		self->s.scale = 1.5f;
	// Removed manual (mins/maxs)  scaling - monster_start() handles it automatically

	self->use = Use_Boss3;
	self->think = Think_runnertankStand;
	self->nextthink = level.time + 10_hz;
	gi.linkentity(self);
}