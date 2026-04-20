/*
==============================================================================

STALKER

==============================================================================
*/

#include "g_local.h"
#include "../../quake2/monsterframes/m_rogue_stalker.h"

static int sound_pain;
static int sound_die;
static int sound_sight;
static int sound_punch_hit1;
static int sound_punch_hit2;
static int sound_idle;

void drone_ai_stand(edict_t *self, float dist);
void drone_ai_run(edict_t *self, float dist);
void drone_ai_walk(edict_t *self, float dist);

static void stalker_stand(edict_t *self);
static void stalker_run(edict_t *self);

static void stalker_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

static void stalker_idle(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_idle, 0.5, ATTN_IDLE, 0);
}

mframe_t stalker_frames_stand[] =
{
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, stalker_idle,
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
mmove_t stalker_move_stand = { FRAME_idle01, FRAME_idle21, stalker_frames_stand, NULL };

static void stalker_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &stalker_move_stand;
}

mframe_t stalker_frames_walk[] =
{
	drone_ai_walk, 10, NULL,
	drone_ai_walk, 10, NULL,
	drone_ai_walk, 10, NULL,
	drone_ai_walk, 10, NULL,
	drone_ai_walk, 10, NULL,
	drone_ai_walk, 10, NULL,
	drone_ai_walk, 10, NULL,
	drone_ai_walk, 10, NULL
};
mmove_t stalker_move_walk = { FRAME_walk01, FRAME_walk08, stalker_frames_walk, stalker_run };

static void stalker_walk(edict_t *self)
{
	self->monsterinfo.currentmove = &stalker_move_walk;
}

mframe_t stalker_frames_run[] =
{
	drone_ai_run, 24, NULL,
	drone_ai_run, 28, NULL,
	drone_ai_run, 32, NULL,
	drone_ai_run, 28, NULL
};
mmove_t stalker_move_run = { FRAME_run01, FRAME_run04, stalker_frames_run, NULL };

static void stalker_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &stalker_move_stand;
	else
		self->monsterinfo.currentmove = &stalker_move_run;
}

static void stalker_fire_ionripper(edict_t *self)
{
	int damage, speed;
	vec3_t forward, right, start, target, dir, offset;

	if (!G_EntExists(self->enemy))
		return;

	damage = IONRIPPER_INITIAL_DAMAGE + IONRIPPER_ADDON_DAMAGE * drone_damagelevel(self);
	speed = IONRIPPER_INITIAL_SPEED + IONRIPPER_ADDON_SPEED * drone_damagelevel(self);

	AngleVectors(self->s.angles, forward, right, NULL);
	VectorSet(offset, 16, 0, 6);
	G_ProjectSource(self->s.origin, offset, forward, right, start);

	VectorCopy(self->enemy->s.origin, target);
	target[2] += self->enemy->viewheight;
	VectorSubtract(target, start, dir);
	VectorNormalize(dir);

	fire_ionripper(self, start, dir, damage, speed, EF_IONRIPPER);

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(self - g_edicts);
	gi.WriteByte(MZ_IONRIPPER | MZ_SILENCED);
	gi.multicast(start, MULTICAST_PVS);
}

mframe_t stalker_frames_shoot[] =
{
	drone_ai_run, 10, NULL,
	drone_ai_run, 10, stalker_fire_ionripper,
	drone_ai_run, 12, stalker_fire_ionripper,
	drone_ai_run, 12, stalker_fire_ionripper
};
mmove_t stalker_move_shoot = { FRAME_run01, FRAME_run04, stalker_frames_shoot, stalker_run };

static void stalker_attack(edict_t *self)
{
	self->monsterinfo.currentmove = &stalker_move_shoot;
	M_DelayNextAttack(self, 0.4, true);
}

static void stalker_swing_attack(edict_t *self)
{
	int damage;

	damage = M_MELEE_DMG_BASE + M_MELEE_DMG_ADDON * drone_damagelevel(self);
	if (M_MELEE_DMG_MAX && damage > M_MELEE_DMG_MAX)
		damage = M_MELEE_DMG_MAX;

	if (M_MeleeAttack(self, self->enemy, 80, damage, 160))
		gi.sound(self, CHAN_WEAPON, (random() < 0.5) ? sound_punch_hit1 : sound_punch_hit2, 1, ATTN_NORM, 0);
}

mframe_t stalker_frames_swing_l[] =
{
	ai_charge, 0, NULL,
	ai_charge, 4, NULL,
	ai_charge, 5, stalker_swing_attack,
	ai_charge, 5, NULL,
	ai_charge, 5, stalker_swing_attack,
	ai_charge, 4, NULL,
	ai_charge, 2, NULL,
	ai_charge, 0, NULL
};
mmove_t stalker_move_swing_l = { FRAME_attack01, FRAME_attack08, stalker_frames_swing_l, stalker_run };

