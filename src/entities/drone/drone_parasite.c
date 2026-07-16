/*
==============================================================================

parasite

==============================================================================
*/

#include "g_local.h"
#include "../../quake2/monsterframes/m_parasite.h"

static int	sound_pain1;
static int	sound_pain2;
static int	sound_die;
static int	sound_launch;
static int	sound_impact;
static int	sound_suck;
static int	sound_reelin;
static int	sound_sight;
static int	sound_tap;
static int	sound_scratch;
static int	sound_search;

void myparasite_stand (edict_t *self);
void myparasite_start_run (edict_t *self);
void myparasite_run (edict_t *self);
void parasite_walk (edict_t *self);
void myparasite_start_walk (edict_t *self);
void myparasite_end_fidget (edict_t *self);
void myparasite_do_fidget (edict_t *self);
void myparasite_refidget (edict_t *self);
void myparasite_checkattack (edict_t *self);
void myparasite_attack1 (edict_t *self);
void myparasite_continue (edict_t *self);

extern mmove_t myparasite_move_break;
extern mmove_t myparasite_move_fire_proboscis;

static constexpr int PARASITE_PROBOSCIS_SPEED = 1250;
static constexpr float PARASITE_PROBOSCIS_RETRACT_MODIFIER = 2.0f;
static constexpr float PARASITE_PROBOSCIS_DRAIN_INTERVAL = 0.1f;
// Attack-initiation reach. Keep under the proboscis flight cap of (PARASITE_PROBOSCIS_SPEED*2)/15
// (~166 at speed 1250, see myparasite_proboscis_think) so the tip doesn't retract before arrival.
static constexpr float PARASITE_PROBOSCIS_RANGE = 160.0f;
static constexpr int PARASITE_PROBOSCIS_IMPACT_DAMAGE = 5;

static constexpr int PROBOSCIS_FLYING = 0;
static constexpr int PROBOSCIS_LATCHED = 1;
static constexpr int PROBOSCIS_RETRACTING = 2;
static constexpr int PROBOSCIS_DONE = 3;

static const vec3_t parasite_break_offsets[] = {
	{ 7.0f, 0, 7.0f },
	{ 6.3f, 14.5f, 4.0f },
	{ 8.5f, 0, 5.6f },
	{ 5.0f, -15.25f, 4.0f },
	{ 9.5f, -1.8f, 5.9f },
	{ 6.2f, 14.0f, 4.0f },
	{ 12.25f, 7.5f, 1.4f },
	{ 13.8f, 0, -2.4f },
	{ 13.8f, 0, -4.0f },
	{ 0.1f, 0, -0.7f },
	{ 5.0f, 0, 3.7f },
	{ 11.0f, 0, 4.0f },
	{ 13.5f, 0, -4.0f },
	{ 13.5f, 0, -4.0f },
	{ 0.2f, 0, -0.7f },
	{ 3.9f, 0, 3.6f },
	{ 8.5f, 0, 5.0f },
	{ 14.0f, 0, -4.0f },
	{ 14.0f, 0, -4.0f },
	{ 0.1f, 0, -0.5f }
};

static const vec3_t parasite_drain_offsets[] = {
	{ -1.7f, 0, 1.2f },
	{ -2.2f, 0, -0.6f },
	{ 7.7f, 0, 7.2f },
	{ 7.2f, 0, 5.7f },
	{ 6.2f, 0, 7.8f },
	{ 4.7f, 0, 6.7f },
	{ 5.0f, 0, 9.0f },
	{ 5.0f, 0, 7.0f },
	{ 5.0f, 0, 10.5f },
	{ 4.5f, 0, 9.7f },
	{ 1.5f, 0, 12.0f },
	{ 2.9f, 0, 11.0f },
	{ 2.1f, 0, 7.6f }
};

static void myparasite_proboscis_reset(edict_t *self);
static void myparasite_proboscis_retract(edict_t *self);
static void myparasite_proboscis_segment_draw(edict_t *self);

static qboolean myparasite_proboscis_inuse(edict_t *self)
{
	return self && self->inuse;
}

static void ParasiteProjectProboscisSource(edict_t *self, const vec3_t offset, vec3_t start)
{
	vec3_t forward, right, scaled_offset;

	AngleVectors(self->s.angles, forward, right, NULL);
	VectorCopy(offset, scaled_offset);
	if (self->s.scale && self->s.scale != 1.0f)
		VectorScale(scaled_offset, self->s.scale, scaled_offset);
	G_ProjectSource(self->s.origin, scaled_offset, forward, right, start);
}

