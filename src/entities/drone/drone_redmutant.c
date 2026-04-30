/*
==============================================================================

RED MUTANT

==============================================================================
*/

#include "g_local.h"
#include "../../quake2/monsterframes/m_redmutant.h"

static int sound_swing;
static int sound_hit;
static int sound_hit2;
static int sound_death;
static int sound_idle;
static int sound_pain1;
static int sound_pain2;
static int sound_sight;
static int sound_search;
static int sound_step1;
static int sound_step2;
static int sound_step3;
static int sound_thud;

#define REDMUTANT_STAND_MAX_Z 36
#define REDMUTANT_IDLE_MAX_Z 56

static void redmutant_set_bbox_height(edict_t *self, float max_z)
{
	if (self->maxs[2] == max_z)
		return;

	self->maxs[2] = max_z;
	gi.linkentity(self);
}

static void redmutant_restore_bbox(edict_t *self)
{
	redmutant_set_bbox_height(self, REDMUTANT_STAND_MAX_Z);
}

static void redmutant_stand(edict_t *self);
static void redmutant_walk(edict_t *self);
static void redmutant_run(edict_t *self);
static void redmutant_attack(edict_t *self);
static void redmutant_melee_unused(edict_t *self);
static void redmutant_post_jump(edict_t *self);
extern mmove_t redmutant_move_jump_finish;

static void redmutant_step(edict_t *self)
{
	switch (GetRandom(0, 2))
	{
	case 0: gi.sound(self, CHAN_BODY, sound_step1, 1, ATTN_NORM, 0); break;
	case 1: gi.sound(self, CHAN_BODY, sound_step2, 1, ATTN_NORM, 0); break;
	default: gi.sound(self, CHAN_BODY, sound_step3, 1, ATTN_NORM, 0); break;
	}
}

static void redmutant_sight(edict_t *self, edict_t *other)
{
	gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
}

static void redmutant_search(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

static void redmutant_swing(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_swing, 1, ATTN_NORM, 0);
}

mframe_t redmutant_frames_stand[] =
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
	drone_ai_stand, 0, NULL
};
mmove_t redmutant_move_stand = { FRAME_stand101, FRAME_stand112, redmutant_frames_stand, NULL };

static void redmutant_stand(edict_t *self)
{
	self->monsterinfo.currentmove = &redmutant_move_stand;
}

static void redmutant_idle_loop(edict_t *self)
{
	if (random() < 0.75)
		self->monsterinfo.nextframe = FRAME_stand202;
}

