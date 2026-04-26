/*
==============================================================================

DAEDALUS

==============================================================================
*/

#include "g_local.h"
#include "../../quake2/monsterframes/m_hover.h"

static int sound_pain1;
static int sound_pain2;
static int sound_death1;
static int sound_death2;
static int sound_sight;
static int sound_search1;
static int sound_search2;

extern mmove_t hover_move_stand;
extern mmove_t hover_move_walk;
extern mmove_t hover_move_run;
extern mmove_t hover_move_pain1;
extern mmove_t hover_move_pain2;
extern mmove_t hover_move_pain3;
extern mmove_t hover_move_death1;

static void daedalus_run(edict_t *self);
static void daedalus_reattack(edict_t *self);
static void daedalus_fire_grenade(edict_t *self);

static void daedalus_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

static void daedalus_search(edict_t *self)
{
	if (!self->goalentity)
		self->goalentity = world;

	if (random() < 0.5)
		gi.sound(self, CHAN_VOICE, sound_search1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_search2, 1, ATTN_NORM, 0);
}

static void daedalus_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &hover_move_stand;
}

static void daedalus_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &hover_move_walk;
}

static void daedalus_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &hover_move_stand;
	else
		self->monsterinfo.currentmove = &hover_move_run;
}

mframe_t daedalus_frames_attack[] =
{
	ai_charge, -10, daedalus_fire_grenade,
	ai_charge, -10, daedalus_fire_grenade,
	ai_charge, 0, daedalus_reattack
};
mmove_t daedalus_move_attack = { FRAME_attak104, FRAME_attak106, daedalus_frames_attack, NULL };

mframe_t daedalus_frames_end_attack[] =
{
	drone_ai_run, 15, NULL,
	drone_ai_run, 15, NULL
};
mmove_t daedalus_move_end_attack = { FRAME_attak107, FRAME_attak108, daedalus_frames_end_attack, daedalus_run };

static void daedalus_fire_grenade(edict_t *self)
{
	int damage, speed, flash_number;
	vec3_t forward, right, start, offset;

	if (!G_EntExists(self->enemy))
		return;

	damage = M_GRENADELAUNCHER_DMG_BASE + M_GRENADELAUNCHER_DMG_ADDON * drone_damagelevel(self);
	if (M_GRENADELAUNCHER_DMG_MAX && damage > M_GRENADELAUNCHER_DMG_MAX)
		damage = M_GRENADELAUNCHER_DMG_MAX;

	speed = M_GRENADELAUNCHER_SPEED_BASE + M_GRENADELAUNCHER_SPEED_ADDON * drone_damagelevel(self);
	if (M_GRENADELAUNCHER_SPEED_MAX && speed > M_GRENADELAUNCHER_SPEED_MAX)
		speed = M_GRENADELAUNCHER_SPEED_MAX;

	AngleVectors(self->s.angles, forward, right, NULL);
	if (self->s.frame == FRAME_attak104)
		VectorSet(offset, 1.7, 7.0, 11.3);
	else
		VectorSet(offset, 1.7, -7.0, 11.3);
	G_ProjectSource(self->s.origin, offset, forward, right, start);

	flash_number = MZ2_GUNCMDR_GRENADE_MORTAR_1;
	MonsterAim(self, M_PROJECTILE_ACC, speed, false, -1, forward, start);
	monster_fire_grenade(self, start, forward, damage, speed, flash_number);
}

static void daedalus_reattack(edict_t *self)
{
	if (G_ValidTarget(self, self->enemy, true, true) && random() <= 0.65)
	{
		self->s.frame = FRAME_attak104;
		return;
	}

	self->monsterinfo.attack_finished = level.time + 1.0;
	if (!(self->monsterinfo.aiflags & AI_STAND_GROUND))
		self->monsterinfo.currentmove = &daedalus_move_end_attack;
}

static void daedalus_attack(edict_t *self)
{
	self->monsterinfo.currentmove = &daedalus_move_attack;
}

static void daedalus_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum |= 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3.0;

	if (random() < 0.5)
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);

	if (skill->value == 3)
		return;

	if (damage <= 25)
	{
		if (random() < 0.5)
			self->monsterinfo.currentmove = &hover_move_pain3;
		else
			self->monsterinfo.currentmove = &hover_move_pain2;
	}
	else
		self->monsterinfo.currentmove = &hover_move_pain1;
}

static void daedalus_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int n;

	M_Notify(self);

	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
		if (vrx_spawn_nonessential_ent(self->s.origin))
		{
			for (n = 0; n < 2; n++)
				ThrowGib(self, "models/objects/gibs/bone/tris.md2", damage, GIB_ORGANIC);
			for (n = 0; n < 2; n++)
				ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
			ThrowHead(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		}
		M_Remove(self, false, false);
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

	DroneList_Remove(self);

	if (random() < 0.5)
		gi.sound(self, CHAN_VOICE, sound_death1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_death2, 1, ATTN_NORM, 0);

	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	vrx_update_drone_death_skin(self);
	self->flags &= ~FL_FLY;
	self->movetype = MOVETYPE_TOSS;
	self->gravity = 1.0;
	if (self->velocity[2] > -120)
		self->velocity[2] = -120;
	self->monsterinfo.currentmove = &hover_move_death1;

	if (self->activator && !self->activator->client)
		self->activator->num_monsters_real--;
}

void init_drone_daedalus(edict_t *self)
{
	sound_pain1 = gi.soundindex("daedalus/daedpain1.wav");
	sound_pain2 = gi.soundindex("daedalus/daedpain2.wav");
	sound_death1 = gi.soundindex("daedalus/daeddeth1.wav");
	sound_death2 = gi.soundindex("daedalus/daeddeth2.wav");
	sound_sight = gi.soundindex("daedalus/daedsght1.wav");
	sound_search1 = gi.soundindex("daedalus/daedsrch1.wav");
	sound_search2 = gi.soundindex("daedalus/daedsrch2.wav");

	gi.soundindex("hover/hovatck1.wav");
	self->s.sound = gi.soundindex("hover/hovidle1.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/hover/tris.md2");
	VectorSet(self->mins, -24, -24, -24);
	VectorSet(self->maxs, 24, 24, 32);
	self->s.skinnum = 2;

	self->health = M_DAEDALUS_INITIAL_HEALTH + M_DAEDALUS_ADDON_HEALTH * self->monsterinfo.level;
	self->max_health = self->health;
	self->gib_health = -100;
	self->mass = 225;

	self->mtype = M_DAEDALUS;
	self->flags |= FL_FLY;
	self->monsterinfo.power_armor_power = M_DAEDALUS_INITIAL_ARMOR + M_DAEDALUS_ADDON_ARMOR * self->monsterinfo.level;
	self->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;
	self->monsterinfo.max_armor = self->monsterinfo.power_armor_power;
	self->monsterinfo.control_cost = M_HOVER_CONTROL_COST;
	self->monsterinfo.cost = M_HOVER_COST;
	self->item = FindItemByClassname("ammo_grenades");

	self->pain = daedalus_pain;
	self->die = daedalus_die;
	self->monsterinfo.stand = daedalus_stand;
	self->monsterinfo.walk = daedalus_walk;
	self->monsterinfo.run = daedalus_run;
	self->monsterinfo.attack = daedalus_attack;
	self->monsterinfo.sight = daedalus_sight;
	self->monsterinfo.idle = daedalus_search;
	self->monsterinfo.pain_chance = 0.2f;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &hover_move_stand;
	self->monsterinfo.scale = MODEL_SCALE;
}