static void myparasite_get_proboscis_start(edict_t *self, vec3_t start)
{
	vec3_t offset;

	if (self->s.frame >= FRAME_break01 &&
		self->s.frame < FRAME_break01 + (int)q_countof(parasite_break_offsets))
	{
		VectorCopy(parasite_break_offsets[self->s.frame - FRAME_break01], offset);
	}
	else if (self->s.frame >= FRAME_drain01 &&
		self->s.frame < FRAME_drain01 + (int)q_countof(parasite_drain_offsets))
	{
		VectorCopy(parasite_drain_offsets[self->s.frame - FRAME_drain01], offset);
	}
	else
	{
		VectorSet(offset, 8, 0, 6);
	}

	ParasiteProjectProboscisSource(self, offset, start);
}

static void myparasite_retract_active_proboscis(edict_t *self)
{
	if (myparasite_proboscis_inuse(self->proboscis) &&
		self->proboscis->style != PROBOSCIS_RETRACTING)
	{
		myparasite_proboscis_retract(self->proboscis);
	}
}

void myparasite_launch (edict_t *self)
{
	gi.sound (self, CHAN_WEAPON, sound_launch, 1, ATTN_NORM, 0);
}

void myparasite_reel_in (edict_t *self)
{
	gi.sound (self, CHAN_WEAPON, sound_reelin, 1, ATTN_NORM, 0);
}

void myparasite_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_WEAPON, sound_sight, 1, ATTN_NORM, 0);
	myparasite_attack1(self);
}

void myparasite_tap (edict_t *self)
{
	gi.sound (self, CHAN_WEAPON, sound_tap, 1, ATTN_IDLE, 0);
}

void myparasite_scratch (edict_t *self)
{
	gi.sound (self, CHAN_WEAPON, sound_scratch, 1, ATTN_IDLE, 0);
}

void myparasite_search (edict_t *self)
{
	gi.sound (self, CHAN_WEAPON, sound_search, 1, ATTN_IDLE, 0);
}


mframe_t myparasite_frames_start_fidget [] =
{
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL
};
mmove_t myparasite_move_start_fidget = {FRAME_stand18, FRAME_stand21, myparasite_frames_start_fidget, myparasite_do_fidget};

mframe_t myparasite_frames_fidget [] =
{	
	drone_ai_stand, 0, myparasite_scratch,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, myparasite_scratch,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL
};
mmove_t myparasite_move_fidget = {FRAME_stand22, FRAME_stand27, myparasite_frames_fidget, myparasite_refidget};

mframe_t myparasite_frames_end_fidget [] =
{
	drone_ai_stand, 0, myparasite_scratch,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL
};
mmove_t myparasite_move_end_fidget = {FRAME_stand28, FRAME_stand35, myparasite_frames_end_fidget, myparasite_stand};

void myparasite_end_fidget (edict_t *self)
{
	self->monsterinfo.currentmove = &myparasite_move_end_fidget;
}

void myparasite_do_fidget (edict_t *self)
{
	self->monsterinfo.currentmove = &myparasite_move_fidget;
}

void myparasite_refidget (edict_t *self)
{ 
	if (random() <= 0.8)
		self->monsterinfo.currentmove = &myparasite_move_fidget;
	else
		self->monsterinfo.currentmove = &myparasite_move_end_fidget;
}

void myparasite_idle (edict_t *self)
{ 
	self->monsterinfo.currentmove = &myparasite_move_start_fidget;
}


mframe_t myparasite_frames_stand [] =
{
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, myparasite_tap,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, myparasite_tap,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, myparasite_tap,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, myparasite_tap,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, myparasite_tap,
	drone_ai_stand, 0, NULL,
	drone_ai_stand, 0, myparasite_tap
};
mmove_t	myparasite_move_stand = {FRAME_stand01, FRAME_stand17, myparasite_frames_stand, myparasite_stand};

void myparasite_stand (edict_t *self)
{
	self->monsterinfo.currentmove = &myparasite_move_stand;
}

mframe_t parasite_frames_walk [] =
{
	drone_ai_walk, 30, NULL,
	drone_ai_walk, 30, NULL,
	drone_ai_walk, 22, NULL,
	drone_ai_walk, 19, NULL,
	drone_ai_walk, 24, NULL,
	drone_ai_walk, 28, NULL,
	drone_ai_walk, 25, NULL
};
mmove_t parasite_move_walk = {FRAME_run03, FRAME_run09, parasite_frames_walk, parasite_walk};

mframe_t parasite_frames_start_walk [] =
{
	drone_ai_walk, 0,	NULL,
	drone_ai_walk, 30, parasite_walk
};
mmove_t parasite_move_start_walk = {FRAME_run01, FRAME_run02, parasite_frames_start_walk, NULL};

mframe_t parasite_frames_stop_walk [] =
{	
	drone_ai_walk, 20, NULL,
	drone_ai_walk, 20,	NULL,
	drone_ai_walk, 12, NULL,
	drone_ai_walk, 10, NULL,
	drone_ai_walk, 0,  NULL,
	drone_ai_walk, 0,  NULL
};
mmove_t parasite_move_stop_walk = {FRAME_run10, FRAME_run15, parasite_frames_stop_walk, NULL};