mframe_t redmutant_frames_idle[] =
{
	drone_ai_stand, 0, redmutant_idle_loop,
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
mmove_t redmutant_move_idle = { FRAME_stand202, FRAME_stand228, redmutant_frames_idle, redmutant_stand };

static void redmutant_idle(edict_t *self)
{
	self->monsterinfo.currentmove = &redmutant_move_idle;
	gi.sound(self, CHAN_VOICE, sound_idle, 1, ATTN_IDLE, 0);
}

mframe_t redmutant_frames_walk[] =
{
	drone_ai_walk, 3, NULL,
	drone_ai_walk, 1, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 10, NULL,
	drone_ai_walk, 13, NULL,
	drone_ai_walk, 10, NULL,
	drone_ai_walk, 0, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 6, NULL,
	drone_ai_walk, 16, NULL,
	drone_ai_walk, 15, NULL,
	drone_ai_walk, 6, NULL
};
mmove_t redmutant_move_walk = { FRAME_walk05, FRAME_walk16, redmutant_frames_walk, NULL };

static void redmutant_walk_loop(edict_t *self)
{
	self->monsterinfo.currentmove = &redmutant_move_walk;
}

mframe_t redmutant_frames_start_walk[] =
{
	drone_ai_walk, 5, NULL,
	drone_ai_walk, 5, NULL,
	drone_ai_walk, -2, NULL,
	drone_ai_walk, 1, NULL
};
mmove_t redmutant_move_start_walk = { FRAME_walk01, FRAME_walk04, redmutant_frames_start_walk, redmutant_walk_loop };

static void redmutant_walk(edict_t *self)
{
	if (!self->goalentity)
		self->goalentity = world;
	self->monsterinfo.currentmove = &redmutant_move_start_walk;
}

mframe_t redmutant_frames_run[] =
{
	drone_ai_run, 44, NULL,
	drone_ai_run, 44, redmutant_step,
	drone_ai_run, 28, NULL,
	drone_ai_run, 8, redmutant_step,
	drone_ai_run, 22, NULL,
	drone_ai_run, 15, NULL
};
mmove_t redmutant_move_run = { FRAME_run03, FRAME_run08, redmutant_frames_run, NULL };

static void redmutant_run(edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &redmutant_move_stand;
	else
		self->monsterinfo.currentmove = &redmutant_move_run;
}

static void redmutant_hit_left(edict_t *self)
{
	int damage;
	vec3_t aim;

	damage = M_MELEE_DMG_BASE + M_MELEE_DMG_ADDON * drone_damagelevel(self);
	if (M_MELEE_DMG_MAX && damage > M_MELEE_DMG_MAX)
		damage = M_MELEE_DMG_MAX;

	VectorSet(aim, 100, self->mins[0], 8);
	if (fire_hit(self, aim, damage, 100))
		gi.sound(self, CHAN_WEAPON, sound_hit, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_WEAPON, sound_swing, 1, ATTN_NORM, 0);
}

static void redmutant_hit_right(edict_t *self)
{
	int damage;
	vec3_t aim;

	damage = M_MELEE_DMG_BASE + M_MELEE_DMG_ADDON * drone_damagelevel(self);
	if (M_MELEE_DMG_MAX && damage > M_MELEE_DMG_MAX)
		damage = M_MELEE_DMG_MAX;

	VectorSet(aim, 100, self->maxs[0], 8);
	if (fire_hit(self, aim, damage, 100))
		gi.sound(self, CHAN_WEAPON, sound_hit2, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_WEAPON, sound_swing, 1, ATTN_NORM, 0);
}

static void redmutant_check_refire(edict_t *self)
{
	if (G_ValidTarget(self, self->enemy, true, true)
		&& ((random() < 0.5) || (entdist(self, self->enemy) <= 96)))
		self->monsterinfo.nextframe = FRAME_attack109;
}

mframe_t redmutant_frames_attack[] =
{
	ai_charge, 0, NULL,
	ai_charge, 0, redmutant_hit_left,
	ai_charge, 0, redmutant_hit_right,
	ai_charge, 0, NULL,
	ai_charge, 0, redmutant_hit_right,
	ai_charge, 0, NULL,
	ai_charge, 0, redmutant_check_refire
};
mmove_t redmutant_move_attack = { FRAME_attack109, FRAME_attack115, redmutant_frames_attack, redmutant_run };

static void redmutant_melee_unused(edict_t *self)
{
	(void)self;
}

static void redmutant_jump_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	int damage, knockback;
	vec3_t point, normal;

	if (self->health <= 0)
	{
		self->touch = NULL;
		return;
	}

	if (G_EntExists(other))
	{
		VectorCopy(self->velocity, normal);
		VectorNormalize(normal);
		VectorMA(self->s.origin, self->maxs[0], normal, point);

		damage = 100 + 20 * drone_damagelevel(self);
		damage = vrx_increase_monster_damage_by_talent(self->activator, damage);
		knockback = min(damage, 500);

		T_Damage(other, self, self, self->velocity, point, normal, damage, knockback, 0, MOD_UNKNOWN);
		self->style = 1;
	}

	if (!M_CheckBottom(self))
	{
		if (self->groundentity)
		{
			self->monsterinfo.nextframe = FRAME_attack102;
			self->touch = NULL;
		}
		return;
	}

	self->touch = NULL;
}

extern mmove_t redmutant_move_jump_air;

static void redmutant_jump_takeoff(edict_t *self)
{
    vec3_t forward;
    qboolean high_jump = false;

    gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
    self->lastsound = level.framenum;
    AngleVectors(self->s.angles, forward, NULL, NULL);
    self->s.origin[2] += 1;
    
    if (random() < 0.28)
        high_jump = true;
        
    VectorScale(forward, 1125, self->velocity);
    self->velocity[2] = high_jump ? 240 : 160;
    self->groundentity = NULL;
    self->monsterinfo.aiflags |= AI_DUCKED;
    self->monsterinfo.attack_finished = level.time + (high_jump ? 1.55 : 1.3);
    self->style = 0;
    self->touch = redmutant_jump_touch;

    self->monsterinfo.currentmove = &redmutant_move_jump_air;
}

