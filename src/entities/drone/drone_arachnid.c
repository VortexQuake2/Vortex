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
static int sound_melee;
static int sound_melee_hit;

#define ARACHNID_DEFAULT_SCALE		0.75f
#define ARACHNID_INVASION_SCALE		0.60f

static void arachnid_stand(edict_t *self);
static void arachnid_run(edict_t *self);

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

static void arachnid_charge_rail(edict_t *self)
{
	if (!G_ValidTarget(self, self->enemy, true, true))
		return;

	gi.sound(self, CHAN_WEAPON, sound_charge, 1, ATTN_NORM, 0);
	VectorCopy(self->enemy->s.origin, self->pos1);
	self->pos1[2] += self->enemy->viewheight;
}

static void arachnid_rail(edict_t *self)
{
	int damage;
	int flash_number;
	vec3_t start, forward, right, offset, dir;

	if (!G_ValidTarget(self, self->enemy, true, true))
		return;

	damage = M_RAILGUN_DMG_BASE + M_RAILGUN_DMG_ADDON * drone_damagelevel(self);
	if (M_RAILGUN_DMG_MAX && damage > M_RAILGUN_DMG_MAX)
		damage = M_RAILGUN_DMG_MAX;

	switch (self->s.frame)
	{
	case FRAME_rails7:
		flash_number = MZ2_ARACHNID_RAIL2;
		break;
	case FRAME_rails_up2:
	case FRAME_rails_up9:
		flash_number = MZ2_ARACHNID_RAIL_UP1;
		break;
	case FRAME_rails_up5:
	case FRAME_rails_up11:
		flash_number = MZ2_ARACHNID_RAIL_UP2;
		break;
	case FRAME_rails3:
	default:
		flash_number = MZ2_ARACHNID_RAIL1;
		break;
	}

	AngleVectors(self->s.angles, forward, right, NULL);
	VectorCopy(monster_flash_offset[flash_number], offset);
	if (self->s.scale)
		VectorScale(offset, self->s.scale, offset);
	G_ProjectSource(self->s.origin, offset, forward, right, start);
	VectorSubtract(self->pos1, start, dir);
	VectorNormalize(dir);

	monster_fire_railgun(self, start, dir, damage, 100, flash_number);
	M_DelayNextAttack(self, 0, true);
}

mframe_t arachnid_frames_attack1[] =
{
	ai_charge, 0, arachnid_charge_rail,
	ai_charge, 0, arachnid_rail,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, arachnid_charge_rail,
	ai_charge, 0, arachnid_rail,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, arachnid_charge_rail,
	ai_charge, 0, NULL
};
mmove_t arachnid_move_attack1 = { FRAME_rails2, FRAME_rails11, arachnid_frames_attack1, arachnid_run };

mframe_t arachnid_frames_attack_up1[] =
{
	ai_charge, 0, arachnid_charge_rail,
	ai_charge, 0, arachnid_rail,
	ai_charge, 0, NULL,
	ai_charge, 0, arachnid_charge_rail,
	ai_charge, 0, arachnid_rail,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, arachnid_charge_rail,
	ai_charge, 0, arachnid_rail,
	ai_charge, 0, arachnid_charge_rail,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL,
	ai_charge, 0, NULL
};
mmove_t arachnid_move_attack_up1 = { FRAME_rails_up1, FRAME_rails_up13, arachnid_frames_attack_up1, arachnid_run };

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

static void arachnid_attack(edict_t *self)
{
	if (!G_ValidTarget(self, self->enemy, true, true))
		return;

	if (self->monsterinfo.melee_finished < level.time && entdist(self, self->enemy) < MELEE_DISTANCE)
		self->monsterinfo.currentmove = &arachnid_move_melee;
	else if ((self->enemy->s.origin[2] - self->s.origin[2]) > 150)
		self->monsterinfo.currentmove = &arachnid_move_attack_up1;
	else
		self->monsterinfo.currentmove = &arachnid_move_attack1;

	M_DelayNextAttack(self, 0, true);
}

static void arachnid_melee(edict_t *self)
{
	if (!G_ValidTarget(self, self->enemy, true, true) || entdist(self, self->enemy) > 96)
		self->monsterinfo.currentmove = &arachnid_move_attack1;
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

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3.0;
	gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

	if (invasion->value == 2)
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

	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
		vrx_throw_drone_gibs(self, damage);
		M_Remove(self, false, false);
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

void init_drone_arachnid(edict_t *self)
{
	sound_step = gi.soundindex("insane/insane11.wav");
	sound_charge = gi.soundindex("gladiator/railgun.wav");
	sound_melee = gi.soundindex("gladiator/melee3.wav");
	sound_melee_hit = gi.soundindex("gladiator/melee2.wav");
	sound_pain = gi.soundindex("arachnid/pain.wav");
	sound_death = gi.soundindex("arachnid/death.wav");
	sound_sight = gi.soundindex("arachnid/sight.wav");

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
	self->mtype = M_ARACHNID;
	self->monsterinfo.control_cost = M_GLADIATOR_CONTROL_COST;
	self->monsterinfo.cost = M_DEFAULT_COST;
	self->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;
	self->monsterinfo.power_armor_power = M_ARACHNID_INITIAL_ARMOR + M_ARACHNID_ADDON_ARMOR * self->monsterinfo.level;
	self->monsterinfo.max_armor = self->monsterinfo.power_armor_power;
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
	self->monsterinfo.sight = arachnid_sight;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &arachnid_move_stand;
	self->monsterinfo.scale = MODEL_SCALE * self->s.scale;
	self->nextthink = level.time + FRAMETIME;
}