void parasite_start_walk (edict_t *self)
{	
	if (!self->goalentity)
		self->goalentity = world;
	self->monsterinfo.currentmove = &parasite_move_start_walk;
}

void parasite_walk (edict_t *self)
{
	self->monsterinfo.currentmove = &parasite_move_walk;
}

mframe_t myparasite_frames_run [] =
{
	drone_ai_run, 35, NULL,
	drone_ai_run, 35, NULL,
	drone_ai_run, 35, NULL,
	drone_ai_run, 35, NULL,
	drone_ai_run, 35, NULL,
	drone_ai_run, 35, NULL,
	drone_ai_run, 35, NULL
};
mmove_t myparasite_move_run = {FRAME_run03, FRAME_run09, myparasite_frames_run, NULL};

mframe_t myparasite_frames_start_run [] =
{
	drone_ai_run, 30,	NULL,
	drone_ai_run, 30, NULL,
};
mmove_t myparasite_move_start_run = {FRAME_run01, FRAME_run02, myparasite_frames_start_run, myparasite_run};

mframe_t parasite_frames_pain[] =
{
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
	ai_move, 0, NULL
};
mmove_t parasite_move_pain = { FRAME_pain101, FRAME_pain111, parasite_frames_pain, myparasite_run };

void myparasite_start_run (edict_t *self)
{	
	myparasite_retract_active_proboscis(self);
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &myparasite_move_stand;
	else
		self->monsterinfo.currentmove = &myparasite_move_start_run;
}

void myparasite_run (edict_t *self)
{
	myparasite_retract_active_proboscis(self);
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &myparasite_move_stand;
	else
		self->monsterinfo.currentmove = &myparasite_move_run;
}

static qboolean ParasiteCanAttack (edict_t *self, vec3_t start, vec3_t end)
{
	if (!G_ValidTarget(self, self->enemy, false, true))
		return false;
	if (entdist(self, self->enemy) > PARASITE_PROBOSCIS_RANGE)
		return false;
	if (!infront(self, self->enemy))
	{
		if (!self->groundentity || self->groundentity != self->enemy)
			return false;
	}

	// miss the attack if we are cursed/confused
	if (que_typeexists(self->curses, CURSE) && rand() > 0.2)
		return false;

	// get starting point
	ParasiteProjectProboscisSource(self, parasite_drain_offsets[0], start);

	// make sure there is a clear shot
	return M_MonsterFindClearShot(self, start, end);
}

static void myparasite_proboscis_reset(edict_t *self)
{
	edict_t *owner;
	edict_t *segment;

	if (!myparasite_proboscis_inuse(self))
		return;

	owner = self->owner;
	segment = self->proboscis;

	if (myparasite_proboscis_inuse(segment))
	{
		segment->prethink = NULL;
		segment->owner = NULL;
		G_FreeEdict(segment);
	}

	self->proboscis = NULL;
	if (myparasite_proboscis_inuse(owner) && owner->proboscis == self)
		owner->proboscis = NULL;

	G_FreeEdict(self);
}

static void myparasite_proboscis_die(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	myparasite_proboscis_reset(self);
}

static void myparasite_proboscis_retract(edict_t *self)
{
	if (!myparasite_proboscis_inuse(self))
		return;

	if (!myparasite_proboscis_inuse(self->owner))
	{
		myparasite_proboscis_reset(self);
		return;
	}

	if (self->owner->monsterinfo.currentmove == &myparasite_move_fire_proboscis)
		self->owner->monsterinfo.nextframe = FRAME_drain12;

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	VectorClear(self->velocity);
	if (self->style != PROBOSCIS_RETRACTING)
		self->speed *= PARASITE_PROBOSCIS_RETRACT_MODIFIER;
	self->style = PROBOSCIS_RETRACTING;
	self->nextthink = level.time + FRAMETIME;
	gi.linkentity(self);
}