static void redmutant_check_landing(edict_t *self)
{
    if (self->groundentity)
    {
        gi.sound(self, CHAN_WEAPON, sound_thud, 1, ATTN_NORM, 0);
        self->monsterinfo.attack_finished = level.time + GetRandom(5, 15) * FRAMETIME;
        self->monsterinfo.aiflags &= ~(AI_HOLD_FRAME | AI_DUCKED);

        self->monsterinfo.currentmove = &redmutant_move_jump_finish;
        self->style = 0;
        self->touch = NULL;
        return;
    }

    self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

static void redmutant_check_landing_ai(edict_t *self, float dist)
{
    ai_charge(self, dist);
    redmutant_check_landing(self);
}

mframe_t redmutant_frames_jump_air[] = {
    redmutant_check_landing_ai, 0, NULL
};
mmove_t redmutant_move_jump_air = { FRAME_attack103, FRAME_attack103, redmutant_frames_jump_air, NULL };

mframe_t redmutant_frames_jump_start[] = {
    ai_charge, 0,  NULL,
    ai_charge, 17, NULL,
    ai_charge, 15, redmutant_jump_takeoff
};
mmove_t redmutant_move_jump_start = { FRAME_attack101, FRAME_attack103, redmutant_frames_jump_start, NULL };

mframe_t redmutant_frames_jump_finish[] = {
    ai_charge, 15, NULL,
    ai_charge, 0,  NULL,
    ai_charge, 3,  NULL,
    ai_charge, 0,  NULL,
    ai_charge, 0,  NULL
};
mmove_t redmutant_move_jump_finish = { FRAME_attack104, FRAME_attack108, redmutant_frames_jump_finish, redmutant_post_jump };

static void redmutant_jump(edict_t *self)
{
    self->monsterinfo.currentmove = &redmutant_move_jump_start;
}

static void redmutant_post_jump(edict_t *self)
{
    if (G_ValidTarget(self, self->enemy, true, true) && (entdist(self, self->enemy) <= MELEE_DISTANCE * 2))
        self->monsterinfo.currentmove = &redmutant_move_attack;
    else
        redmutant_run(self);
}

static void redmutant_flip_takeoff(edict_t *self)
{
    vec3_t forward;

    gi.sound(self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
    self->lastsound = level.framenum;
    AngleVectors(self->s.angles, forward, NULL, NULL);
    self->s.origin[2] += 1;
    
    VectorScale(forward, 800, self->velocity);
    self->velocity[2] = 200;
    self->groundentity = NULL;
    self->monsterinfo.aiflags |= AI_DUCKED;
}

static void redmutant_flip_check_landing(edict_t *self)
{
    if (self->groundentity)
    {
        gi.sound(self, CHAN_WEAPON, sound_thud, 1, ATTN_NORM, 0);
        self->monsterinfo.aiflags &= ~(AI_HOLD_FRAME | AI_DUCKED);
        return;
    }
    
    self->monsterinfo.nextframe = FRAME_attack108;
    self->monsterinfo.aiflags |= AI_HOLD_FRAME;
}

static void redmutant_flip_check_landing_ai(edict_t *self, float dist)
{
    ai_charge(self, dist);
    redmutant_flip_check_landing(self);
}

mframe_t redmutant_frames_flip[] = {
    ai_charge, 0,  NULL,
    ai_charge, 17, NULL,
    ai_charge, 15, redmutant_flip_takeoff,
    ai_charge, 15, NULL,
    ai_charge, 15, NULL,
    ai_charge, 0,  NULL,
    ai_charge, 3,  NULL,
    redmutant_flip_check_landing_ai, 0, NULL
};
mmove_t redmutant_move_flip = { FRAME_attack101, FRAME_attack108, redmutant_frames_flip, redmutant_post_jump };

static void redmutant_flip(edict_t *self)
{
    self->monsterinfo.currentmove = &redmutant_move_flip;
}

static void redmutant_attack(edict_t *self)
{
    float dist;
    float height_diff;

    if (!self->groundentity && !self->waterlevel)
        return;
    if (!G_ValidTarget(self, self->enemy, true, true))
        return;

    dist = entdist(self, self->enemy);
    height_diff = self->enemy->absmin[2] - self->absmin[2];

    if (dist <= MELEE_DISTANCE)
    {
        redmutant_restore_bbox(self);
        self->monsterinfo.currentmove = &redmutant_move_attack;
    }
    else if ((dist <= 384) && (height_diff > -64) && (height_diff < 96) && (random() < 0.8))
    {
        if (random() < 0.5)
            redmutant_jump(self);
        else
            redmutant_flip(self);
    }
}

static void redmutant_dead(edict_t *self)
{
	VectorSet(self->mins, -16, -16, -24);
	VectorSet(self->maxs, 16, 16, 0);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
	M_PrepBodyRemoval(self);
}

static void redmutant_shrink(edict_t *self)
{
	self->maxs[2] = 0;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}

static void ai_move_slide_right(edict_t *self, float dist)
{
	M_walkmove(self, self->s.angles[YAW] + 90, dist);
}

static void ai_move_slide_left(edict_t *self, float dist)
{
	M_walkmove(self, self->s.angles[YAW] - 90, dist);
}

mframe_t redmutant_frames_death1[] =
{
	ai_move_slide_right, 0, NULL,
	ai_move_slide_right, 0, NULL,
	ai_move_slide_right, 0, NULL,
	ai_move_slide_right, 2, NULL,
	ai_move_slide_right, 5, NULL,
	ai_move_slide_right, 7, redmutant_shrink,
	ai_move_slide_right, 6, NULL,
	ai_move_slide_right, 2, NULL,
	ai_move_slide_right, 0, NULL,
	ai_move_slide_right, 0, NULL,
	ai_move_slide_right, 0, NULL,
	ai_move_slide_right, 0, NULL,
	ai_move_slide_right, 0, NULL,
	ai_move_slide_right, 0, NULL,
	ai_move_slide_right, 0, NULL,
	ai_move_slide_right, 0, NULL,
	ai_move_slide_right, 0, NULL,
	ai_move_slide_right, 0, NULL,
	ai_move_slide_right, 0, NULL,
	ai_move_slide_right, 0, NULL
};
mmove_t redmutant_move_death1 = { FRAME_death101, FRAME_death120, redmutant_frames_death1, redmutant_dead };

mframe_t redmutant_frames_death2[] =
{
	ai_move_slide_left, 0, NULL,
	ai_move_slide_left, 1, NULL,
	ai_move_slide_left, 6, NULL,
	ai_move_slide_left, 8, NULL,
	ai_move_slide_left, 3, redmutant_shrink,
	ai_move_slide_left, 2, NULL,
	ai_move_slide_left, 0, NULL
};
mmove_t redmutant_move_death2 = { FRAME_death201, FRAME_death207, redmutant_frames_death2, redmutant_dead };

static void redmutant_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	int n;

	M_Notify(self);

#ifdef OLD_NOLAG_STYLE
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
			ThrowGib(self, "models/monsters/mutant/gibs/chest.md2", damage, GIB_ORGANIC);
			ThrowHead(self, "models/monsters/mutant/gibs/head.md2", damage, GIB_ORGANIC);
		}
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

	gi.sound(self, CHAN_VOICE, sound_death, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	self->s.skinnum = 1;
	self->monsterinfo.currentmove = (random() < 0.5) ? &redmutant_move_death1 : &redmutant_move_death2;

	DroneList_Remove(self);

	if (self->activator && !self->activator->client)
		self->activator->num_monsters_real--;
}

mframe_t redmutant_frames_pain1[] =
{
	ai_move, 4, NULL,
	ai_move, -3, NULL,
	ai_move, -8, NULL,
	ai_move, 2, NULL,
	ai_move, 5, NULL
};
mmove_t redmutant_move_pain1 = { FRAME_pain101, FRAME_pain105, redmutant_frames_pain1, redmutant_run };

mframe_t redmutant_frames_pain2[] =
{
	ai_move, -24, NULL,
	ai_move, 11, NULL,
	ai_move, 5, NULL,
	ai_move, -2, NULL,
	ai_move, 6, NULL,
	ai_move, 4, NULL
};
mmove_t redmutant_move_pain2 = { FRAME_pain201, FRAME_pain206, redmutant_frames_pain2, redmutant_run };

mframe_t redmutant_frames_pain3[] =
{
	ai_move, -22, NULL,
	ai_move, 3, NULL,
	ai_move, 3, NULL,
	ai_move, 2, NULL,
	ai_move, 1, NULL,
	ai_move, 1, NULL,
	ai_move, 6, NULL,
	ai_move, 3, NULL,
	ai_move, 2, NULL,
	ai_move, 0, NULL,
	ai_move, 1, NULL
};
mmove_t redmutant_move_pain3 = { FRAME_pain301, FRAME_pain311, redmutant_frames_pain3, redmutant_run };

static void redmutant_pain(edict_t *self, edict_t *other, float kick, int damage)
{
	float r;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3.0;
	r = random();

	if (r < 0.33)
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else if (r < 0.66)
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);

	if (skill->value == 3)
		return;

	redmutant_restore_bbox(self);

	if (r < 0.33)
		self->monsterinfo.currentmove = &redmutant_move_pain1;
	else if (r < 0.66)
		self->monsterinfo.currentmove = &redmutant_move_pain2;
	else
		self->monsterinfo.currentmove = &redmutant_move_pain3;
}