mframe_t stalker_frames_swing_r[] =
{
	ai_charge, 0, NULL,
	ai_charge, 6, stalker_swing_attack,
	ai_charge, 5, NULL,
	ai_charge, 5, stalker_swing_attack,
	ai_charge, 0, NULL
};
mmove_t stalker_move_swing_r = { FRAME_attack11, FRAME_attack15, stalker_frames_swing_r, stalker_run };

static void stalker_melee(edict_t *self)
{
	if (!G_ValidTarget(self, self->enemy, true, true) || entdist(self, self->enemy) > 96)
	{
		self->monsterinfo.melee_finished = level.time + 0.5;
		stalker_run(self);
		return;
	}

	if (random() < 0.5)
		self->monsterinfo.currentmove = &stalker_move_swing_l;
	else
		self->monsterinfo.currentmove = &stalker_move_swing_r;
}

mframe_t stalker_frames_pain[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t stalker_move_pain = { FRAME_pain01, FRAME_pain04, stalker_frames_pain, stalker_run };

static void stalker_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum |= 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3.0;
	gi.sound(self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);

	if (skill->value == 3)
		return;

	self->monsterinfo.currentmove = &stalker_move_pain;
}

static void stalker_dead(edict_t *self)
{
	VectorSet(self->mins, -28, -28, -18);
	VectorSet(self->maxs, 28, 28, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
	M_PrepBodyRemoval(self);
}

mframe_t stalker_frames_death[] =
{
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL,
	ai_move, 0, NULL
};
mmove_t stalker_move_death = { FRAME_death01, FRAME_death09, stalker_frames_death, stalker_dead };

static void stalker_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int n;

	M_Notify(self);

	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
		if (vrx_spawn_nonessential_ent(self->s.origin))
		{
			ThrowGib(self, "models/monsters/stalker/gibs/bodya.md2", damage, GIB_ORGANIC);
			ThrowGib(self, "models/monsters/stalker/gibs/bodyb.md2", damage, GIB_ORGANIC);
			for (n = 0; n < 2; n++)
				ThrowGib(self, "models/monsters/stalker/gibs/claw.md2", damage, GIB_ORGANIC);
			ThrowHead(self, "models/monsters/stalker/gibs/head.md2", damage, GIB_ORGANIC);
		}
		M_Remove(self, false, false);
		return;
	}

	if (self->deadflag == DEAD_DEAD)
		return;

	DroneList_Remove(self);
	gi.sound(self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.currentmove = &stalker_move_death;

	if (self->activator && !self->activator->client)
		self->activator->num_monsters_real--;
}

void init_drone_stalker(edict_t *self)
{
	sound_pain = gi.soundindex("stalker/pain.wav");
	sound_die = gi.soundindex("stalker/death.wav");
	sound_sight = gi.soundindex("stalker/sight.wav");
	sound_punch_hit1 = gi.soundindex("stalker/melee1.wav");
	sound_punch_hit2 = gi.soundindex("stalker/melee2.wav");
	sound_idle = gi.soundindex("stalker/idle.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/monsters/stalker/tris.md2");
	gi.modelindex("models/monsters/stalker/gibs/bodya.md2");
	gi.modelindex("models/monsters/stalker/gibs/bodyb.md2");
	gi.modelindex("models/monsters/stalker/gibs/claw.md2");
	gi.modelindex("models/monsters/stalker/gibs/foot.md2");
	gi.modelindex("models/monsters/stalker/gibs/head.md2");
	gi.modelindex("models/monsters/stalker/gibs/leg.md2");

	VectorSet(self->mins, -28, -28, -18);
	VectorSet(self->maxs, 28, 28, 18);

	self->health = M_PARASITE_INITIAL_HEALTH + M_PARASITE_ADDON_HEALTH * self->monsterinfo.level;
	self->max_health = self->health;
	self->gib_health = -125;
	self->mass = 250;
	self->mtype = M_STALKER;

	//self->monsterinfo.power_armor_type = POWER_ARMOR_SCREEN;
	//self->monsterinfo.power_armor_power = M_BERSERKER_INITIAL_ARMOR + M_BERSERKER_ADDON_ARMOR * self->monsterinfo.level;
	//self->monsterinfo.max_armor = self->monsterinfo.power_armor_power;
	self->monsterinfo.control_cost = M_BERSERKER_CONTROL_COST;
	self->monsterinfo.cost = M_DEFAULT_COST;

	self->item = FindItemByClassname("ammo_cells");

	self->pain = stalker_pain;
	self->die = stalker_die;
	self->monsterinfo.stand = stalker_stand;
	self->monsterinfo.walk = stalker_walk;
	self->monsterinfo.run = stalker_run;
	self->monsterinfo.attack = stalker_attack;
	self->monsterinfo.melee = stalker_melee;
	self->monsterinfo.sight = stalker_sight;
	self->monsterinfo.idle = stalker_idle;
	self->monsterinfo.pain_chance = 0.3f;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &stalker_move_stand;
	self->monsterinfo.scale = MODEL_SCALE;
	self->nextthink = level.time + FRAMETIME;
}