static void myparasite_break_noise(edict_t *self)
{
	gi.sound(self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

static void myparasite_break_retract(edict_t *self)
{
	if (myparasite_proboscis_inuse(self->proboscis))
		myparasite_proboscis_retract(self->proboscis);
}

static void myparasite_break_wait(edict_t *self)
{
	if (myparasite_proboscis_inuse(self->proboscis) &&
		self->proboscis->style != PROBOSCIS_DONE)
	{
		self->monsterinfo.nextframe = FRAME_break19;
	}
	else if (random() < 0.5f)
	{
		myparasite_reel_in(self);
		self->monsterinfo.nextframe = FRAME_break31;
	}
}

static void myparasite_break_sound(edict_t *self)
{
	if (random() < 0.5f)
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	else
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);

	self->pain_debounce_time = level.time + 3.0f;
}

static void myparasite_charge_proboscis(edict_t *self, float dist)
{
	if (self->s.frame >= FRAME_break01 && self->s.frame <= FRAME_break32)
		ai_move(self, dist);
	else
		ai_charge(self, dist);

	if (myparasite_proboscis_inuse(self->proboscis) &&
		myparasite_proboscis_inuse(self->proboscis->proboscis))
	{
		myparasite_proboscis_segment_draw(self->proboscis->proboscis);
	}
}

static void myparasite_proboscis_drain(edict_t *self)
{
	edict_t *owner = self->owner;
	edict_t *target = self->enemy;
	vec3_t dir;
	int damage;
	int pull = 0;

	if (!myparasite_proboscis_inuse(owner) ||
		!G_ValidTarget(owner, target, false, true))
		return;

	owner->lastsound = level.framenum;

	damage = PARASITE_INITIAL_DMG + PARASITE_ADDON_DMG * drone_damagelevel(owner);
	if (PARASITE_MAX_DMG && damage > PARASITE_MAX_DMG)
		damage = PARASITE_MAX_DMG;
	damage = vrx_increase_monster_damage_by_talent(owner->activator, damage);

	if (owner->groundentity)
	{
		pull = PARASITE_INITIAL_KNOCKBACK + PARASITE_ADDON_KNOCKBACK * drone_damagelevel(owner);
		if (PARASITE_MAX_KNOCKBACK && pull < PARASITE_MAX_KNOCKBACK)
			pull = PARASITE_MAX_KNOCKBACK;
		if (target->groundentity)
			pull *= 2;
	}

	if (owner->health < owner->max_health)
	{
		owner->health += damage;
		if (owner->health > owner->max_health)
			owner->health = owner->max_health;
		if (owner->health >= owner->max_health / 2)
			owner->s.skinnum = 0;
	}

	VectorSubtract(self->s.origin, owner->s.origin, dir);
	VectorNormalize(dir);
	T_Damage(target, self, owner, dir, target->s.origin,
		self->s.origin, damage, pull, DAMAGE_NO_ABILITIES, MOD_UNKNOWN);
}

static void myparasite_proboscis_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	edict_t *owner = self->owner;
	qboolean latch_target;
	vec3_t p;
	vec3_t approach;
	vec3_t normal;

	if (!myparasite_proboscis_inuse(owner))
	{
		myparasite_proboscis_reset(self);
		return;
	}

	if (surf && (surf->flags & SURF_SKY))
	{
		myparasite_proboscis_reset(self);
		return;
	}

	if (other == owner || owner->monsterinfo.currentmove != &myparasite_move_fire_proboscis)
		return;

	latch_target = G_ValidTarget(owner, other, false, true) &&
		(other == owner->enemy || other->client);

	if (plane)
		VectorCopy(plane->normal, normal);
	else
		VectorClear(normal);

	VectorCopy(self->s.origin, p);

	if (latch_target)
	{
		VectorCopy(self->velocity, approach);
		if (!VectorNormalize(approach))
		{
			myparasite_get_proboscis_start(owner, approach);
			VectorSubtract(self->s.origin, approach, approach);
			VectorNormalize(approach);
		}
		VectorMA(self->s.origin, -12, approach, p);

		owner->monsterinfo.nextframe = FRAME_drain06;
		self->movetype = MOVETYPE_NONE;
		self->solid = SOLID_NOT;
		self->style = PROBOSCIS_LATCHED;
		self->enemy = other;
		VectorSubtract(p, other->s.origin, self->move_origin);
		self->s.alpha = 0.35f;
		self->s.renderfx |= RF_TRANSLUCENT;
		VectorClear(self->velocity);
		gi.sound(self, CHAN_WEAPON, sound_suck, 1, ATTN_NORM, 0);
	}
	else
	{
		if (plane)
			VectorMA(self->s.origin, 1, plane->normal, p);

		if (other && (other->svflags & (SVF_MONSTER | SVF_DEADMONSTER)))
		{
			myparasite_proboscis_retract(self);
		}
		else
		{
			owner->monsterinfo.currentmove = &myparasite_move_break;
			owner->monsterinfo.nextframe = 0;
			owner->s.angles[YAW] = self->s.angles[YAW];

			self->movetype = MOVETYPE_NONE;
			self->solid = SOLID_NOT;
			self->style = PROBOSCIS_LATCHED;
			self->enemy = NULL;
			VectorClear(self->velocity);
		}
	}

	if (other && other->takedamage && G_ValidTarget(owner, other, false, false))
	{
		T_Damage(other, self, owner, normal, self->s.origin,
			normal, PARASITE_PROBOSCIS_IMPACT_DAMAGE, 0,
			DAMAGE_NO_ABILITIES, MOD_UNKNOWN);
	}

	gi.positioned_sound(self->s.origin, owner, CHAN_AUTO, sound_impact, 1, ATTN_NORM, 0);

	VectorCopy(p, self->s.origin);
	self->nextthink = level.time + FRAMETIME;
	gi.linkentity(self);
}