void init_drone_redmutant(edict_t *self)
{
	sound_swing = gi.soundindex("mutant/mutatck1.wav");
	sound_hit = gi.soundindex("mutant/mutatck2.wav");
	sound_hit2 = gi.soundindex("mutant/mutatck3.wav");
	sound_death = gi.soundindex("mutant/mutdeth1.wav");
	sound_idle = gi.soundindex("mutant/mutidle1.wav");
	sound_pain1 = gi.soundindex("mutant/mutpain1.wav");
	sound_pain2 = gi.soundindex("mutant/mutpain2.wav");
	sound_sight = gi.soundindex("mutant/mutsght1.wav");
	sound_search = gi.soundindex("mutant/mutsrch1.wav");
	sound_step1 = gi.soundindex("mutant/step1.wav");
	sound_step2 = gi.soundindex("mutant/step2.wav");
	sound_step3 = gi.soundindex("mutant/step3.wav");
	sound_thud = gi.soundindex("mutant/thud1.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex("models/vault/monsters/mutant/tris.md2");
	gi.modelindex("models/monsters/mutant/gibs/head.md2");
	gi.modelindex("models/monsters/mutant/gibs/chest.md2");
	gi.modelindex("models/monsters/mutant/gibs/hand.md2");
	gi.modelindex("models/monsters/mutant/gibs/foot.md2");

	VectorSet(self->mins, -18, -18, -24);
	VectorSet(self->maxs, 18, 18, REDMUTANT_STAND_MAX_Z);

	self->health = M_REDMUTANT_INITIAL_HEALTH + M_REDMUTANT_ADDON_HEALTH * self->monsterinfo.level;
	self->max_health = self->health;
	self->gib_health = -BASE_GIB_HEALTH;
	self->mass = 350;

	self->monsterinfo.pain_chance = 0.15f;
	self->pain = redmutant_pain;
	self->die = redmutant_die;

	self->monsterinfo.stand = redmutant_stand;
	self->monsterinfo.walk = redmutant_walk;
	self->monsterinfo.run = redmutant_run;
	self->monsterinfo.attack = redmutant_attack;
	self->monsterinfo.melee = redmutant_melee_unused;
	self->monsterinfo.sight = redmutant_sight;
	self->monsterinfo.idle = redmutant_idle;
	self->monsterinfo.jumpup = 64;
	self->monsterinfo.jumpdn = 512;
	self->monsterinfo.aiflags |= AI_NO_CIRCLE_STRAFE;

	self->monsterinfo.power_armor_type = POWER_ARMOR_SHIELD;
	self->monsterinfo.power_armor_power = M_REDMUTANT_INITIAL_ARMOR + M_REDMUTANT_ADDON_ARMOR * self->monsterinfo.level;
	self->monsterinfo.max_armor = self->monsterinfo.power_armor_power;
	self->monsterinfo.control_cost = M_MUTANT_CONTROL_COST;
	self->monsterinfo.cost = M_MUTANT_COST;
	self->mtype = M_REDMUTANT;

	gi.linkentity(self);

	self->monsterinfo.currentmove = &redmutant_move_stand;
	self->monsterinfo.scale = MODEL_SCALE;
	self->nextthink = level.time + FRAMETIME;
}