static void myparasite_proboscis_think(edict_t *self)
{
	edict_t *owner = self->owner;
	vec3_t start, dir, to_target, from_owner;
	float dist;
	trace_t tr;

	if (!myparasite_proboscis_inuse(owner) || owner->deadflag == DEAD_DEAD || owner->health <= 0)
	{
		myparasite_proboscis_reset(self);
		return;
	}

	self->nextthink = level.time + FRAMETIME;

	if (self->style == PROBOSCIS_DONE)
	{
		myparasite_proboscis_reset(self);
		return;
	}

	if (self->style == PROBOSCIS_RETRACTING)
	{
		myparasite_get_proboscis_start(owner, start);
		VectorSubtract(self->s.origin, start, dir);
		dist = VectorNormalize(dir);

		if (!dist || dist <= self->speed * FRAMETIME * 2.0f)
		{
			self->style = PROBOSCIS_DONE;
			VectorCopy(start, self->s.origin);
			VectorClear(self->velocity);
			self->think = myparasite_proboscis_reset;
			self->nextthink = level.time + FRAMETIME;
			gi.linkentity(self);
			return;
		}

		VectorMA(self->s.origin, -self->speed * FRAMETIME, dir, self->s.origin);
		vectoangles(dir, self->s.angles);
		gi.linkentity(self);
		return;
	}

	if (self->style == PROBOSCIS_LATCHED)
	{
		if (!self->enemy)
		{
			gi.linkentity(self);
			return;
		}

		if (!G_ValidTarget(owner, self->enemy, false, true))
		{
			myparasite_proboscis_retract(self);
			return;
		}

		VectorCopy(self->s.origin, self->s.old_origin);
		VectorAdd(self->enemy->s.origin, self->move_origin, self->s.origin);
		myparasite_get_proboscis_start(owner, start);
		VectorSubtract(self->s.origin, start, dir);
		if (VectorNormalize(dir))
			vectoangles(dir, self->s.angles);

		tr = gi.trace(start, NULL, NULL, self->s.origin, owner, MASK_SOLID);
		if (tr.fraction != 1.0f)
		{
			VectorCopy(self->s.old_origin, self->s.origin);
			myparasite_proboscis_retract(self);
			return;
		}

		if (self->timestamp <= level.time)
		{
			myparasite_proboscis_drain(self);
			self->timestamp = level.time + PARASITE_PROBOSCIS_DRAIN_INTERVAL;
		}

		gi.linkentity(self);
		return;
	}

	if (!G_ValidTarget(owner, owner->enemy, false, true))
	{
		myparasite_proboscis_retract(self);
		return;
	}

	VectorSubtract(self->s.origin, owner->enemy->s.origin, to_target);
	dist = VectorNormalize(to_target);
	if (dist > (self->speed * 2.0f) / 15.0f)
	{
		VectorSubtract(self->s.origin, owner->s.origin, from_owner);
		VectorNormalize(from_owner);
		if (DotProduct(to_target, from_owner) > 0.0f)
			myparasite_proboscis_retract(self);
	}
}

static void myparasite_proboscis_segment_draw(edict_t *self)
{
	edict_t *tip = self->owner;
	edict_t *owner;
	vec3_t start, dir;

	if (!myparasite_proboscis_inuse(tip) ||
		!myparasite_proboscis_inuse(tip->owner))
	{
		G_FreeEdict(self);
		return;
	}

	owner = tip->owner;
	if (owner->proboscis != tip)
	{
		G_FreeEdict(self);
		return;
	}

	myparasite_get_proboscis_start(owner, start);
	VectorCopy(start, self->s.origin);
	VectorSubtract(tip->s.origin, start, dir);
	if (VectorNormalize(dir))
		VectorMA(tip->s.origin, -8, dir, self->s.old_origin);
	else
		VectorCopy(tip->s.origin, self->s.old_origin);
	gi.linkentity(self);
}

static void myparasite_fire_proboscis_entity(edict_t *self, vec3_t start, vec3_t dir)
{
	edict_t *tip;
	edict_t *segment;
	trace_t tr;
	vec3_t end;

	VectorNormalize(dir);

	tip = G_Spawn();
	VectorCopy(start, tip->s.origin);
	VectorCopy(start, tip->s.old_origin);
	vectoangles(dir, tip->s.angles);
	tip->s.modelindex = gi.modelindex("models/monsters/parasite/tip/tris.md2");
	tip->movetype = MOVETYPE_FLYMISSILE;
	tip->clipmask = MASK_SHOT & ~CONTENTS_DEADMONSTER;
	tip->solid = SOLID_BBOX;
	tip->svflags |= SVF_PROJECTILE;
	tip->owner = self;
	tip->speed = PARASITE_PROBOSCIS_SPEED;
	VectorScale(dir, tip->speed, tip->velocity);
	tip->takedamage = DAMAGE_YES;
	tip->health = 1;
	tip->flags |= FL_NO_KNOCKBACK;
	tip->touch = myparasite_proboscis_touch;
	tip->think = myparasite_proboscis_think;
	tip->die = myparasite_proboscis_die;
	tip->nextthink = level.time + FRAMETIME;
	tip->style = PROBOSCIS_FLYING;
	tip->timestamp = level.time + PARASITE_PROBOSCIS_DRAIN_INTERVAL;
	tip->classname = "parasite_proboscis_tip";
	VectorClear(tip->mins);
	VectorClear(tip->maxs);

	segment = G_Spawn();
	segment->s.modelindex = gi.modelindex("models/monsters/parasite/segment/tris.md2");
	segment->s.renderfx = RF_BEAM;
	segment->movetype = MOVETYPE_NONE;
	segment->solid = SOLID_NOT;
	segment->owner = tip;
	segment->prethink = myparasite_proboscis_segment_draw;
	segment->classname = "parasite_proboscis_segment";
	tip->proboscis = segment;
	self->proboscis = tip;

	myparasite_proboscis_segment_draw(segment);
	gi.linkentity(tip);
	gi.linkentity(segment);

	VectorMA(start, FRAMETIME, tip->velocity, end);
	tr = gi.trace(start, NULL, NULL, end, self, tip->clipmask);
	if (tr.startsolid || tr.fraction < 1.0f)
	{
		VectorCopy(tr.endpos, tip->s.origin);
		if (tr.startsolid)
			VectorScale(dir, -1, tr.plane.normal);
		tip->touch(tip, tr.ent ? tr.ent : world, &tr.plane, tr.surface);
	}
}

static void myparasite_fire_proboscis(edict_t *self)
{
	vec3_t start, forward;

	if (!G_ValidTarget(self, self->enemy, false, true))
	{
		self->monsterinfo.nextframe = FRAME_drain14;
		return;
	}

	if (myparasite_proboscis_inuse(self->proboscis))
		myparasite_proboscis_reset(self->proboscis);

	myparasite_get_proboscis_start(self, start);
	if (!M_MonsterHasClearShotFrom(self, start))
	{
		self->monsterinfo.nextframe = FRAME_drain14;
		return;
	}

	MonsterAim(self, M_PROJECTILE_ACC, PARASITE_PROBOSCIS_SPEED, false, -1, forward, start);
	if (!VectorNormalize(forward))
		AngleVectors(self->s.angles, forward, NULL, NULL);

	myparasite_fire_proboscis_entity(self, start, forward);
}

static void myparasite_proboscis_wait(edict_t *self)
{
	if (!myparasite_proboscis_inuse(self->proboscis) ||
		self->proboscis->style >= PROBOSCIS_RETRACTING)
	{
		self->monsterinfo.nextframe = FRAME_drain12;
		return;
	}

	if (self->proboscis->style == PROBOSCIS_LATCHED)
	{
		self->monsterinfo.nextframe = FRAME_drain06;
		return;
	}

	if (self->s.frame == FRAME_drain04)
		self->monsterinfo.nextframe = FRAME_drain05;
	else
		self->monsterinfo.nextframe = FRAME_drain04;
}

static void myparasite_proboscis_pull_wait(edict_t *self)
{
	if (!myparasite_proboscis_inuse(self->proboscis) ||
		self->proboscis->style == PROBOSCIS_DONE)
	{
		self->monsterinfo.nextframe = FRAME_drain14;
		return;
	}

	if (self->proboscis->style != PROBOSCIS_RETRACTING)
		myparasite_proboscis_retract(self->proboscis);

	if (self->s.frame == FRAME_drain12)
		self->monsterinfo.nextframe = FRAME_drain13;
	else
		self->monsterinfo.nextframe = FRAME_drain12;
}

mframe_t myparasite_frames_fire_proboscis [] =
{
	myparasite_charge_proboscis, 0,	myparasite_launch,
	myparasite_charge_proboscis, 0,	NULL,
	myparasite_charge_proboscis, 15,	myparasite_fire_proboscis,
	myparasite_charge_proboscis, 0,	myparasite_proboscis_wait,
	myparasite_charge_proboscis, 0,	myparasite_proboscis_wait,
	myparasite_charge_proboscis, 0,	NULL,
	myparasite_charge_proboscis, 0,	NULL,
	myparasite_charge_proboscis, -2,	NULL,
	myparasite_charge_proboscis, -2,	NULL,
	myparasite_charge_proboscis, -3,	NULL,
	myparasite_charge_proboscis, -2,	NULL,
	myparasite_charge_proboscis, 0,	myparasite_proboscis_pull_wait,
	myparasite_charge_proboscis, -1,	myparasite_proboscis_pull_wait,
	myparasite_charge_proboscis, 0,	myparasite_reel_in,
	myparasite_charge_proboscis, -2,	NULL,
	myparasite_charge_proboscis, -2,	NULL,
	myparasite_charge_proboscis, -3,	NULL,
	myparasite_charge_proboscis, 0,	NULL
};
mmove_t myparasite_move_fire_proboscis = {FRAME_drain01, FRAME_drain18, myparasite_frames_fire_proboscis, myparasite_start_run};

void myparasite_continue (edict_t *self)
{
	myparasite_checkattack(self);
}

mframe_t myparasite_frames_break [] =
{
	myparasite_charge_proboscis, 0,	NULL,
	myparasite_charge_proboscis, -3,	myparasite_break_noise,
	myparasite_charge_proboscis, 1,	NULL,
	myparasite_charge_proboscis, 2,	NULL,
	myparasite_charge_proboscis, -3,	NULL,
	myparasite_charge_proboscis, 1,	NULL,
	myparasite_charge_proboscis, 1,	NULL,
	myparasite_charge_proboscis, 3,	NULL,
	myparasite_charge_proboscis, 0,	myparasite_break_noise,
	myparasite_charge_proboscis, -18,	NULL,
	myparasite_charge_proboscis, 3,	NULL,
	myparasite_charge_proboscis, 9,	NULL,
	myparasite_charge_proboscis, 6,	NULL,
	myparasite_charge_proboscis, 0,	NULL,
	myparasite_charge_proboscis, -18,	NULL,
	myparasite_charge_proboscis, 0,	NULL,
	myparasite_charge_proboscis, 8,	myparasite_break_retract,
	myparasite_charge_proboscis, 9,	NULL,
	myparasite_charge_proboscis, 0,	myparasite_break_wait,
	myparasite_charge_proboscis, -18,	myparasite_break_sound,
	myparasite_charge_proboscis, 0,	NULL,
	myparasite_charge_proboscis, 0,	NULL,
	myparasite_charge_proboscis, 0,	NULL,
	myparasite_charge_proboscis, 0,	NULL,
	myparasite_charge_proboscis, 0,	NULL,
	myparasite_charge_proboscis, 0,	NULL,
	myparasite_charge_proboscis, 0,	NULL,
	myparasite_charge_proboscis, 4,	NULL,
	myparasite_charge_proboscis, 11,	NULL,
	myparasite_charge_proboscis, -2,	NULL,
	myparasite_charge_proboscis, -5,	NULL,
	myparasite_charge_proboscis, 1,	NULL
};
mmove_t myparasite_move_break = {FRAME_break01, FRAME_break32, myparasite_frames_break, myparasite_start_run};

/*
=== 
Break Stuff Ends
===
*/
void myparasite_checkattack (edict_t *self)
{
	vec3_t start, end;

	if (!ParasiteCanAttack(self, start, end))
		return;

	self->monsterinfo.currentmove = &myparasite_move_fire_proboscis;

	// don't call the attack function again for awhile!
	//self->monsterinfo.attack_finished = level.time + 1;
}

void myparasite_attack1 (edict_t *self)
{
	if (!self->enemy)
		return;
	if (!self->enemy->inuse)
		return;

	myparasite_checkattack(self);
}

/*
===
Death Stuff Starts
===
*/

void myparasite_dead (edict_t *self)
{
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	//self->nextthink = 0;
	gi.linkentity (self);
	M_PrepBodyRemoval(self);
}

static void myparasite_shrink(edict_t *self)
{
	self->maxs[2] = 0;
	self->svflags |= SVF_DEADMONSTER;
	gi.linkentity(self);
}
mframe_t myparasite_frames_death [] =
{
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL,
	ai_move, 0,	 NULL
};
mmove_t myparasite_move_death = {FRAME_death101, FRAME_death107, myparasite_frames_death, myparasite_dead};

void myparasite_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (myparasite_proboscis_inuse(self->proboscis))
		myparasite_proboscis_reset(self->proboscis);

	M_Notify(self);

#ifdef OLD_NOLAG_STYLE
	// reduce lag by removing the entity right away
	if (nolag->value)
	{
		M_Remove(self, false, true);
		return;
	}
#endif

	//if (self->deadflag != DEAD_DEAD)
	//	level.total_monsters--;

// check for gib
	if (self->health <= self->gib_health)
	{
		gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/udeath.wav"), 1, ATTN_NORM, 0);
		vrx_throw_drone_gibs(self, damage);
		//self->deadflag = DEAD_DEAD;
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

// regular death
	gi.sound (self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);
	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;
	self->monsterinfo.currentmove = &myparasite_move_death;

	DroneList_Remove(self);
	if (self->activator && !self->activator->client)
	{
		self->activator->num_monsters_real--;
		// gi.bprintf(PRINT_HIGH, "releasing %p (%d)\n", self, self->activator->num_monsters_real);
	}
}

/*
===
End Death Stuff
===
*/

void myparasite_touch(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf) {
	vec3_t		start;
	vec3_t		forward, right;
	vec3_t		offset;

	if (other && other->client && self->activator && self->activator == other) {
		AngleVectors (other->client->v_angle, forward, right, NULL);
		VectorScale (forward, -3, other->client->kick_origin);
		VectorSet(offset, 0, 7,  other->viewheight-8);
		P_ProjectSource (other->client, other->s.origin, offset, forward, right, start);

		self->velocity[0] += forward[0] * 50;
		self->velocity[1] += forward[1] * 50;
		self->velocity[2] += forward[2] * 50;
//		if (self->groundentity)
//			self->velocity[2] = 250;
	}
}

void myparasite_melee (edict_t *self)
{
	// prevent circle-strafing
}

void myparasite_pain(edict_t* self, edict_t* other, float kick, int damage)
{
	const double rng = random();
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	myparasite_retract_active_proboscis(self);

	// we're already in a pain state
	if (self->monsterinfo.currentmove == &parasite_move_pain)
		return;

	// monster players don't get pain state induced
	if (G_GetClient(self))
		return;

	// no pain in invasion hard mode
	if (invasion->value == 2)
		return;

	// if we're fidgeting, always go into pain state.
	if (rng <= (1.0f - self->monsterinfo.pain_chance) &&
		self->monsterinfo.currentmove != &myparasite_move_fidget &&
		self->monsterinfo.currentmove != &myparasite_move_end_fidget &&
		self->monsterinfo.currentmove != &myparasite_move_start_fidget)
		return;

	if (random() < 0.5)
	{
		gi.sound(self, CHAN_VOICE, sound_pain1, 1, ATTN_NORM, 0);
	}
	else
	{
		gi.sound(self, CHAN_VOICE, sound_pain2, 1, ATTN_NORM, 0);
	}

	self->monsterinfo.currentmove = &parasite_move_pain;
}

/*QUAKED monster_parasite (1 .5 0) (-16 -16 -24) (16 16 32) Ambush Trigger_Spawn Sight
*/
void init_drone_parasite (edict_t *self)
{
//	if (deathmatch->value)
//	{
//		G_FreeEdict (self);
//		return;
//	}

	sound_pain1 = gi.soundindex ("parasite/parpain1.wav");	
	sound_pain2 = gi.soundindex ("parasite/parpain2.wav");	
	sound_die = gi.soundindex ("parasite/pardeth1.wav");	
	sound_launch = gi.soundindex("parasite/paratck1.wav");
	sound_impact = gi.soundindex("parasite/paratck2.wav");
	sound_suck = gi.soundindex("parasite/paratck3.wav");
	sound_reelin = gi.soundindex("parasite/paratck4.wav");
	sound_sight = gi.soundindex("parasite/parsght1.wav");
	sound_tap = gi.soundindex("parasite/paridle1.wav");
	sound_scratch = gi.soundindex("parasite/paridle2.wav");
	sound_search = gi.soundindex("parasite/parsrch1.wav");

	self->s.modelindex = gi.modelindex ("models/monsters/parasite/tris.md2");
	gi.modelindex("models/monsters/parasite/tip/tris.md2");
	gi.modelindex("models/monsters/parasite/segment/tris.md2");
	VectorSet (self->mins, -16, -16, -24);
	VectorSet (self->maxs, 16, 16, 24);
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;

	//if (self->activator && self->activator->client)
	self->health = M_PARASITE_INITIAL_HEALTH + M_PARASITE_ADDON_HEALTH*self->monsterinfo.level; // hlt: parasite
	//else self->health = 200 + 80*self->monsterinfo.level;

	self->max_health = self->health;
	self->gib_health = -BASE_GIB_HEALTH;
	self->mass = 100;

	self->pain = myparasite_pain;
	self->die = myparasite_die;
//	self->touch = myparasite_touch;

	self->monsterinfo.stand = myparasite_stand;
	self->monsterinfo.walk = parasite_start_walk;
	self->monsterinfo.jumpdn = 512;
	self->monsterinfo.jumpup = 32;
	self->monsterinfo.run = myparasite_start_run;
	self->monsterinfo.attack = myparasite_attack1;
	self->monsterinfo.sight = myparasite_sight;
	//self->monsterinfo.idle = myparasite_idle;

	//K03 Begin
	M_SetMonsterArmor(self, M_PARASITE_INITIAL_ARMOR + M_PARASITE_ADDON_ARMOR*self->monsterinfo.level);
	self->monsterinfo.control_cost = M_PARASITE_CONTROL_COST;
	self->monsterinfo.cost = M_PARASITE_COST;
	self->monsterinfo.aiflags |= AI_NO_CIRCLE_STRAFE;
	self->monsterinfo.melee = myparasite_melee;
	self->monsterinfo.pain_chance = 0.25f;
	//self->monsterinfo.melee = 1;
	self->mtype = M_PARASITE;
	//K03 End

	gi.linkentity (self);

	self->monsterinfo.currentmove = &myparasite_move_stand;	
	self->monsterinfo.scale = MODEL_SCALE;

//	walkmonster_start (self);
	self->nextthink = level.time + FRAMETIME;

	//self->activator->num_monsters += self->monsterinfo.control_cost;
}
