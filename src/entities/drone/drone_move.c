// m_move.c -- monster movement

#include <pthread.h>

#include "g_local.h"


/*
=============
M_CheckBottom

Returns false if any part of the bottom of the entity is off an edge that
is not a staircase.

=============
*/
// static int c_yes, c_no;

qboolean M_CheckBottom (edict_t *ent)
{
	vec3_t	mins, maxs, start, stop;
	trace_t	trace;
	int		x, y;
	float	mid, bottom;
	//GHz START - minimum step size based on the size of monster's hitbox
	float	step = 2*STEPSIZE;
	if (ent->size[0] > 48)
		step = 64;
	//GHz END

	VectorAdd (ent->s.origin, ent->mins, mins);
	VectorAdd (ent->s.origin, ent->maxs, maxs);

// if all of the points under the corners are solid world, don't bother
// with the tougher checks
// the corners must be within 16 of the midpoint
	start[2] = mins[2] - 1;
	for	(x=0 ; x<=1 ; x++)
		for	(y=0 ; y<=1 ; y++)
		{
			start[0] = x ? maxs[0] : mins[0];
			start[1] = y ? maxs[1] : mins[1];
			if (gi.pointcontents (start) != CONTENTS_SOLID)
				goto realcheck;
		}

	// c_yes++;
	return true;		// we got out easy

realcheck:
	// c_no++;
//
// check it for real...
//
	start[2] = mins[2];
	
// the midpoint must be within 16 of the bottom
	start[0] = stop[0] = (mins[0] + maxs[0])*0.5;
	start[1] = stop[1] = (mins[1] + maxs[1])*0.5;
	stop[2] = start[2] - step;//2 * STEPSIZE; GHz - min stepsize is an attempted fix for large monster hitboxes having trouble on ramps
	trace = gi.trace (start, vec3_origin, vec3_origin, stop, ent,MASK_PLAYERSOLID /*MASK_MONSTERSOLID*/);

	if (trace.fraction == 1.0)
		return false;
	mid = bottom = trace.endpos[2];
	
// the corners must be within 16 of the midpoint	
	for	(x=0 ; x<=1 ; x++)
		for	(y=0 ; y<=1 ; y++)
		{
			start[0] = stop[0] = x ? maxs[0] : mins[0];
			start[1] = stop[1] = y ? maxs[1] : mins[1];
			
			trace = gi.trace (start, vec3_origin, vec3_origin, stop, ent, MASK_PLAYERSOLID /*MASK_MONSTERSOLID*/);
			
			if (trace.fraction != 1.0 && trace.endpos[2] > bottom)
				bottom = trace.endpos[2];
			//FIXME: bosses often fail this last check on ramps, where their large hitbox hangs in the air
			if (trace.fraction == 1.0 || mid - trace.endpos[2] > 0.5*step)//STEPSIZE) GHz
			{
				//gi.dprintf("fraction %.1f dist %.1f\n", trace.fraction, mid - trace.endpos[2]);
				return false;
			}
		}

	// c_yes++;
	return true;
}

bool CheckPathFall(vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs);
qboolean LandCloserToGoal (edict_t *self, vec3_t goal_pos, vec3_t landing_pos)
{
	// az: greatly simplifying this. the question is whether we get closer,
	// nothing more - we should've already decided on the path by now.
	float stepsize = STEPSIZE;
	if (self->monsterinfo.aiflags & AI_NOSTEP)
		stepsize = 1;

	// sure. fall. it's closer. or it's really just a step away.
	float dz = fabs(landing_pos[2] - goal_pos[2]);
	float cdz = fabs(self->s.origin[2] - goal_pos[2]);
	return dz < cdz || dz < stepsize;
}

qboolean CheckHazards (edict_t *self, vec3_t landing_pos)
{
	vec3_t start;

	// are we on dry ground?
	if (!self->waterlevel)
	{
		// check position 1 unit above the bottom of entity's bounding box
		VectorCopy(landing_pos, start);
		start[2] += self->mins[2] + 1;	

		// return false if the landing area is in lava or slime
		if (gi.pointcontents(start) & (CONTENTS_LAVA|CONTENTS_SLIME))
			return false;
	}

	return true;
}

void vrx_pf_get_node_position (int nodenum, vec3_t pos);
int vrx_pf_nearest_waypoint_index_along_path(vec3_t start, const nodeid_t *wp, size_t wpcount);
qboolean CanJumpDown (edict_t *self, vec3_t neworg)
{
	vec3_t	start;
	edict_t *goal;
	trace_t	tr;

	if (self->monsterinfo.jumpdn < 1)
		return false; // we can't jump down!

	// determine goal entity, if there is one
	if (self->movetarget && self->movetarget->inuse)
		goal = self->movetarget;
	else if (self->enemy && self->enemy->inuse)
		goal = self->enemy;
	else if (self->goalentity && self->goalentity->inuse)
		goal = self->goalentity;
	else
		return false;

	// az: let's just not jump at random off ledges
	if (goal == world)
		return false;

	// trace down
	VectorCopy(neworg, start);
	start[2] -= 8192;
	//tr = gi.trace(neworg, self->mins, self->maxs, start, self, MASK_MONSTERSOLID);
	tr = gi.trace(neworg, vec3_origin, vec3_origin, start, self, MASK_MONSTERSOLID);

	// the landing position is less than 1 unit down, so it's not worth it
	//if (fabs(tr.endpos[2] - self->s.origin[2]) < STEPSIZE)
	//{
	//	gi.dprintf("can't jump down, not worth it\n");
	//	return false;
	//}

	// the landing position is hazardous, don't jump!
	if (!CheckHazards(self, tr.endpos))
	{
		//gi.dprintf("can't jump down, hazard below\n");
		return false;
	}

	// are we following a path?
	if (self->monsterinfo.numWaypoints 
		&& self->monsterinfo.nextWaypoint < self->monsterinfo.numWaypoints)
	{
		
		//int nearestWpNum;
		vec3_t v;

		/*
		nearestWpNum = NearestWaypointNum(tr.endpos, self->monsterinfo.waypoint);
		gi.dprintf("nearest wp to fall: %d vs %d\n", nearestWpNum, self->monsterinfo.nextWaypoint);
		
		// is the landing position closer to the next waypoint?
		if (nearestWpNum < self->monsterinfo.nextWaypoint)
			return false;*/

		vrx_pf_get_node_position(self->monsterinfo.waypoint[self->monsterinfo.nextWaypoint], v);
		if (!LandCloserToGoal(self, v, tr.endpos))
		{
			// is the landing position closer to the final waypoint?
			vrx_pf_get_node_position(self->monsterinfo.waypoint[self->monsterinfo.numWaypoints-1], v);
			if (!LandCloserToGoal(self, v, tr.endpos))
			{
				//gi.dprintf("can't jump down, landing position farther than current position\n");
				return false;
			}
		}
		
		
		//if (self->s.origin[2] - tr.endpos[2] > STEPSIZE)
		//	return false;
		// the landing position is non-hazardous and closer to our final or next waypoint
		//gi.dprintf("jump down! org: %f %f %f fall: %f %f %f\n", self->s.origin[0], self->s.origin[1], self->s.origin[2], tr.endpos[0], tr.endpos[1], tr.endpos[2]);
		// it's important to follow paths as closely as possible, as they are already designed to avoid obstructions

		return true;
	}

	// is the landing position closer to our goal entity?
	if (!LandCloserToGoal(self, goal->s.origin, tr.endpos))
		return false;

	return true;
}

qboolean CanJumpUp (edict_t *self, vec3_t neworg, vec3_t end)
{
	int		jumpdist;
	vec3_t	angles, start;
	trace_t	tr;

	if (self->monsterinfo.jumpup < 1)
		return false; // we can't jump up!

	VectorCopy(neworg, start);
	jumpdist = 8;//STEPSIZE;

	for ( ; ; jumpdist += 8)
	{
		// we can't jump any higher
		if (jumpdist > self->monsterinfo.jumpup)
			return false;

		// each loop increments start position 1 step higher
		start[2] = neworg[2] + jumpdist;

		// trace to the floor (end position is always 1 step below floor)
		tr = gi.trace(start, self->mins, self->maxs, end, self, MASK_MONSTERSOLID);

		if (tr.ent && tr.ent != world) // az: Don't jump over anything but the world...
		    return false;

		// we have cleared the obstacle!
		if (!tr.startsolid && !tr.allsolid)
			break;
	}

	// make sure we land on a flat plane
	vectoangles(tr.plane.normal, angles);
	AngleCheck(&angles[PITCH]);
	if (angles[PITCH] != 270)
		return false;
	
	VectorCopy(tr.endpos, self->s.origin);
	//gi.dprintf("allowed monster to jump up\n");
	return true;
}

// modified SV_movestep for use with player-controlled monsters
qboolean M_Move (edict_t *ent, vec3_t move, qboolean relink)
{
	vec3_t		oldorg, neworg, end;
	trace_t		trace;//, tr;
	const float		stepsize=STEPSIZE;

// try the move	
	VectorCopy (ent->s.origin, oldorg);
	VectorAdd (ent->s.origin, move, neworg);

	neworg[2] += stepsize;
	VectorCopy (neworg, end);
	end[2] -= stepsize*2;

	trace = gi.trace (neworg, ent->mins, ent->maxs, end, ent, MASK_MONSTERSOLID);

	if (trace.allsolid)
	{
		// if we would have collided with a live entity, call its touch function
		// this prevents player-monsters from being invulnerable to obstacles
		if (G_EntIsAlive(trace.ent) && trace.ent->touch)
			trace.ent->touch(trace.ent, ent, &trace.plane, trace.surface);
		return false;
	}

	if (trace.startsolid)
	{
		neworg[2] -= stepsize;
		trace = gi.trace (neworg, ent->mins, ent->maxs, end, ent, MASK_MONSTERSOLID);
		if (trace.allsolid || trace.startsolid)
			return false;
	}

	if (trace.fraction == 1)
	{
	//	gi.dprintf("going to fall\n");
	// if monster had the ground pulled out, go ahead and fall
	//	VectorSubtract(trace.endpos, oldorg, forward);
	//	VectorMA(oldorg, 64, forward, end);
	
		if ( ent->flags & FL_PARTIALGROUND )
		{
			VectorAdd (ent->s.origin, move, ent->s.origin);
			if (relink)
			{
				gi.linkentity (ent);
				G_TouchTriggers (ent);
			}
			ent->groundentity = NULL;
			return true;
		}
	}

// check point traces down for dangling corners
	VectorCopy (trace.endpos, ent->s.origin);
	
	if (!M_CheckBottom (ent))
	{
		if (ent->flags & FL_PARTIALGROUND)
		{	// entity had floor mostly pulled out from underneath it
			// and is trying to correct
			if (relink)
			{
				gi.linkentity (ent);
				G_TouchTriggers (ent);
			}
			return true;
		}
	}
	
	if (ent->flags & FL_PARTIALGROUND)
		ent->flags &= ~FL_PARTIALGROUND;

	ent->groundentity = trace.ent;
	if (trace.ent)
		ent->groundentity_linkcount = trace.ent->linkcount;

// the move is ok
	if (relink)
	{
		gi.linkentity (ent);
		G_TouchTriggers (ent);
	}

	return true;
}

static float M_FlyFloorClearance(const edict_t *ent)
{
	switch (ent->mtype)
	{
	case M_CARRIER:
		return 104.0f;
	case M_FIXBOT:
	case M_FIXBOT_BOSS:
	case M_BOSS2_SMALL:
		return 40.0f;
	default:
		return 16.0f;
	}
}

qboolean M_MoveVertical(edict_t* ent, vec3_t dest, vec3_t neworg)
{
	float        dz;                        // delta Z
	float        idealZ;                // ideal Z position, either a destination waypoint/node or a goal entity
	const float        zSpeed = 8;        // Z movement speed
	const float        floorClearance = M_FlyFloorClearance(ent);
	vec3_t        end, goalpos;
	trace_t        trace;
	//qboolean stopOnCollision=false;
	VectorCopy(ent->s.origin, neworg);
	if (ent->monsterinfo.bump_delay > level.time || ent->monsterinfo.Zchange_delay > level.time)
		return true; // don't adjust vertically
	if (dest)
	{
		idealZ = dest[2] + 16;
		VectorCopy(dest, goalpos);
	}
	else if (ent->goalentity)
	{
		if (ent->goalentity == world && floorClearance <= 16.0f)
			return true;
		if (ent->goalentity == world)
		{
			idealZ = ent->s.origin[2];
			VectorCopy(ent->s.origin, goalpos);
		}
		else
		{
			idealZ = ent->goalentity->s.origin[2] + 16;
			VectorCopy(ent->goalentity->s.origin, goalpos);
		}
	}
	else if (floorClearance <= 16.0f)
		return true; // no goal, no destination - probably idle
	else
	{
		idealZ = ent->s.origin[2];
		VectorCopy(ent->s.origin, goalpos);
	}
	// if we can see our enemy, stay above him
	if (ent->enemy && visible(ent, ent->enemy) && entdist(ent, ent->enemy) <= 256)
		idealZ = ent->enemy->absmax[2] + 48;
	// goal position is obstructed, so vertical movement should fail if we are obstructed
	//else if (!G_IsClearPath(ent, MASK_SHOT, ent->s.origin, goalpos))
	//        stopOnCollision = true;
	//gi.dprintf("idealZ @ %.0f ", idealZ);
	// check floor height
	VectorCopy(ent->s.origin, end);
	end[2] -= 8192;
	trace = gi.trace(ent->s.origin, ent->mins, ent->maxs, end, ent, MASK_MONSTERSOLID);
	// always stay above the floor, regardless of goal/destination
	if (idealZ < trace.endpos[2] + floorClearance)
		idealZ = trace.endpos[2] + floorClearance;
	//gi.dprintf("floor @ %.0f position @ %.0f\n", trace.endpos[2], ent->s.origin[2]);
	dz = ent->s.origin[2] - idealZ;
	VectorCopy(ent->s.origin, end);
	if (dz > 0) // we are above our target
	{
		if (dz > zSpeed)
			dz = zSpeed;
		end[2] -= dz;
	}
	else if (dz < 0) // we are below our target
	{
		dz = fabs(dz);
		if (dz > zSpeed)
			dz = zSpeed;
		end[2] += dz;
	}
	else // we at the correct Z height, so we're done
		return true;
	trace = gi.trace(ent->s.origin, ent->mins, ent->maxs, end, ent, MASK_MONSTERSOLID);
	if (trace.fraction == 1)
	{
		VectorCopy(trace.endpos, neworg);
		/*if (relink)
		{
				gi.linkentity (ent);
				G_TouchTriggers (ent);
		}*/
		return true;
	}
	return false;
}

static constexpr float FLY_POSITION_UPDATE_MIN = 3.0f;
static constexpr float FLY_POSITION_UPDATE_MAX = 10.0f;
static constexpr float FLY_TURN_FACTOR_FAST = 0.45f;
static constexpr float FLY_TURN_FACTOR_BASE = 0.84f;
static constexpr float FLY_TURN_FACTOR_SPEED_SCALE = 0.08f;
static constexpr float FLY_PITCH_LERP_SPEED = 4.0f;
static constexpr float FLY_WALL_STUCK_THRESHOLD = 1.5f;
static constexpr float FLY_DESCENT_SPEED_MULTIPLIER = 0.6f;
// Fraction of fly_speed allowed as downward velocity while we can't see our enemy
static constexpr float FLY_SEARCH_DESCENT_SPEED_SCALE = 0.35f;
static constexpr float FLY_TARGET_LEAD_TIME = 0.45f;
static constexpr float FLY_CATCHUP_MARGIN = 32.0f;
static constexpr float FLY_ATTACK_SPEED_SCALE = 1.35f;
static constexpr float FLY_ATTACK_ACCEL_SCALE = 1.6f;
static constexpr float FLY_LOS_PRESERVE_SPEED_SCALE = 1.12f;
static constexpr float FLY_LOS_PRESERVE_ACCEL_SCALE = 1.30f;
static constexpr float FLY_LOS_PRESERVE_TURN_FACTOR = 0.32f;
static constexpr float FLY_LOS_PRESSURE_SNAP_DOT = 0.0f;
static constexpr float FLY_LOS_FULL_SIGHT_FRACTION = 0.98f;
static constexpr float FLY_LOS_SIDE_SWITCH_SPEED = 64.0f;
static constexpr float FLY_LOS_PRESERVE_MIN_FRACTION = 0.25f;
static constexpr float FLY_LOS_PREVENT_THRESHOLD = 0.86f;
static constexpr float FLY_LOS_PREVENT_MIN_GAIN = 0.10f;
static constexpr float FLY_LOS_PREVENT_ALLOWED_DROP = 0.06f;
static constexpr float FLY_LOS_RECOVER_MEMORY_TIME = 1.6f;
static constexpr float FLY_LOS_RECOVER_LAST_SEEN_RANGE = 512.0f;
static constexpr float FLY_LOS_RECOVER_MAX_RANGE = 1200.0f;
static constexpr float FLY_LOS_RECOVER_MIN_GAIN = 0.08f;
static constexpr float FLY_BLOCKED_DESCENT_HEIGHT = 32.0f;
static constexpr float FLY_BLOCKED_DESCENT_XY_SCALE = 0.45f;
static constexpr float FLY_COMBAT_WALL_STUCK_THRESHOLD = 0.35f;
static constexpr int FLY_SEPARATION_MAX_NEIGHBORS = 24;

typedef enum fly_los_mode_e
{
	FLY_LOS_MODE_PRESERVE,
	FLY_LOS_MODE_PREVENT,
	FLY_LOS_MODE_RECOVER
} fly_los_mode_t;

typedef struct flystep_intent_s
{
	qboolean visible_combat_enemy;
	qboolean recent_combat_memory;
	qboolean lost_combat_goal;
	qboolean path_context;
	qboolean use_dest;
} flystep_intent_t;

typedef struct flystep_ctx_s
{
	flystep_intent_t intent;
	vec3_t dir;
	vec3_t final_dir;
	vec3_t towards_origin;
	vec3_t towards_velocity;
	vec3_t wanted_pos;
	vec3_t wanted_dir;
	float current_speed;
	float dist_to_wanted;
	qboolean following_paths;
	qboolean catchup_goal;
	qboolean los_preserve_drive;
	qboolean los_recover_drive;
	qboolean water_recovery_drive;
	qboolean bad_movement_direction;
} flystep_ctx_t;

static float fly_frand_range(float min_value, float max_value)
{
	return min_value + random() * (max_value - min_value);
}

static float fly_clampf(float value, float min_value, float max_value)
{
	if (value < min_value)
		return min_value;
	if (value > max_value)
		return max_value;
	return value;
}

static void fly_lerp(vec3_t out, vec3_t a, vec3_t b, float t)
{
	out[0] = a[0] + (b[0] - a[0]) * t;
	out[1] = a[1] + (b[1] - a[1]) * t;
	out[2] = a[2] + (b[2] - a[2]) * t;
}

// Spherical interpolation, a direct port of RemasterQ2's slerp() (q_vec3.h).
// Returns 'from' at t=0 and 'to' at t=1; assumes both inputs are unit vectors.
// Remaster steers fliers with slerp rather than a normalized lerp so the turn
// arc stays at a constant angular rate regardless of how far apart the headings
// are - it tracks moving targets more cleanly than nlerp near large angles.
static void fly_slerp(vec3_t out, vec3_t from, vec3_t to, float t)
{
	float dot = DotProduct(from, to);
	float a_factor, b_factor, ang, sin_omega;

	// nearly parallel - lerp is stable and avoids the sin(0) blowup
	if (dot >= 0.9995f)
	{
		fly_lerp(out, from, to, t);
		return;
	}

	// nearly opposite - route through a perpendicular axis (matches remaster)
	if (dot <= -0.9995f)
	{
		vec3_t axis = { 1.0f, 0.0f, 0.0f };
		vec3_t c;

		CrossProduct(axis, to, c);
		if (t <= 0.5f)
			fly_lerp(out, from, c, t * 2.0f);
		else
			fly_lerp(out, c, to, (t - 0.5f) * 2.0f);
		return;
	}

	ang = acosf(dot);
	sin_omega = sinf(ang);
	a_factor = sinf((1.0f - t) * ang) / sin_omega;
	b_factor = sinf(t * ang) / sin_omega;

	out[0] = from[0] * a_factor + to[0] * b_factor;
	out[1] = from[1] * a_factor + to[1] * b_factor;
	out[2] = from[2] * a_factor + to[2] * b_factor;
}

static qboolean fly_vector_valid(vec3_t v)
{
	return isfinite(v[0]) && isfinite(v[1]) && isfinite(v[2]);
}

static float fly_entity_unit(edict_t *ent, unsigned int salt)
{
	unsigned int n = (unsigned int)(ent - g_edicts + 1);

	n ^= salt * 0x9e3779b9u;
	n ^= n >> 16;
	n *= 0x7feb352du;
	n ^= n >> 15;
	n *= 0x846ca68bu;
	n ^= n >> 16;
	return (float)(n & 0xffffu) / 65535.0f;
}

static void fly_entity_fallback_dir(edict_t *ent, vec3_t out)
{
	float angle = fly_entity_unit(ent, 0x51u) * M_PI * 2.0f;

	VectorSet(out, cosf(angle), sinf(angle), 0.2f + fly_entity_unit(ent, 0x52u) * 0.35f);
	VectorNormalize(out);
}

static qboolean fly_following_path(edict_t *ent)
{
	if (ent->monsterinfo.aiflags & (AI_FIND_NAVI | AI_COMBAT_POINT | AI_LOST_SIGHT))
		return true;

	if (ent->goalentity && ent->goalentity->inuse)
	{
		if (ent->goalentity->mtype == INVASION_NAVI || ent->goalentity->mtype == PLAYER_NAVI ||
			ent->goalentity->mtype == INVASION_PLAYERSPAWN)
			return true;
	}

	if (ent->enemy && ent->enemy->inuse && invasion->value && ent->enemy->mtype == INVASION_PLAYERSPAWN)
		return true;

	return false;
}

static qboolean fly_has_visible_combat_enemy(edict_t *ent)
{
	if (!ent->enemy || !ent->enemy->inuse)
		return false;

	if (invasion->value && ent->enemy->mtype == INVASION_PLAYERSPAWN)
		return false;

	if (!M_MonsterHasCombatSight(ent, ent->enemy))
		return false;

	ent->monsterinfo.aiflags &= ~AI_LOST_SIGHT;
	ent->monsterinfo.search_frames = 0;
	VectorCopy(ent->enemy->s.origin, ent->monsterinfo.last_sighting);
	ent->monsterinfo.trail_time = level.time;
	return true;
}

static qboolean fly_has_recent_combat_memory(edict_t *ent);

static qboolean fly_has_recent_combat_memory(edict_t *ent)
{
	if (!ent->enemy || !ent->enemy->inuse)
		return false;

	if (invasion->value && ent->enemy->mtype == INVASION_PLAYERSPAWN)
		return false;

	if (ent->monsterinfo.aiflags & AI_LOST_SIGHT)
		return false;

	return ent->monsterinfo.search_frames <= 3;
}

static qboolean fly_has_lost_combat_goal(edict_t *ent, vec3_t dest)
{
	if (!ent->enemy || !ent->enemy->inuse)
		return false;

	if (invasion->value && ent->enemy->mtype == INVASION_PLAYERSPAWN)
		return false;

	if (!(ent->monsterinfo.aiflags & AI_LOST_SIGHT))
		return false;

	if (dest)
		return true;

	if (!VectorCompare(ent->monsterinfo.last_sighting, vec3_origin))
		return true;

	return ent->goalentity && ent->goalentity->inuse && ent->goalentity == ent->enemy;
}

static qboolean fly_get_alternate_intent(edict_t *ent, vec3_t dest, flystep_intent_t *intent)
{
	flystep_intent_t local_intent;

	if (!intent)
		intent = &local_intent;

	intent->visible_combat_enemy = false;
	intent->recent_combat_memory = false;
	intent->lost_combat_goal = false;
	intent->path_context = false;
	intent->use_dest = false;

	if (!(ent->monsterinfo.aiflags & AI_ALTERNATE_FLY))
		return false;

	intent->visible_combat_enemy = fly_has_visible_combat_enemy(ent);
	intent->recent_combat_memory = intent->visible_combat_enemy || fly_has_recent_combat_memory(ent);
	intent->lost_combat_goal = fly_has_lost_combat_goal(ent, dest);
	intent->path_context = fly_following_path(ent) && !intent->visible_combat_enemy;
	intent->use_dest = dest && !intent->visible_combat_enemy;

	if (!intent->recent_combat_memory && !intent->lost_combat_goal)
		return false;

	// Invasion destination movement is used by navis/base pathing and expects an immediate origin step.
	if (dest && invasion->value && !intent->lost_combat_goal)
		return false;

	return true;
}

static qboolean fly_can_separate_from(edict_t *ent, edict_t *other)
{
	if (!other || other == ent || !G_EntIsAlive(other))
		return false;
	if (!(other->svflags & SVF_MONSTER) || other->solid == SOLID_NOT)
		return false;
	if (!(other->flags & (FL_FLY | FL_SWIM)))
		return false;
	if (!(other->monsterinfo.aiflags & AI_ALTERNATE_FLY))
		return false;
	return true;
}

static qboolean fly_apply_neighbor_separation(edict_t *ent, vec3_t wanted_pos)
{
	const float separation_radius = 112.0f;
	const float max_push = 96.0f;
	const float z_scale = 0.45f;
	edict_t *touch[FLY_SEPARATION_MAX_NEIGHBORS];
	vec3_t mins, maxs, away, separation;
	float dist, weight, push;
	int i, num;

	VectorSet(mins,
		ent->s.origin[0] - separation_radius,
		ent->s.origin[1] - separation_radius,
		ent->s.origin[2] - separation_radius);
	VectorSet(maxs,
		ent->s.origin[0] + separation_radius,
		ent->s.origin[1] + separation_radius,
		ent->s.origin[2] + separation_radius);

	num = gi.BoxEdicts(mins, maxs, touch, FLY_SEPARATION_MAX_NEIGHBORS, AREA_SOLID);
	VectorClear(separation);

	for (i = 0; i < num; ++i)
	{
		edict_t *other = touch[i];

		if (!fly_can_separate_from(ent, other))
			continue;

		VectorSubtract(ent->s.origin, other->s.origin, away);
		away[2] *= z_scale;
		dist = VectorNormalize(away);
		if (dist > separation_radius)
			continue;
		if (dist <= 0.1f)
		{
			fly_entity_fallback_dir(ent, away);
			dist = 1.0f;
		}

		weight = (separation_radius - dist) / separation_radius;
		if (ent->enemy && other->enemy == ent->enemy)
			weight *= 1.35f;
		VectorMA(separation, weight, away, separation);
	}

	push = VectorNormalize(separation);
	if (push <= 0.1f)
		return false;

	push = fly_clampf(push * max_push, 0.0f, max_push);
	VectorMA(wanted_pos, push, separation, wanted_pos);
	return true;
}

static qboolean fly_use_alternate_step(edict_t *ent, vec3_t dest)
{
	return fly_get_alternate_intent(ent, dest, NULL);
}

static void fly_reset_alternate_step(edict_t *ent)
{
	VectorClear(ent->velocity);
	ent->monsterinfo.fly_pinned = false;
	ent->monsterinfo.fly_wall_stuck_time = 0.0f;
}

static qboolean fly_has_last_sighting(edict_t *ent)
{
	return !VectorCompare(ent->monsterinfo.last_sighting, vec3_origin);
}

static void fly_face_target(edict_t *ent, vec3_t target_origin)
{
	vec3_t target_dir;

	VectorSubtract(target_origin, ent->s.origin, target_dir);
	if (VectorNormalize(target_dir) <= 0.1f)
		return;

	ent->ideal_yaw = vectoyaw(target_dir);
	M_ChangeYaw(ent);
}

static qboolean fly_clamp_to_ceiling(edict_t *ent, vec3_t wanted_pos)
{
	const float lookahead = 256.0f;
	const float clearance = 8.0f;
	vec3_t ceiling_check;
	trace_t tr;
	float desired_up;
	float trace_height;
	float highest_origin;

	desired_up = wanted_pos[2] - ent->s.origin[2];
	if (desired_up <= 1.0f)
		return false;

	VectorCopy(ent->s.origin, ceiling_check);
	trace_height = max(lookahead, desired_up + clearance);
	ceiling_check[2] += trace_height;

	tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, ceiling_check, ent, MASK_SOLID | CONTENTS_MONSTERCLIP);
	if (tr.fraction == 1.0f || tr.startsolid || tr.allsolid)
		return false;

	highest_origin = tr.endpos[2] - clearance;
	if (wanted_pos[2] <= highest_origin)
		return false;

	wanted_pos[2] = highest_origin;
	if (ent->monsterinfo.fly_pinned)
		ent->monsterinfo.fly_position_time = 0.0f;
	ent->monsterinfo.fly_pinned = false;
	return true;
}

static qboolean fly_can_descend(edict_t *ent)
{
	vec3_t descent_check;
	trace_t tr;

	VectorCopy(ent->s.origin, descent_check);
	descent_check[2] -= ent->monsterinfo.fly_acceleration;

	tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, descent_check, ent, MASK_SOLID | CONTENTS_MONSTERCLIP);
	return tr.fraction == 1.0f && !tr.startsolid && !tr.allsolid;
}

static qboolean fly_force_descent(edict_t *ent, vec3_t wanted_dir, float xy_scale)
{
	if (!fly_can_descend(ent))
		return false;

	ent->monsterinfo.fly_position_time = 0.0f;
	ent->monsterinfo.fly_pinned = false;
	wanted_dir[0] *= xy_scale;
	wanted_dir[1] *= xy_scale;
	wanted_dir[2] = -FLY_DESCENT_SPEED_MULTIPLIER;
	VectorNormalize(wanted_dir);
	return true;
}

static qboolean fly_should_descend_for_lower_path(edict_t *ent, qboolean following_paths, vec3_t towards_origin, vec3_t wanted_pos)
{
	trace_t tr;

	if (!following_paths)
		return false;
	if (ent->s.origin[2] <= towards_origin[2] + FLY_BLOCKED_DESCENT_HEIGHT)
		return false;

	tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, wanted_pos, ent, MASK_SOLID | CONTENTS_MONSTERCLIP);
	return tr.fraction < 1.0f && !tr.startsolid && !tr.allsolid;
}

static qboolean fly_consider_stair_climb_dir(edict_t *ent, vec3_t candidate, vec3_t target_dir,
	float *best_score, vec3_t best_dir)
{
	const float probe_min = 64.0f;
	const float min_trace_fraction = 0.60f;
	const float min_z_gain = 4.0f;
	vec3_t dir, end;
	trace_t tr;
	float probe_dist, z_gain, score;

	VectorCopy(candidate, dir);
	if (VectorNormalize(dir) <= 0.1f)
		return false;

	probe_dist = max(probe_min, ent->monsterinfo.fly_acceleration * 2.5f);
	VectorMA(ent->s.origin, probe_dist, dir, end);
	tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, end, ent, MASK_SOLID | CONTENTS_MONSTERCLIP);
	if (tr.startsolid || tr.allsolid || tr.fraction < min_trace_fraction)
		return false;

	z_gain = tr.endpos[2] - ent->s.origin[2];
	if (z_gain < min_z_gain)
		return false;

	score = tr.fraction * 36.0f + z_gain * 0.25f + DotProduct(dir, target_dir) * 8.0f;
	if (score <= *best_score)
		return true;

	*best_score = score;
	VectorCopy(dir, best_dir);
	return true;
}

static qboolean fly_try_climb_for_higher_target(edict_t *ent, vec3_t target_origin, vec3_t wanted_dir)
{
	const float min_height = 24.0f;
	const float max_height = 320.0f;
	const float max_range = 420.0f;
	const float forward_weight = 0.90f;
	const float up_weight = 0.75f;
	vec3_t to_target, target_dir, candidate, best_dir;
	float height_delta, horizontal_dist, best_score;
	qboolean found;

	VectorSubtract(target_origin, ent->s.origin, to_target);
	height_delta = to_target[2];
	if (height_delta < min_height || height_delta > max_height)
		return false;

	VectorCopy(to_target, target_dir);
	target_dir[2] = 0.0f;
	horizontal_dist = VectorNormalize(target_dir);
	if (horizontal_dist > max_range)
		return false;
	if (horizontal_dist <= 0.1f)
	{
		VectorCopy(wanted_dir, target_dir);
		target_dir[2] = 0.0f;
		if (VectorNormalize(target_dir) <= 0.1f)
			return false;
	}

	best_score = -9999.0f;
	VectorClear(best_dir);
	found = false;

	VectorScale(target_dir, forward_weight, candidate);
	candidate[2] = up_weight;
	found |= fly_consider_stair_climb_dir(ent, candidate, target_dir, &best_score, best_dir);

	VectorScale(target_dir, forward_weight * 0.45f, candidate);
	candidate[2] = up_weight * 1.25f;
	found |= fly_consider_stair_climb_dir(ent, candidate, target_dir, &best_score, best_dir);

	VectorCopy(wanted_dir, candidate);
	candidate[2] += up_weight;
	found |= fly_consider_stair_climb_dir(ent, candidate, target_dir, &best_score, best_dir);

	if (!found)
		return false;

	VectorCopy(best_dir, wanted_dir);
	ent->monsterinfo.fly_position_time = 0.0f;
	ent->monsterinfo.fly_pinned = false;
	return true;
}

static void G_IdealHoverPosition(edict_t *ent, qboolean path_context, vec3_t out)
{
	float theta, phi, distance_scale;
	vec3_t direction;

	VectorClear(out);

	if ((!ent->enemy && !(ent->monsterinfo.aiflags & AI_MEDIC)) ||
		path_context)
		return;

	theta = random() * M_PI * 2.0f;
	if (ent->monsterinfo.fly_above)
		phi = acosf(0.7f + random() * 0.3f);
	else if (ent->monsterinfo.fly_buzzard || (ent->monsterinfo.aiflags & AI_MEDIC))
		phi = acosf(random());
	else
		phi = acosf(random() * 0.7f);

	VectorSet(direction, sinf(phi) * cosf(theta), sinf(phi) * sinf(theta), cosf(phi));

	distance_scale = fly_frand_range(ent->monsterinfo.fly_min_distance, ent->monsterinfo.fly_max_distance);
	VectorScale(direction, distance_scale, out);
}

static qboolean fly_adjust_visible_combat_goal(edict_t *ent, vec3_t target_origin, vec3_t target_velocity, vec3_t wanted_pos, qboolean visible_combat_enemy)
{
	const float catchup_distance_scale = 0.85f;
	const float attack_distance_fraction = 0.2f;
	const float attack_strafe_speed_scale = 0.6f;
	const float attack_drift_speed_scale = 0.3f;
	const float attack_strafe_min = 32.0f;
	const float attack_strafe_max = 128.0f;
	const float attack_orbit_distance = 48.0f;
	const float attack_height_variance = 56.0f;
	const float target_strafe_min_speed = 20.0f;
	const float target_strafe_distance_scale = 0.45f;
	const float combat_min_height = 48.0f;
	const float combat_height_above_view = 24.0f;
	const float lower_target_descent_height = 96.0f;
	const float lower_target_min_height = 16.0f;
	float current_dist, min_dist, max_dist, desired_dist, attack_dist, radial_speed, catchup_step, strafe_dist, min_height;
	float attack_fraction, strafe_variance, orbit_phase, orbit;
	float target_lateral_speed, target_lateral_speed_abs;
	qboolean attack_drive, sliding_attack, force_drive, strafe_left, lower_target;
	vec3_t away, ideal_offset, predicted_target, lateral;

	if (!visible_combat_enemy || ent->monsterinfo.fly_pinned)
		return false;

	VectorSubtract(ent->s.origin, target_origin, away);
	current_dist = VectorNormalize(away);
	if (current_dist <= 0.1f)
		return false;

	min_dist = max(0.0f, ent->monsterinfo.fly_min_distance);
	max_dist = max(min_dist, ent->monsterinfo.fly_max_distance);
	desired_dist = current_dist;
	orbit_phase = level.time * (0.55f + fly_entity_unit(ent, 0x70u) * 0.65f) +
		fly_entity_unit(ent, 0x71u) * M_PI * 2.0f;
	orbit = sinf(orbit_phase);
	attack_fraction = attack_distance_fraction + ((fly_entity_unit(ent, 0x61u) - 0.5f) * 0.22f) + orbit * 0.08f;
	attack_dist = fly_clampf(min_dist + ((max_dist - min_dist) * attack_fraction), min_dist, max_dist);
	attack_drive = (ent->monsterinfo.attack_state == AS_STRAIGHT || ent->monsterinfo.attack_state == AS_SLIDING);
	sliding_attack = (ent->monsterinfo.attack_state == AS_SLIDING);
	lower_target = ent->s.origin[2] > target_origin[2] + lower_target_descent_height;
	force_drive = false;

	if (current_dist > max_dist + FLY_CATCHUP_MARGIN)
	{
		if (attack_drive)
			desired_dist = attack_dist;
		else
			desired_dist = max(min_dist, max_dist * catchup_distance_scale);
		force_drive = true;
	}
	else
	{
		radial_speed = DotProduct(ent->velocity, away) - DotProduct(target_velocity, away);
		if (radial_speed > 8.0f && current_dist > min_dist + FLY_CATCHUP_MARGIN)
		{
			catchup_step = max(FLY_CATCHUP_MARGIN, ent->monsterinfo.fly_speed * 0.35f);
			if (attack_drive)
				desired_dist = attack_dist;
			else
				desired_dist = fly_clampf(current_dist - catchup_step, min_dist, max_dist);
			force_drive = true;
		}
		else if (attack_drive)
		{
			desired_dist = attack_dist;
			force_drive = true;
		}
		else
		{
			return false;
		}
	}

	VectorScale(away, desired_dist, ideal_offset);

	if (attack_drive && !(ent->flags & FL_SWIM))
	{
		min_height = combat_min_height;
		if (ent->enemy && ent->enemy->viewheight > 0)
			min_height = max(min_height, ent->enemy->viewheight + combat_height_above_view);
		min_height += fly_entity_unit(ent, 0x62u) * attack_height_variance;
		if (ideal_offset[2] < min_height)
			ideal_offset[2] = min_height;
	}
	if (attack_drive && lower_target && !(ent->flags & FL_SWIM))
	{
		if (ideal_offset[2] > lower_target_min_height)
			ideal_offset[2] = lower_target_min_height;
		force_drive = true;
	}

	if (attack_drive)
	{
		VectorSet(lateral, -away[1], away[0], 0.0f);
		if (VectorNormalize(lateral) > 0.1f)
		{
			target_lateral_speed = DotProduct(target_velocity, lateral);
			target_lateral_speed_abs = fabs(target_lateral_speed);

			if (target_lateral_speed_abs > target_strafe_min_speed)
			{
				if (target_lateral_speed < 0.0f)
					VectorNegate(lateral, lateral);
				strafe_dist = fly_clampf(target_lateral_speed_abs * target_strafe_distance_scale,
					attack_strafe_min, attack_strafe_max);
			}
			else
			{
				strafe_left = orbit >= 0.0f;
				if (fly_entity_unit(ent, 0x63u) > 0.5f)
					strafe_left = !strafe_left;
				if (!strafe_left)
					VectorNegate(lateral, lateral);
				strafe_dist = fly_clampf(ent->monsterinfo.fly_speed *
					(sliding_attack ? attack_strafe_speed_scale : attack_drift_speed_scale),
					attack_strafe_min, attack_strafe_max);
			}

			strafe_variance = 0.60f + fly_entity_unit(ent, 0x64u) * 0.65f + fabsf(orbit) * 0.35f;
			strafe_dist = fly_clampf(strafe_dist * strafe_variance, attack_strafe_min, attack_strafe_max);
			strafe_dist = fly_clampf(strafe_dist + orbit * attack_orbit_distance,
				attack_strafe_min, attack_strafe_max);
			VectorMA(ideal_offset, strafe_dist, lateral, ideal_offset);
		}
	}

	VectorMA(target_origin, FLY_TARGET_LEAD_TIME, target_velocity, predicted_target);
	VectorAdd(predicted_target, ideal_offset, wanted_pos);
	VectorCopy(ideal_offset, ent->monsterinfo.fly_ideal_position);

	if (ent->monsterinfo.fly_position_time > level.time + 0.5f)
		ent->monsterinfo.fly_position_time = level.time + 0.5f;

	return force_drive;
}

qboolean SV_flystep_testvisposition(vec3_t start, vec3_t end, vec3_t starta, vec3_t startb, edict_t* ent)
{
	trace_t tr;

	tr = gi.trace(start, NULL, NULL, end, ent, MASK_SOLID | CONTENTS_MONSTERCLIP);
	if (tr.fraction == 1.0f)
	{
		tr = gi.trace(starta, ent->mins, ent->maxs, startb, ent, MASK_SOLID | CONTENTS_MONSTERCLIP);
		if (tr.fraction == 1.0f)
			return true;
	}

	return false;
}

// Alternate flyers steer directly, so they need local side probes instead of
// Horde's temporary ai_run pursuit goals when LOS is about to be blocked.
static float fly_sight_fraction_to_point_from(edict_t *ent, vec3_t origin, vec3_t point)
{
	vec3_t start;
	trace_t tr;

	VectorCopy(origin, start);
	if (ent->viewheight)
		start[2] += ent->viewheight;
	else
		start[2] += (ent->mins[2] + ent->maxs[2]) * 0.5f;

	if (!gi.inPVS(start, point))
		return 0.0f;

	tr = gi.trace(start, NULL, NULL, point, ent, MASK_SOLID);
	if (tr.startsolid || tr.allsolid)
		return 0.0f;

	return fly_clampf(tr.fraction, 0.0f, 1.0f);
}

static float fly_combat_sight_fraction_from(edict_t *ent, vec3_t origin, edict_t *target)
{
	vec3_t point;
	float best, fraction;

	if (!G_EntExists(target))
		return 0.0f;

	best = 0.0f;

	G_EntViewPoint(target, point);
	fraction = fly_sight_fraction_to_point_from(ent, origin, point);
	if (fraction > best)
		best = fraction;

	G_EntMidPoint(target, point);
	fraction = fly_sight_fraction_to_point_from(ent, origin, point);
	if (fraction > best)
		best = fraction;

	VectorCopy(target->s.origin, point);
	fraction = fly_sight_fraction_to_point_from(ent, origin, point);
	if (fraction > best)
		best = fraction;

	return best;
}

static float fly_predicted_combat_sight_fraction_from(edict_t *ent, vec3_t origin, edict_t *target, vec3_t target_velocity)
{
	vec3_t point;
	float best, fraction;

	if (!G_EntExists(target))
		return 0.0f;

	best = 0.0f;

	G_EntViewPoint(target, point);
	VectorMA(point, FLY_TARGET_LEAD_TIME, target_velocity, point);
	fraction = fly_sight_fraction_to_point_from(ent, origin, point);
	if (fraction > best)
		best = fraction;

	G_EntMidPoint(target, point);
	VectorMA(point, FLY_TARGET_LEAD_TIME, target_velocity, point);
	fraction = fly_sight_fraction_to_point_from(ent, origin, point);
	if (fraction > best)
		best = fraction;

	VectorMA(target->s.origin, FLY_TARGET_LEAD_TIME, target_velocity, point);
	fraction = fly_sight_fraction_to_point_from(ent, origin, point);
	if (fraction > best)
		best = fraction;

	return best;
}

static qboolean fly_recent_los_recovery_allowed(edict_t *ent, edict_t *target)
{
	if (!G_EntExists(target) || !fly_has_last_sighting(ent))
		return false;

	if (ent->monsterinfo.aiflags & AI_LOST_SIGHT)
		return false;

	if (ent->monsterinfo.trail_time <= 0.0f ||
		level.time > ent->monsterinfo.trail_time + FLY_LOS_RECOVER_MEMORY_TIME)
		return false;

	if (entdist(ent, target) > FLY_LOS_RECOVER_MAX_RANGE)
		return false;

	if (distance(target->s.origin, ent->monsterinfo.last_sighting) > FLY_LOS_RECOVER_LAST_SEEN_RANGE)
		return false;

	return true;
}

static qboolean fly_los_side_vectors(edict_t *ent, vec3_t target_dir, vec3_t target_velocity,
	vec3_t side, vec3_t other_side)
{
	vec3_t lateral;
	float lateral_speed;

	VectorSet(lateral, -target_dir[1], target_dir[0], 0.0f);
	if (VectorNormalize(lateral) <= 0.1f)
		return false;

	lateral_speed = DotProduct(target_velocity, lateral);
	if (lateral_speed < -FLY_LOS_SIDE_SWITCH_SPEED ||
		(fabsf(lateral_speed) <= FLY_LOS_SIDE_SWITCH_SPEED && fly_entity_unit(ent, 0x91u) < 0.5f))
	{
		VectorNegate(lateral, lateral);
	}

	VectorCopy(lateral, side);
	VectorNegate(side, other_side);
	return true;
}

static qboolean fly_consider_los_dir(edict_t *ent, edict_t *target, vec3_t target_velocity,
	vec3_t candidate, vec3_t wanted_dir, fly_los_mode_t mode, float baseline_sight,
	float preference, float *best_score, vec3_t best_dir)
{
	vec3_t dir, end;
	trace_t tr;
	float probe_base, accel_scale, trace_weight, dot_weight, down_bonus;
	float probe_dist, sight, future_sight, actual_sight, score;

	VectorCopy(candidate, dir);
	if (VectorNormalize(dir) <= 0.1f)
		return false;

	switch (mode)
	{
	case FLY_LOS_MODE_PRESERVE:
		probe_base = 72.0f;
		accel_scale = 2.5f;
		trace_weight = 28.0f;
		dot_weight = 4.0f;
		down_bonus = 2.0f;
		break;
	case FLY_LOS_MODE_PREVENT:
		probe_base = 72.0f;
		accel_scale = 3.0f;
		trace_weight = 24.0f;
		dot_weight = 4.0f;
		down_bonus = 1.5f;
		break;
	default:
		probe_base = 96.0f;
		accel_scale = 3.0f;
		trace_weight = 24.0f;
		dot_weight = 5.0f;
		down_bonus = 2.0f;
		break;
	}

	probe_dist = max(probe_base, ent->monsterinfo.fly_acceleration * accel_scale);
	VectorMA(ent->s.origin, probe_dist, dir, end);
	tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, end, ent, MASK_SOLID | CONTENTS_MONSTERCLIP);
	if (tr.startsolid || tr.allsolid || tr.fraction < FLY_LOS_PRESERVE_MIN_FRACTION)
		return false;

	switch (mode)
	{
	case FLY_LOS_MODE_PRESERVE:
		sight = fly_combat_sight_fraction_from(ent, tr.endpos, target);
		if (sight < FLY_LOS_FULL_SIGHT_FRACTION)
			return false;
		score = sight * 100.0f;
		break;
	case FLY_LOS_MODE_PREVENT:
		future_sight = fly_predicted_combat_sight_fraction_from(ent, tr.endpos, target, target_velocity);
		actual_sight = fly_combat_sight_fraction_from(ent, tr.endpos, target);
		if (actual_sight < FLY_LOS_FULL_SIGHT_FRACTION &&
			future_sight < baseline_sight + FLY_LOS_PREVENT_MIN_GAIN)
			return false;
		if (future_sight < baseline_sight + FLY_LOS_PREVENT_MIN_GAIN &&
			future_sight < FLY_LOS_FULL_SIGHT_FRACTION)
			return false;
		score = future_sight * 100.0f + actual_sight * 30.0f;
		break;
	default:
		sight = fly_combat_sight_fraction_from(ent, tr.endpos, target);
		if (sight < FLY_LOS_FULL_SIGHT_FRACTION && sight < baseline_sight + FLY_LOS_RECOVER_MIN_GAIN)
			return false;
		score = sight * 100.0f;
		break;
	}

	score += tr.fraction * trace_weight + DotProduct(dir, wanted_dir) * dot_weight + preference;
	if (dir[2] < -0.1f)
		score += down_bonus;

	if (score <= *best_score)
		return true;

	*best_score = score;
	VectorCopy(dir, best_dir);
	return true;
}

static qboolean fly_consider_weighted_los_dir(edict_t *ent, edict_t *target, vec3_t target_velocity,
	vec3_t side, float side_weight, vec3_t forward, float forward_weight, vec3_t down,
	float down_weight, vec3_t wall, float wall_weight, vec3_t wanted_dir, fly_los_mode_t mode,
	float baseline_sight, float preference, float *best_score, vec3_t best_dir)
{
	vec3_t candidate;

	VectorScale(side, side_weight, candidate);
	VectorMA(candidate, forward_weight, forward, candidate);
	VectorMA(candidate, down_weight, down, candidate);
	VectorMA(candidate, wall_weight, wall, candidate);
	return fly_consider_los_dir(ent, target, target_velocity, candidate, wanted_dir,
		mode, baseline_sight, preference, best_score, best_dir);
}

static qboolean fly_try_preserve_combat_sight(edict_t *ent, vec3_t towards_origin, vec3_t target_velocity,
	vec3_t wanted_dir, vec3_t obstacle_normal, vec3_t out_dir)
{
	vec3_t to_target, side, other_side, down, wall_push, best_dir;
	float best_score;
	qboolean found;

	if (!ent->enemy || !ent->enemy->inuse)
		return false;

	VectorSubtract(towards_origin, ent->s.origin, to_target);
	to_target[2] = 0.0f;
	if (VectorNormalize(to_target) <= 0.1f)
		return false;

	if (!fly_los_side_vectors(ent, to_target, target_velocity, side, other_side))
		return false;

	VectorSet(down, 0.0f, 0.0f, -1.0f);
	VectorCopy(obstacle_normal, wall_push);
	wall_push[2] = min(wall_push[2], 0.0f);
	if (VectorNormalize(wall_push) <= 0.1f)
		VectorClear(wall_push);

	best_score = -9999.0f;
	VectorClear(best_dir);
	found = false;

	found |= fly_consider_weighted_los_dir(ent, ent->enemy, target_velocity, side, 1.05f,
		to_target, 0.75f, down, 0.0f, wall_push, 0.35f, wanted_dir, FLY_LOS_MODE_PRESERVE,
		0.0f, 0.0f, &best_score, best_dir);
	found |= fly_consider_weighted_los_dir(ent, ent->enemy, target_velocity, side, 1.05f,
		to_target, 0.0f, down, 0.55f, wall_push, 0.35f, wanted_dir, FLY_LOS_MODE_PRESERVE,
		0.0f, 0.0f, &best_score, best_dir);
	found |= fly_consider_weighted_los_dir(ent, ent->enemy, target_velocity, side, 1.05f,
		to_target, 0.0f, down, 0.0f, wall_push, 0.35f, wanted_dir, FLY_LOS_MODE_PRESERVE,
		0.0f, 0.0f, &best_score, best_dir);
	found |= fly_consider_weighted_los_dir(ent, ent->enemy, target_velocity, other_side, 1.05f,
		to_target, 0.75f, down, 0.0f, wall_push, 0.35f, wanted_dir, FLY_LOS_MODE_PRESERVE,
		0.0f, 0.0f, &best_score, best_dir);
	found |= fly_consider_weighted_los_dir(ent, ent->enemy, target_velocity, other_side, 1.05f,
		to_target, 0.0f, down, 0.55f, wall_push, 0.35f, wanted_dir, FLY_LOS_MODE_PRESERVE,
		0.0f, 0.0f, &best_score, best_dir);
	found |= fly_consider_weighted_los_dir(ent, ent->enemy, target_velocity, vec3_origin, 0.0f,
		to_target, 0.75f, down, 0.55f, wall_push, 0.35f, wanted_dir, FLY_LOS_MODE_PRESERVE,
		0.0f, 0.0f, &best_score, best_dir);

	if (!found)
		return false;

	VectorCopy(best_dir, out_dir);
	return true;
}

static qboolean fly_try_prevent_combat_sight_loss(edict_t *ent, vec3_t target_origin, vec3_t target_velocity,
	vec3_t wanted_dir, vec3_t out_dir)
{
	const float probe_min = 96.0f;
	vec3_t to_target, side, other_side, down, best_dir, intended_end;
	trace_t tr;
	float current_future_sight, intended_future_sight, best_score, side_preference, other_preference, probe_dist;
	qboolean found;

	if (!G_EntExists(ent->enemy))
		return false;

	current_future_sight = fly_predicted_combat_sight_fraction_from(ent, ent->s.origin, ent->enemy, target_velocity);
	intended_future_sight = current_future_sight;
	probe_dist = max(probe_min, ent->monsterinfo.fly_acceleration * 3.0f);
	VectorMA(ent->s.origin, probe_dist, wanted_dir, intended_end);
	tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, intended_end, ent, MASK_SOLID | CONTENTS_MONSTERCLIP);
	if (!tr.startsolid && !tr.allsolid && tr.fraction >= FLY_LOS_PRESERVE_MIN_FRACTION)
		intended_future_sight = fly_predicted_combat_sight_fraction_from(ent, tr.endpos, ent->enemy, target_velocity);

	if (current_future_sight >= FLY_LOS_PREVENT_THRESHOLD &&
		intended_future_sight >= current_future_sight - FLY_LOS_PREVENT_ALLOWED_DROP)
		return false;
	current_future_sight = min(current_future_sight, intended_future_sight);

	VectorSubtract(target_origin, ent->s.origin, to_target);
	to_target[2] = 0.0f;
	if (VectorNormalize(to_target) <= 0.1f)
		return false;

	if (!fly_los_side_vectors(ent, to_target, target_velocity, side, other_side))
		return false;

	VectorSet(down, 0.0f, 0.0f, -1.0f);
	VectorClear(best_dir);
	best_score = -9999.0f;
	side_preference = 5.0f + fly_entity_unit(ent, 0x94u);
	other_preference = fly_entity_unit(ent, 0x95u);
	found = false;

	found |= fly_consider_weighted_los_dir(ent, ent->enemy, target_velocity, side, 1.15f,
		to_target, 0.85f, down, 0.0f, vec3_origin, 0.0f, to_target, FLY_LOS_MODE_PREVENT,
		current_future_sight, side_preference, &best_score, best_dir);
	found |= fly_consider_weighted_los_dir(ent, ent->enemy, target_velocity, side, 1.15f,
		to_target, 0.85f * 0.45f, down, 0.35f, vec3_origin, 0.0f, to_target, FLY_LOS_MODE_PREVENT,
		current_future_sight, side_preference * 0.75f, &best_score, best_dir);
	found |= fly_consider_weighted_los_dir(ent, ent->enemy, target_velocity, other_side, 1.15f,
		to_target, 0.85f, down, 0.0f, vec3_origin, 0.0f, to_target, FLY_LOS_MODE_PREVENT,
		current_future_sight, other_preference, &best_score, best_dir);
	found |= fly_consider_weighted_los_dir(ent, ent->enemy, target_velocity, other_side, 1.15f,
		to_target, 0.85f * 0.45f, down, 0.35f, vec3_origin, 0.0f, to_target, FLY_LOS_MODE_PREVENT,
		current_future_sight, other_preference * 0.75f, &best_score, best_dir);

	if (!found)
		return false;

	VectorCopy(best_dir, out_dir);
	return true;
}

static qboolean fly_try_recover_combat_sight(edict_t *ent, vec3_t target_origin, vec3_t target_velocity,
	vec3_t wanted_dir, vec3_t out_dir)
{
	const float probe_min = 96.0f;
	vec3_t to_target, target_dir, side, other_side, down, candidate, point, best_dir;
	trace_t tr;
	float target_dist, current_sight, side_preference, other_preference;
	float block_fraction, temp_dist, side_offset;
	qboolean found;

	if (!fly_recent_los_recovery_allowed(ent, ent->enemy))
		return false;

	current_sight = fly_combat_sight_fraction_from(ent, ent->s.origin, ent->enemy);
	if (current_sight >= FLY_LOS_FULL_SIGHT_FRACTION)
		return false;

	VectorSubtract(target_origin, ent->s.origin, to_target);
	target_dist = VectorNormalize(to_target);
	if (target_dist <= 0.1f)
		return false;

	VectorCopy(to_target, target_dir);
	target_dir[2] = 0.0f;
	if (VectorNormalize(target_dir) <= 0.1f)
	{
		VectorCopy(wanted_dir, target_dir);
		target_dir[2] = 0.0f;
		if (VectorNormalize(target_dir) <= 0.1f)
			return false;
	}

	if (!fly_los_side_vectors(ent, target_dir, target_velocity, side, other_side))
		return false;

	VectorSet(down, 0.0f, 0.0f, -1.0f);

	found = false;
	VectorClear(best_dir);
	side_preference = 4.0f + fly_entity_unit(ent, 0x92u);
	other_preference = fly_entity_unit(ent, 0x93u);
	current_sight = max(current_sight, 0.0f);
	{
		float best_score = -9999.0f;

		tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, target_origin, ent, MASK_SOLID | CONTENTS_MONSTERCLIP);
		if (!tr.startsolid && !tr.allsolid && tr.fraction < 1.0f)
		{
			block_fraction = fly_clampf(tr.fraction, 0.0f, 1.0f);
			temp_dist = target_dist * ((block_fraction + 1.0f) * 0.5f);
			temp_dist = fly_clampf(temp_dist, probe_min, target_dist);
			side_offset = max(max(fabsf(ent->maxs[0]), fabsf(ent->maxs[1])) + 24.0f, 32.0f);

			VectorMA(ent->s.origin, temp_dist, target_dir, point);
			VectorMA(point, side_offset, side, point);
			VectorSubtract(point, ent->s.origin, candidate);
			found |= fly_consider_los_dir(ent, ent->enemy, target_velocity, candidate, target_dir,
				FLY_LOS_MODE_RECOVER, current_sight, side_preference + 2.0f, &best_score, best_dir);

			VectorMA(ent->s.origin, temp_dist, target_dir, point);
			VectorMA(point, side_offset, other_side, point);
			VectorSubtract(point, ent->s.origin, candidate);
			found |= fly_consider_los_dir(ent, ent->enemy, target_velocity, candidate, target_dir,
				FLY_LOS_MODE_RECOVER, current_sight, other_preference + 2.0f, &best_score, best_dir);
		}

		found |= fly_consider_weighted_los_dir(ent, ent->enemy, target_velocity, side, 1.20f,
			target_dir, 0.80f, down, 0.0f, vec3_origin, 0.0f, target_dir, FLY_LOS_MODE_RECOVER,
			current_sight, side_preference, &best_score, best_dir);
		found |= fly_consider_weighted_los_dir(ent, ent->enemy, target_velocity, side, 1.20f,
			target_dir, 0.80f * 0.45f, down, 0.45f, vec3_origin, 0.0f, target_dir,
			FLY_LOS_MODE_RECOVER, current_sight, side_preference * 0.75f, &best_score, best_dir);
		found |= fly_consider_weighted_los_dir(ent, ent->enemy, target_velocity, other_side, 1.20f,
			target_dir, 0.80f, down, 0.0f, vec3_origin, 0.0f, target_dir, FLY_LOS_MODE_RECOVER,
			current_sight, other_preference, &best_score, best_dir);
		found |= fly_consider_weighted_los_dir(ent, ent->enemy, target_velocity, other_side, 1.20f,
			target_dir, 0.80f * 0.45f, down, 0.45f, vec3_origin, 0.0f, target_dir,
			FLY_LOS_MODE_RECOVER, current_sight, other_preference * 0.75f, &best_score, best_dir);
		found |= fly_consider_weighted_los_dir(ent, ent->enemy, target_velocity, vec3_origin, 0.0f,
			target_dir, 0.80f, down, 0.45f, vec3_origin, 0.0f, target_dir, FLY_LOS_MODE_RECOVER,
			current_sight, 0.0f, &best_score, best_dir);
	}

	if (!found)
		return false;

	VectorCopy(best_dir, out_dir);
	return true;
}

static qboolean fly_try_water_recovery(edict_t *ent, vec3_t wanted_dir)
{
	int i;
	vec3_t dir, end, test, angles, forward;
	trace_t tr;

	if (!(ent->flags & FL_FLY) || !ent->waterlevel)
		return false;

	ent->monsterinfo.fly_pinned = false;
	ent->monsterinfo.fly_position_time = 0.0f;

	VectorSet(dir, 0.0f, 0.0f, 1.0f);
	for (i = -1; i < 8; i++)
	{
		if (i >= 0)
		{
			if (i == 0 && VectorLength(wanted_dir) > 0.1f)
			{
				VectorCopy(wanted_dir, dir);
				dir[2] += 1.0f;
			}
			else
			{
				VectorSet(angles, 0.0f, ent->s.angles[YAW] + 45.0f * (float)i, 0.0f);
				AngleVectors(angles, forward, NULL, NULL);
				VectorCopy(forward, dir);
				dir[2] = 0.75f;
			}
			if (VectorNormalize(dir) <= 0.1f)
				VectorSet(dir, 0.0f, 0.0f, 1.0f);
		}

		VectorMA(ent->s.origin, 128.0f, dir, end);
		tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, end, ent, MASK_SOLID | CONTENTS_MONSTERCLIP);
		if (tr.allsolid || tr.startsolid)
			continue;

		VectorCopy(tr.endpos, test);
		test[2] += ent->mins[2] + 1.0f;
		if (!(gi.pointcontents(test) & MASK_WATER))
			break;
	}

	VectorCopy(dir, ent->monsterinfo.fly_recovery_dir);
	ent->monsterinfo.fly_recovery_time = level.time + 0.6f;
	VectorCopy(dir, wanted_dir);
	return true;
}

static void flystep_unpin(edict_t *ent)
{
	ent->monsterinfo.fly_position_time = 0.0f;
	ent->monsterinfo.fly_pinned = false;
}

static void flystep_refresh_hover(edict_t *ent, flystep_ctx_t *ctx)
{
	if (ent->monsterinfo.fly_position_time <= level.time ||
		(ent->enemy && ent->monsterinfo.fly_pinned && !visible(ent, ent->enemy)) ||
		(ent->enemy && VectorCompare(ent->monsterinfo.fly_ideal_position, vec3_origin)))
	{
		ent->monsterinfo.fly_pinned = false;
		ent->monsterinfo.fly_position_time = level.time + fly_frand_range(FLY_POSITION_UPDATE_MIN, FLY_POSITION_UPDATE_MAX);
		G_IdealHoverPosition(ent, ctx->intent.path_context, ent->monsterinfo.fly_ideal_position);
	}
}

static void flystep_read_velocity(edict_t *ent, flystep_ctx_t *ctx)
{
	VectorCopy(ent->velocity, ctx->dir);
	ctx->current_speed = VectorNormalize(ctx->dir);
	if (ctx->current_speed < 0.1f)
		VectorClear(ctx->dir);
}

static void flystep_decelerate_without_target(edict_t *ent, flystep_ctx_t *ctx)
{
	float accel = ent->monsterinfo.fly_acceleration;

	if (ctx->current_speed > 0.0f)
		ctx->current_speed = max(0.0f, ctx->current_speed - accel);

	if (ctx->current_speed > 0.0f)
		VectorScale(ctx->dir, ctx->current_speed, ent->velocity);
	else
		VectorClear(ent->velocity);
}

static qboolean flystep_pick_target(edict_t *ent, vec3_t dest, flystep_ctx_t *ctx)
{
	ctx->following_paths = ctx->intent.path_context;
	VectorClear(ctx->towards_origin);
	VectorClear(ctx->towards_velocity);

	if (ctx->intent.use_dest)
	{
		VectorCopy(dest, ctx->towards_origin);
		ctx->following_paths = true;
		return true;
	}

	if (ctx->following_paths)
	{
		if (ent->goalentity && ent->goalentity->inuse)
		{
			VectorCopy(ent->goalentity->s.origin, ctx->towards_origin);
			return true;
		}
		if (fly_has_last_sighting(ent))
		{
			VectorCopy(ent->monsterinfo.last_sighting, ctx->towards_origin);
			return true;
		}
	}
	else if (!ctx->intent.visible_combat_enemy && ctx->intent.recent_combat_memory && fly_has_last_sighting(ent))
	{
		VectorCopy(ent->monsterinfo.last_sighting, ctx->towards_origin);
		ctx->following_paths = true;
		return true;
	}
	else if (ent->enemy && ent->enemy->inuse)
	{
		VectorCopy(ent->enemy->s.origin, ctx->towards_origin);
		VectorCopy(ent->enemy->velocity, ctx->towards_velocity);
		return true;
	}
	else if (ent->goalentity && ent->goalentity->inuse && ent->goalentity != world)
	{
		VectorCopy(ent->goalentity->s.origin, ctx->towards_origin);
		return true;
	}

	return false;
}

static void flystep_build_wanted_position(edict_t *ent, flystep_ctx_t *ctx)
{
	vec3_t box_mins, box_maxs, dest_diff;
	trace_t tr;

	if (ent->monsterinfo.fly_pinned)
		VectorCopy(ent->monsterinfo.fly_ideal_position, ctx->wanted_pos);
	else if (ctx->following_paths)
		VectorCopy(ctx->towards_origin, ctx->wanted_pos);
	else
	{
		VectorMA(ctx->towards_origin, FLY_TARGET_LEAD_TIME, ctx->towards_velocity, ctx->wanted_pos);
		VectorAdd(ctx->wanted_pos, ent->monsterinfo.fly_ideal_position, ctx->wanted_pos);
	}

	if (!ctx->following_paths)
		ctx->catchup_goal = fly_adjust_visible_combat_goal(ent, ctx->towards_origin, ctx->towards_velocity,
			ctx->wanted_pos, ctx->intent.visible_combat_enemy);

	if (ctx->intent.visible_combat_enemy && !ctx->following_paths)
		fly_apply_neighbor_separation(ent, ctx->wanted_pos);

	VectorSet(box_mins, -8.0f, -8.0f, -8.0f);
	VectorSet(box_maxs, 8.0f, 8.0f, 8.0f);
	tr = gi.trace(ctx->towards_origin, box_mins, box_maxs, ctx->wanted_pos, ent, MASK_SOLID | CONTENTS_MONSTERCLIP);
	if (!tr.allsolid)
		VectorCopy(tr.endpos, ctx->wanted_pos);

	fly_clamp_to_ceiling(ent, ctx->wanted_pos);

	VectorSubtract(ctx->wanted_pos, ent->s.origin, dest_diff);
	if (dest_diff[2] > ent->mins[2] && dest_diff[2] < ent->maxs[2])
		dest_diff[2] = 0.0f;

	VectorCopy(dest_diff, ctx->wanted_dir);
	ctx->dist_to_wanted = VectorNormalize(ctx->wanted_dir);
}

static void flystep_apply_goal_pressure(edict_t *ent, flystep_ctx_t *ctx)
{
	if (ctx->dist_to_wanted > 0.1f &&
		fly_should_descend_for_lower_path(ent, ctx->following_paths, ctx->towards_origin, ctx->wanted_pos))
		fly_force_descent(ent, ctx->wanted_dir, FLY_BLOCKED_DESCENT_XY_SCALE);

	if (ctx->dist_to_wanted > 0.1f &&
		(ctx->intent.visible_combat_enemy || ctx->following_paths || ctx->intent.recent_combat_memory))
	{
		vec3_t climb_target;

		if (ctx->intent.visible_combat_enemy && ent->enemy && ent->enemy->inuse)
			VectorCopy(ent->enemy->s.origin, climb_target);
		else
			VectorCopy(ctx->towards_origin, climb_target);

		if (fly_try_climb_for_higher_target(ent, climb_target, ctx->wanted_dir))
			ctx->catchup_goal = true;
	}

	if (ctx->intent.visible_combat_enemy && ctx->dist_to_wanted > 0.1f &&
		ent->enemy && ent->enemy->inuse &&
		fly_try_prevent_combat_sight_loss(ent, ent->enemy->s.origin, ent->enemy->velocity,
			ctx->wanted_dir, ctx->wanted_dir))
	{
		ctx->los_preserve_drive = true;
		flystep_unpin(ent);
	}
	else if (!ctx->intent.visible_combat_enemy && ctx->intent.recent_combat_memory && ctx->dist_to_wanted > 0.1f &&
		ent->enemy && ent->enemy->inuse &&
		fly_try_recover_combat_sight(ent, ent->enemy->s.origin, ent->enemy->velocity,
			ctx->wanted_dir, ctx->wanted_dir))
	{
		ctx->los_recover_drive = true;
		flystep_unpin(ent);
	}
}

static void flystep_face_move_target(edict_t *ent, flystep_ctx_t *ctx)
{
	if (ctx->intent.visible_combat_enemy ||
		(ctx->los_recover_drive && ent->enemy && ent->enemy->inuse))
		fly_face_target(ent, ent->enemy->s.origin);
}

static void flystep_avoid_blocked_move(edict_t *ent, flystep_ctx_t *ctx)
{
	vec3_t trace_end, aim_fwd, aim_rgt, aim_up, yaw_angles;
	trace_t tr;

	if (ctx->dist_to_wanted > 0.1f)
	{
		VectorMA(ent->s.origin, ent->monsterinfo.fly_acceleration, ctx->wanted_dir, trace_end);
		tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, trace_end, ent, MASK_SOLID | CONTENTS_MONSTERCLIP);
	}
	else
	{
		tr.fraction = 1.0f;
	}

	VectorSet(yaw_angles, 0.0f, ent->s.angles[YAW], 0.0f);
	AngleVectors(yaw_angles, aim_fwd, aim_rgt, aim_up);

	if (tr.fraction < 0.25f && ctx->dist_to_wanted > 0.1f)
	{
		vec3_t bottom_pos, top_pos, startb, floor_check, side_offset, lateral_offset, left_start, right_start, obstacle_normal;
		qboolean bottom_visible, top_visible, left_visible, right_visible, force_descent;

		if (ent->monsterinfo.fly_wall_stuck_time == 0.0f)
			ent->monsterinfo.fly_wall_stuck_time = level.time;

		VectorCopy(tr.plane.normal, obstacle_normal);

		VectorCopy(ent->s.origin, bottom_pos);
		bottom_pos[2] += ent->mins[2];
		VectorCopy(ent->s.origin, top_pos);
		top_pos[2] += ent->maxs[2];

		VectorCopy(ent->s.origin, startb);
		startb[2] += ent->mins[2] - ent->monsterinfo.fly_acceleration;
		bottom_visible = SV_flystep_testvisposition(bottom_pos, ctx->wanted_pos, ent->s.origin, startb, ent);

		VectorCopy(top_pos, startb);
		startb[2] += ent->monsterinfo.fly_acceleration;
		top_visible = SV_flystep_testvisposition(top_pos, ctx->wanted_pos, ent->s.origin, startb, ent);

		force_descent = false;
		if (ctx->intent.visible_combat_enemy &&
			fly_try_preserve_combat_sight(ent, ctx->towards_origin, ctx->towards_velocity,
				ctx->wanted_dir, obstacle_normal, ctx->wanted_dir))
		{
			ctx->los_preserve_drive = true;
			flystep_unpin(ent);
		}
		else if ((ctx->following_paths || ctx->intent.visible_combat_enemy) &&
			ent->s.origin[2] > ctx->towards_origin[2] + FLY_BLOCKED_DESCENT_HEIGHT &&
			fly_force_descent(ent, ctx->wanted_dir, FLY_BLOCKED_DESCENT_XY_SCALE))
		{
			force_descent = true;
		}
		else if (level.time > ent->monsterinfo.fly_wall_stuck_time +
			(ctx->intent.visible_combat_enemy ? FLY_COMBAT_WALL_STUCK_THRESHOLD : FLY_WALL_STUCK_THRESHOLD))
		{
			VectorCopy(ent->s.origin, floor_check);
			floor_check[2] -= 512.0f;
			tr = gi.trace(ent->s.origin, ent->mins, ent->maxs, floor_check, ent, MASK_SOLID | CONTENTS_MONSTERCLIP);
			if (tr.fraction < 1.0f && fly_force_descent(ent, ctx->wanted_dir, 0.3f))
			{
				force_descent = true;
			}
		}

		if (!ctx->los_preserve_drive && !force_descent)
		{
			if (bottom_visible == top_visible)
			{
				side_offset[0] = aim_fwd[0] * ent->maxs[0];
				side_offset[1] = aim_fwd[1] * ent->maxs[1];
				side_offset[2] = aim_fwd[2] * ent->maxs[2];
				lateral_offset[0] = aim_rgt[0] * ent->maxs[0];
				lateral_offset[1] = aim_rgt[1] * ent->maxs[1];
				lateral_offset[2] = aim_rgt[2] * ent->maxs[2];

				VectorAdd(ent->s.origin, side_offset, left_start);
				VectorSubtract(left_start, lateral_offset, left_start);
				VectorAdd(ent->s.origin, side_offset, right_start);
				VectorAdd(right_start, lateral_offset, right_start);

				left_visible = gi.trace(left_start, NULL, NULL, ctx->wanted_pos, ent, MASK_SOLID | CONTENTS_MONSTERCLIP).fraction == 1.0f;
				right_visible = gi.trace(right_start, NULL, NULL, ctx->wanted_pos, ent, MASK_SOLID | CONTENTS_MONSTERCLIP).fraction == 1.0f;

				if (left_visible != right_visible)
				{
					if (right_visible)
						VectorAdd(ctx->wanted_dir, aim_rgt, ctx->wanted_dir);
					else
						VectorSubtract(ctx->wanted_dir, aim_rgt, ctx->wanted_dir);
				}
				else
				{
					VectorCopy(obstacle_normal, ctx->wanted_dir);
					ctx->wanted_dir[2] -= 0.2f;
				}
			}
			else
			{
				if (top_visible)
					VectorAdd(ctx->wanted_dir, aim_up, ctx->wanted_dir);
				else
					VectorSubtract(ctx->wanted_dir, aim_up, ctx->wanted_dir);
			}
			VectorNormalize(ctx->wanted_dir);
		}
	}
	else
	{
		ent->monsterinfo.fly_wall_stuck_time = 0.0f;
	}

}

static void flystep_resolve_medium(edict_t *ent, flystep_ctx_t *ctx)
{
	ctx->water_recovery_drive = fly_try_water_recovery(ent, ctx->wanted_dir);
	ctx->bad_movement_direction = false;
	if (!ctx->water_recovery_drive && ctx->dist_to_wanted > 0.1f)
	{
		vec3_t contents_test;
		VectorMA(ent->s.origin, max(ctx->current_speed, ent->monsterinfo.fly_speed) * FRAMETIME, ctx->wanted_dir, contents_test);
		if ((ent->flags & FL_FLY) && ent->waterlevel < 3)
			ctx->bad_movement_direction = (gi.pointcontents(contents_test) & MASK_WATER) != 0;
		else if (ent->flags & FL_SWIM)
			ctx->bad_movement_direction = (gi.pointcontents(contents_test) & MASK_WATER) == 0;
	}

	if (ctx->bad_movement_direction)
	{
		if (ent->monsterinfo.fly_recovery_time < level.time)
		{
			VectorSet(ent->monsterinfo.fly_recovery_dir, crandom(), crandom(), crandom());
			if (VectorNormalize(ent->monsterinfo.fly_recovery_dir) < 0.1f)
				VectorSet(ent->monsterinfo.fly_recovery_dir, 0.0f, 0.0f, 1.0f);
			ent->monsterinfo.fly_recovery_time = level.time + 1.0f;
		}
		VectorCopy(ent->monsterinfo.fly_recovery_dir, ctx->wanted_dir);
	}
}

static void flystep_blend_direction(edict_t *ent, flystep_ctx_t *ctx)
{
	qboolean los_pressure_drive = ctx->los_preserve_drive || ctx->los_recover_drive;
	float turn_factor;

	if (ctx->dir[0] || ctx->dir[1] || ctx->dir[2])
	{
		float dir_dot = DotProduct(ctx->dir, ctx->wanted_dir);

		if (los_pressure_drive && dir_dot < FLY_LOS_PRESSURE_SNAP_DOT)
		{
			VectorCopy(ctx->wanted_dir, ctx->final_dir);
		}
		else if (los_pressure_drive)
		{
			turn_factor = FLY_LOS_PRESERVE_TURN_FACTOR;
			fly_slerp(ctx->final_dir, ctx->dir, ctx->wanted_dir, 1.0f - turn_factor);
			if (VectorNormalize(ctx->final_dir) < 0.1f)
				VectorCopy(ctx->wanted_dir, ctx->final_dir);
		}
		else if (ctx->catchup_goal && dir_dot < 0.0f)
		{
			VectorCopy(ctx->wanted_dir, ctx->final_dir);
		}
		else if (ctx->catchup_goal && dir_dot < 0.7f)
		{
			turn_factor = 0.2f;
			fly_slerp(ctx->final_dir, ctx->dir, ctx->wanted_dir, 1.0f - turn_factor);
			if (VectorNormalize(ctx->final_dir) < 0.1f)
				VectorCopy(ctx->wanted_dir, ctx->final_dir);
		}
		else if (((ent->monsterinfo.fly_thrusters && !ent->monsterinfo.fly_pinned) ||
			ctx->following_paths || ctx->water_recovery_drive) && DotProduct(ctx->dir, ctx->wanted_dir) > 0.0f)
		{
			turn_factor = FLY_TURN_FACTOR_FAST;
			fly_slerp(ctx->final_dir, ctx->dir, ctx->wanted_dir, 1.0f - turn_factor);
			if (VectorNormalize(ctx->final_dir) < 0.1f)
				VectorCopy(ctx->wanted_dir, ctx->final_dir);
		}
		else
		{
			turn_factor = min(1.0f, FLY_TURN_FACTOR_BASE +
				(FLY_TURN_FACTOR_SPEED_SCALE * (ctx->current_speed / ent->monsterinfo.fly_speed)));
			fly_slerp(ctx->final_dir, ctx->dir, ctx->wanted_dir, 1.0f - turn_factor);
			if (VectorNormalize(ctx->final_dir) < 0.1f)
				VectorCopy(ctx->wanted_dir, ctx->final_dir);
		}
	}
	else
	{
		VectorCopy(ctx->wanted_dir, ctx->final_dir);
	}
}

static qboolean flystep_commit_velocity(edict_t *ent, flystep_ctx_t *ctx)
{
	float base_fly_speed = ent->monsterinfo.fly_speed;
	float accel = ent->monsterinfo.fly_acceleration;
	float speed_factor, wanted_speed;
	qboolean los_pressure_drive = ctx->los_preserve_drive || ctx->los_recover_drive;
	qboolean combat_attack_drive = ctx->intent.visible_combat_enemy &&
		(ent->monsterinfo.attack_state == AS_STRAIGHT || ent->monsterinfo.attack_state == AS_SLIDING);

	if (combat_attack_drive)
	{
		base_fly_speed *= FLY_ATTACK_SPEED_SCALE;
		accel *= FLY_ATTACK_ACCEL_SCALE;
	}
	if (los_pressure_drive)
	{
		base_fly_speed *= FLY_LOS_PRESERVE_SPEED_SCALE;
		accel *= FLY_LOS_PRESERVE_ACCEL_SCALE;
	}

	if (!ent->enemy || ctx->water_recovery_drive ||
		(ent->monsterinfo.fly_thrusters && !ent->monsterinfo.fly_pinned) ||
		ctx->following_paths || ctx->catchup_goal)
	{
		if (ctx->following_paths && (ctx->dir[0] || ctx->dir[1] || ctx->dir[2]) &&
			DotProduct(ctx->wanted_dir, ctx->dir) < -0.25f)
			speed_factor = 0.0f;
		else
			speed_factor = 1.0f;
	}
	else
	{
		speed_factor = min(1.0f, ctx->dist_to_wanted / base_fly_speed);
	}

	if (ctx->bad_movement_direction)
		speed_factor = -speed_factor;

	if (DotProduct(ctx->final_dir, ctx->wanted_dir) < 0.25f)
		accel *= 2.0f;

	wanted_speed = base_fly_speed * speed_factor;
	if (ctx->current_speed > wanted_speed)
		ctx->current_speed = max(wanted_speed, ctx->current_speed - accel);
	else if (ctx->current_speed < wanted_speed)
		ctx->current_speed = min(wanted_speed, ctx->current_speed + accel);

	if (!fly_vector_valid(ctx->final_dir) || !isfinite(ctx->current_speed))
		return false;

	VectorScale(ctx->final_dir, ctx->current_speed, ent->velocity);

	if (!ctx->intent.visible_combat_enemy && ent->velocity[2] < 0.0f)
	{
		float max_descent = ent->monsterinfo.fly_speed * FLY_SEARCH_DESCENT_SPEED_SCALE;

		if (ent->velocity[2] < -max_descent)
			ent->velocity[2] = -max_descent;
	}

	return true;
}

static void flystep_update_pitch(edict_t *ent, flystep_ctx_t *ctx)
{
	if (ent->enemy && (ent->monsterinfo.fly_buzzard || (ent->monsterinfo.aiflags & AI_MEDIC)))
	{
		vec3_t pitch_dir, pitch_angles, pitch_target;
		if (ctx->intent.visible_combat_enemy)
			VectorCopy(ent->enemy->s.origin, pitch_target);
		else
			VectorCopy(ctx->towards_origin, pitch_target);

		VectorSubtract(ent->s.origin, pitch_target, pitch_dir);
		if (VectorNormalize(pitch_dir) > 0.1f)
		{
			vectoangles(pitch_dir, pitch_angles);
			ent->s.angles[PITCH] = LerpAngle(ent->s.angles[PITCH], -pitch_angles[PITCH], FRAMETIME * FLY_PITCH_LERP_SPEED);
		}
	}
	else
	{
		ent->s.angles[PITCH] = 0.0f;
	}
}

qboolean SV_alternate_flystep(edict_t* ent, vec3_t dest, vec3_t move, qboolean relink)
{
	flystep_ctx_t ctx = { 0 };

	(void)move;
	(void)relink;

	if (!fly_get_alternate_intent(ent, dest, &ctx.intent))
		return false;

	if ((ent->flags & FL_SWIM) && ent->waterlevel < WATER_WAIST)
		return true;

	if (ent->monsterinfo.fly_speed <= 0.0f || ent->monsterinfo.fly_acceleration <= 0.0f)
		return false;

	if (ent->monsterinfo.fly_max_distance < ent->monsterinfo.fly_min_distance)
		ent->monsterinfo.fly_max_distance = ent->monsterinfo.fly_min_distance;

	flystep_refresh_hover(ent, &ctx);
	flystep_read_velocity(ent, &ctx);

	if (!flystep_pick_target(ent, dest, &ctx))
	{
		flystep_decelerate_without_target(ent, &ctx);
		return true;
	}

	flystep_build_wanted_position(ent, &ctx);
	flystep_apply_goal_pressure(ent, &ctx);
	flystep_face_move_target(ent, &ctx);
	flystep_avoid_blocked_move(ent, &ctx);
	flystep_resolve_medium(ent, &ctx);
	flystep_blend_direction(ent, &ctx);

	if (!flystep_commit_velocity(ent, &ctx))
		return false;

	flystep_update_pitch(ent, &ctx);

	return true;
}

qboolean M_FlyMove(edict_t* ent, vec3_t dest, vec3_t move, qboolean relink)
{
	//float        dz;                        // delta Z
	//float        idealZ;                // ideal Z position, either a destination waypoint/node or a goal entity
	float        zSpeed = 8;        // Z movement speed
	trace_t        trace;
	vec3_t        neworg1, neworg2;

	if (ent->monsterinfo.aiflags & AI_ALTERNATE_FLY)
	{
		if (SV_alternate_flystep(ent, dest, move, relink))
			return true;

		fly_reset_alternate_step(ent);
	}

	if (!M_MoveVertical(ent, dest, neworg1))
	{
		//gi.dprintf("movevertical failed\n");
		return false;
	}
	VectorAdd(neworg1, move, neworg2);
	trace = gi.trace(neworg1, ent->mins, ent->maxs, neworg2, ent, MASK_MONSTERSOLID);
	if (trace.fraction == 1)
	{
		// has our Z position changed?
		if (fabs(ent->s.origin[2] - neworg2[2]) > 1)
		{
			ent->monsterinfo.Zchanged = true;
			//gi.dprintf("Z changed\n");
		}
		else
		{
			// if our Z position changed last frame, then delay the next Z position change
			if (ent->monsterinfo.Zchanged)
			{
				//gi.dprintf("** Z position change will be delayed **\n");
				ent->monsterinfo.Zchange_delay = level.time + GetRandom(3, 9) * FRAMETIME; // don't bounce!
			}
			// our Z position hasn't changed recently
			ent->monsterinfo.Zchanged = false;
		}
		VectorCopy(trace.endpos, ent->s.origin);
		if (relink)
		{
			gi.linkentity(ent);
			G_TouchTriggers(ent);
		}

		//gi.dprintf("flymove was able to move %.0f\n", distance(neworg1, neworg2));
		return true;
	}
	//return M_MoveVertical(ent, dest, relink); //false;
	return false;
}

/*
=============
SV_movestep

Called by monster program code.
The move will be adjusted for slopes and stairs, but if the move isn't
possible, no move is done, false is returned, and
pr_global_struct->trace_normal is set to the normal of the blocking wall
=============
*/
//FIXME since we need to test end position contents here, can we avoid doing
//it again later in catagorize position?
qboolean SV_movestep(edict_t* ent, vec3_t dest, vec3_t move, qboolean relink)
{
	float		dz;
	vec3_t		oldorg, neworg, end;
	trace_t		trace;//, tr;
	int			i;
	float		stepsize;
	vec3_t		test;
	int			contents;
	int			jump=0;

// try the move	
	VectorCopy (ent->s.origin, oldorg);
	VectorAdd (ent->s.origin, move, neworg);

// flying monsters don't step up
	if ((ent->flags & (FL_SWIM|FL_FLY)) || (ent->waterlevel > 1))
	{
		if (ent->flags & FL_FLY)
			return M_FlyMove(ent, dest, move, relink);

		if (ent->monsterinfo.aiflags & AI_ALTERNATE_FLY)
		{
			if (SV_alternate_flystep(ent, dest, move, relink))
				return true;

			fly_reset_alternate_step(ent);
		}

		//	gi.dprintf("trying to swim\n");
		// try one move with vertical motion, then one without
		for (i = 0; i < 2; i++)
		{
			VectorAdd(ent->s.origin, move, neworg);
			if (i == 0 && ent->enemy)
			{
				if (!ent->goalentity)
					ent->goalentity = ent->enemy;
				if (dest)
				{
					dz = ent->s.origin[2] - dest[2];
					//gi.dprintf("adjusting z height to dest z\n");
				}
				else
					dz = ent->s.origin[2] - ent->goalentity->s.origin[2];
				if (ent->goalentity->client)
				{
					if (dz > 40)
						neworg[2] -= 8;
					if (!((ent->flags & FL_SWIM) && (ent->waterlevel < 2)))
						if (dz < 30)
							neworg[2] += 8;
				}
				else
				{
					if (dz > 8)
						neworg[2] -= 8;
					else if (dz > 0)
						neworg[2] -= dz;
					else if (dz < -8)
						neworg[2] += 8;
					else
						neworg[2] += dz;
				}
			}
			trace = gi.trace (ent->s.origin, ent->mins, ent->maxs, neworg, ent, MASK_MONSTERSOLID);
	
			// fly monsters don't enter water voluntarily
			if (ent->flags & FL_FLY)
			{
				if (!ent->waterlevel)
				{
					test[0] = trace.endpos[0];
					test[1] = trace.endpos[1];
					test[2] = trace.endpos[2] + ent->mins[2] + 1;
					contents = gi.pointcontents(test);
					if (contents & MASK_WATER)
						return false;
				}
			}

			// swim monsters don't exit water voluntarily
			if (ent->flags & FL_SWIM)
			{
				if (ent->waterlevel < 2)
				{
					test[0] = trace.endpos[0];
					test[1] = trace.endpos[1];
					test[2] = trace.endpos[2] + ent->mins[2] + 1;
					contents = gi.pointcontents(test);
					if (!(contents & MASK_WATER))
						return false;
				}
			}

			if (trace.fraction == 1)
			{
				VectorCopy (trace.endpos, ent->s.origin);
				if (relink)
				{
					gi.linkentity (ent);
					G_TouchTriggers (ent);
				}
				return true;
			}
			//gi.dprintf("swim move failed\n");
			
			if (!ent->enemy)
				break;
		}
		
		return false;
	}

// push down from a step height above the wished position
	if (!(ent->monsterinfo.aiflags & AI_NOSTEP))
		stepsize = STEPSIZE;
	else
		stepsize = 1;

	neworg[2] += stepsize;
	VectorCopy (neworg, end);
	end[2] -= stepsize*2;
 
	// this trace checks from a position one step above the entity (at top of bbox)
	// to one step below the entity (bottom of bbox)
	trace = gi.trace (neworg, ent->mins, ent->maxs, end, ent, MASK_MONSTERSOLID);

	// there is an obstruction bigger than a step
	if (trace.allsolid)
//GHz START
	{
		// try to jump over it
		if (G_EntIsAlive(trace.ent) || !CanJumpUp(ent, neworg, end))
		{
			//gi.dprintf("couldn't jump over obstruction\n");
			return false;
		}
		else
		{
			jump = 1;
		}
	}
//GHz END

	// not enough room at this height--head of bbox intersects something solid
	// so push down and just try to walk forward at floor height
	else if (trace.startsolid)
	{
		neworg[2] -= stepsize;
		trace = gi.trace (neworg, ent->mins, ent->maxs, end, ent, MASK_MONSTERSOLID);
		if (trace.allsolid || trace.startsolid)
		{
			//gi.dprintf("too tall!\n");
			return false;
		}
	}

	// don't go in to water
	if (ent->waterlevel == 0)
	{
		test[0] = trace.endpos[0];
		test[1] = trace.endpos[1];
		test[2] = trace.endpos[2] + ent->mins[2] + 1;	
		contents = gi.pointcontents(test);

		if (contents & (CONTENTS_LAVA|CONTENTS_SLIME))
			return false;
	}

//GHz 5/8/2010 - don't get stuck on steep little ramps
	if (trace.fraction < 1 && trace.plane.normal[2] < 0.7) // too steep
	{
		//gi.dprintf("too steep\n");
		return false;
	}

//GHz START
//	if (CanJumpDown(ent, trace.endpos))
//		jump = -1;
//GHz END

//	VectorSubtract(trace.endpos, oldorg, forward);
//	VectorNormalize(forward);
//	VectorMA(oldorg, 32, forward, end);

//	VectorAdd(trace.endpos, move, end);
//	VectorAdd(end, move, end);

	if ((trace.fraction == 1) && (jump != 1))
	{
		//gi.dprintf("going to fall\n");
	// if monster had the ground pulled out, go ahead and fall
	//	VectorSubtract(trace.endpos, oldorg, forward);
	//	VectorMA(oldorg, 64, forward, end);
		if (!CanJumpDown(ent, trace.endpos))
		{
			if ( ent->flags & FL_PARTIALGROUND )
			{
				VectorAdd (ent->s.origin, move, ent->s.origin);
				if (relink)
				{
					gi.linkentity (ent);
					G_TouchTriggers (ent);
				}
				ent->groundentity = NULL;
				return true;
			}
			//gi.dprintf("don't walk off a ledge\n");
			return false;		// walked off an edge
		}
		else
			jump = -1;
	}

// check point traces down for dangling corners
	//GHz START
	/*
	// fix for monsters walking thru walls
	tr = gi.trace(trace.endpos, ent->mins, ent->maxs, trace.endpos, ent, MASK_SOLID);
	if (tr.contents & MASK_SOLID)
		return false;
	*/
	//GHz END
	
	if (jump != 1)
		VectorCopy (trace.endpos, ent->s.origin);
	
	if (!M_CheckBottom (ent))
	{
		//gi.dprintf("partial ground\n");
		if (ent->flags & FL_PARTIALGROUND)
		{	// entity had floor mostly pulled out from underneath it
			// and is trying to correct
			if (relink)
			{
				gi.linkentity (ent);
				G_TouchTriggers (ent);
			}
			return true;
		}
		if (CanJumpDown(ent, trace.endpos))
			jump = -1;
		if (!jump)
		{
			VectorCopy (oldorg, ent->s.origin);
			//gi.dprintf("don't fall\n");
			return false;
		}
	}
	else if (jump == -1)
		jump = 0;

	
	if ( ent->flags & FL_PARTIALGROUND )
	{
		ent->flags &= ~FL_PARTIALGROUND;
	}

	ent->groundentity = trace.ent;
	if (trace.ent)
		ent->groundentity_linkcount = trace.ent->linkcount;

	if (jump == -1)
	{
		/*
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_DEBUGTRAIL);
		gi.WritePosition (oldorg);
		gi.WritePosition (end);
		gi.multicast (end, MULTICAST_ALL);
		*/

		//VectorScale(move, 10, ent->velocity);
		// ent->velocity[2] = 200;
	}
	else if (jump == 1)
	{
		ent->velocity[2] = 200;
		//gi.dprintf("jumped at %d\n",level.framenum);
	}

// the move is ok
	if (relink)
	{
		gi.linkentity (ent);
		G_TouchTriggers (ent);
	}
	//gi.dprintf("moved successfully at %d\n", level.framenum);
	return true;
}

/*
======================
SV_StepDirection

Turns to the movement direction, and walks the current distance if
facing it.

======================
*/
qboolean SV_StepDirection(edict_t* ent, vec3_t dest, float yaw, float dist, qboolean try_smallstep)
{
	vec3_t		move, oldorigin;
	//float		delta;
	//float		old_dist;
	
	// dist = dist);

	//gi.dprintf("SV_StepDirection\n");
	//old_dist = dist;
	ent->ideal_yaw = yaw;
	M_ChangeYaw (ent);

	if (!dist || fabs(dist) < 1)
		return true;

	yaw = yaw*M_PI*2 / 360;
	VectorCopy (ent->s.origin, oldorigin);

	// loop until we can move successfully
	while ((int)dist != 0)
	{
		move[0] = cos(yaw)*dist;
		move[1] = sin(yaw)*dist;
		move[2] = 0;

		
		if (SV_movestep (ent, dest, move, false))
		{
			/*
			delta = ent->s.angles[YAW] - ent->ideal_yaw;
			if (delta > 45 && delta < 315)
			{		// not turned far enough, so don't take the step
				VectorCopy (oldorigin, ent->s.origin);
				gi.dprintf("not turned far enough!\n");
			}
			*/
			gi.linkentity (ent);
			G_TouchTriggers (ent);
			/*
			if (old_dist != dist)
				gi.dprintf("took a smaller step of %d instead of %d\n", (int)dist, (int)old_dist);
			else
				gi.dprintf("moved full distance\n");
			*/
			return true;
		}

		if (!try_smallstep)
			break;
		if (dist > 0)
			dist--;
		else
			dist++;
	}

	gi.linkentity (ent);
	G_TouchTriggers (ent);
	//gi.dprintf("%d: %s can't walk forward!\n", level.framenum, V_GetMonsterName(ent));
	return false;
}

/*
======================
SV_FixCheckBottom

======================
*/
void SV_FixCheckBottom (edict_t *ent)
{
	ent->flags |= FL_PARTIALGROUND;
}


/*
================
SV_NewChaseDir

================
*/
#define	DI_NODIR	-1
void SV_NewChaseDir (edict_t *actor, edict_t *enemy, float dist)
{
	float	deltax,deltay;
	float	d[3];
	float	tdir, olddir, turnaround;

	//FIXME: how did we get here with no enemy
	if (!enemy)
		return;

	olddir = anglemod( (int)(actor->ideal_yaw/45)*45 );
	turnaround = anglemod(olddir - 180);

	deltax = enemy->s.origin[0] - actor->s.origin[0];
	deltay = enemy->s.origin[1] - actor->s.origin[1];
	if (deltax>10)
		d[1]= 0;
	else if (deltax<-10)
		d[1]= 180;
	else
		d[1]= DI_NODIR;
	if (deltay<-10)
		d[2]= 270;
	else if (deltay>10)
		d[2]= 90;
	else
		d[2]= DI_NODIR;

// try direct route
	if (d[1] != DI_NODIR && d[2] != DI_NODIR)
	{
		if (d[1] == 0)
			tdir = d[2] == 90 ? 45 : 315;
		else
			tdir = d[2] == 90 ? 135 : 215;
			
		if (tdir != turnaround && SV_StepDirection(actor, NULL, tdir, dist, true))
			return;
	}

// try other directions
	if ( ((randomMT()&3) & 1) ||  fabsf(deltay)>fabsf(deltax))
	{
		tdir=d[1];
		d[1]=d[2];
		d[2]=tdir;
	}

	if (d[1]!=DI_NODIR && d[1]!=turnaround 
	&& SV_StepDirection(actor, NULL, d[1], dist, true))
			return;

	if (d[2]!=DI_NODIR && d[2]!=turnaround
	&& SV_StepDirection(actor, NULL, d[2], dist, true))
			return;

/* there is no direct path to the player, so pick another direction */

	if (olddir!=DI_NODIR && SV_StepDirection(actor, NULL, olddir, dist, true))
			return;

	if (randomMT()&1) 	/*randomly determine direction of search*/
	{
		for (tdir=0 ; tdir<=315 ; tdir += 45)
			if (tdir!=turnaround && SV_StepDirection(actor, NULL, tdir, dist, true) )
					return;
	}
	else
	{
		for (tdir=315 ; tdir >=0 ; tdir -= 45)
			if (tdir!=turnaround && SV_StepDirection(actor, NULL, tdir, dist, true) )
					return;
	}

	if (turnaround != DI_NODIR && SV_StepDirection(actor, NULL, turnaround, dist, true) )
			return;

	actor->ideal_yaw = olddir;		// can't move

// if a bridge was pulled out from underneath a monster, it may not have
// a valid standing position at all

	if (!M_CheckBottom (actor))
		SV_FixCheckBottom (actor);
}

void SV_NewChaseDir1 (edict_t *self, edict_t *goal, float dist)
{
	int		i, bestyaw, minyaw, maxyaw;	
	vec3_t	v;

	if (!goal)
		return;

	VectorSubtract(goal->s.origin, self->s.origin, v);
	bestyaw = vectoyaw(v);

	minyaw = bestyaw - 90;
	maxyaw = bestyaw + 90;

	if (minyaw < 0)
		minyaw += 360;
	if (maxyaw > 360)
		maxyaw -= 360;

	for (i=minyaw; i<maxyaw; i+=30) {
		if (SV_StepDirection(self, NULL, i, dist, false))
			return;
	}

	//gi.dprintf("couldnt find a better direction!\n");
	SV_NewChaseDir(self, goal, dist);
}

qboolean CheckYawStep (edict_t *self, float minyaw, float maxyaw, float dist)
{
	int		i, max;
	float	yaw;

	AngleCheck(&minyaw);
	AngleCheck(&maxyaw);

	// calculate the maximum yaw variance
	max = 360 - fabs(minyaw - maxyaw);

	// we will start at the minimum yaw angle and move towards maxyaw
	yaw = minyaw;

	for (i = 0; i < max; i += 30) 
	{
		// if we changed course a while ago, then try a partial step 50% of the time
		if (level.time - self->monsterinfo.bump_delay > 1.0 || random() < 0.5)
		{
			yaw += i;
			AngleCheck(&yaw);

			if (SV_StepDirection(self, NULL, yaw, dist, true))
			{
				//gi.dprintf("attempt small step\n");
				return true;
			}
		}
		// otherwise, we might be stuck, so try a full step
		else
		{
			if (SV_StepDirection(self, NULL, i, dist, false))
			{
				//gi.dprintf("attempt full step\n");
				return true;
			}
		}
	}

	return false;
}

void SV_NewChaseDir3(edict_t* actor, vec3_t goalpos, float dist)
{
	float	deltax, deltay;
	float	d[3];
	float	tdir, olddir, turnaround;

	//FIXME: how did we get here with no enemy
	if (!goalpos)
		return;

	olddir = anglemod((int)(actor->ideal_yaw / 45) * 45);
	turnaround = anglemod(olddir - 180);
	//gi.dprintf("olddir=%.0f, turnaround=%.0f\n", olddir, turnaround);

	deltax = goalpos[0] - actor->s.origin[0];
	deltay = goalpos[1] - actor->s.origin[1];
	if (deltax > 10)
		d[1] = 0;
	else if (deltax < -10)
		d[1] = 180;
	else
		d[1] = DI_NODIR;
	if (deltay < -10)
		d[2] = 270;
	else if (deltay > 10)
		d[2] = 90;
	else
		d[2] = DI_NODIR;

	// try direct route
	if (d[1] != DI_NODIR && d[2] != DI_NODIR)
	{
		if (d[1] == 0)
			tdir = d[2] == 90 ? 45 : 315;
		else
			tdir = d[2] == 90 ? 135 : 215;

		if (tdir != turnaround && SV_StepDirection(actor, goalpos, tdir, dist, true))
			return;
	}

	// try other directions
	if (((rand() & 3) & 1) || fabsf(deltay) > fabsf(deltax))
	{
		tdir = d[1];
		d[1] = d[2];
		d[2] = tdir;
	}

	if (d[1] != DI_NODIR && d[1] != turnaround
		&& SV_StepDirection(actor, goalpos, d[1], dist, true))
		return;

	if (d[2] != DI_NODIR && d[2] != turnaround
		&& SV_StepDirection(actor, goalpos, d[2], dist, true))
		return;

	/* there is no direct path to the player, so pick another direction */

	if (olddir != DI_NODIR && SV_StepDirection(actor, goalpos, olddir, dist, true))
		return;

	if (rand() & 1) 	/*randomly determine direction of search*/
	{
		for (tdir = 0; tdir <= 315; tdir += 45)
			if (tdir != turnaround && SV_StepDirection(actor, goalpos, tdir, dist, true))
				return;
	}
	else
	{
		for (tdir = 315; tdir >= 0; tdir -= 45)
			if (tdir != turnaround && SV_StepDirection(actor, goalpos, tdir, dist, true))
				return;
	}

	if (turnaround != DI_NODIR && SV_StepDirection(actor, goalpos, turnaround, dist, true))
		return;

	actor->ideal_yaw = olddir;		// can't move

	// if a bridge was pulled out from underneath a monster, it may not have
	// a valid standing position at all

	if (!M_CheckBottom(actor))
		SV_FixCheckBottom(actor);
}

void SV_MoveRandom(edict_t* actor, vec3_t dest, float dist)
{
	float i, tdir;

	// try to take a full step in any random direction
	tdir = GetRandom(0, 360);
	for (i = 0; i < 7; i++)
	{
		if (SV_StepDirection(actor, dest, tdir, dist, false))
			return;
		tdir += 45;
		AngleCheck(&tdir);
	}
}

static qboolean SV_FlyIdleWallDodge(edict_t *actor, float dist)
{
	vec3_t start, end, forward, wall_angles;
	trace_t tr;
	float yaw1, yaw2;

	if (!(actor->flags & FL_FLY))
		return false;

	AngleVectors(actor->s.angles, forward, NULL, NULL);
	G_EntMidPoint(actor, start);
	VectorMA(start, max(64.0f, fabsf(dist) * 4.0f), forward, end);
	tr = gi.trace(start, NULL, NULL, end, actor, MASK_MONSTERSOLID);
	if (tr.fraction == 1.0f || tr.startsolid || tr.allsolid)
		return false;

	vectoangles(tr.plane.normal, wall_angles);
	yaw1 = wall_angles[YAW] + 90.0f;
	yaw2 = wall_angles[YAW] - 90.0f;
	AngleCheck(&yaw1);
	AngleCheck(&yaw2);

	actor->monsterinfo.lefty = 1 - actor->monsterinfo.lefty;
	if (actor->monsterinfo.lefty)
	{
		if (SV_StepDirection(actor, NULL, yaw1, dist, true))
			return true;
		return SV_StepDirection(actor, NULL, yaw2, dist, true);
	}

	if (SV_StepDirection(actor, NULL, yaw2, dist, true))
		return true;
	return SV_StepDirection(actor, NULL, yaw1, dist, true);
}

void SV_NewChaseDir2 (edict_t *self, vec3_t dest, float dist)
{
	float	minyaw, maxyaw, bestyaw, temp;
	vec3_t	v;

	if (!dest)
		return;

	VectorSubtract(dest, self->s.origin, v);
	bestyaw = vectoyaw(v);

	minyaw = bestyaw - 90;
	maxyaw = bestyaw + 90;

	// try a step forward +/- 90 degrees from ideal yaw
	if (CheckYawStep(self, minyaw, maxyaw, dist))
	{
		//gi.dprintf("%s took a step +/- 90 degrees from ideal yaw\n", 
		//        GetMonsterKindString(self->mtype));
		return;
	}

	// that didn't work, so flip the search pattern and
	// try going in the opposite direction
	temp = minyaw;
	minyaw = maxyaw;
	maxyaw = temp;
	if (CheckYawStep(self, minyaw, maxyaw, dist))
	{
		//gi.dprintf("%s took a step 180 degrees from ideal yaw\n", 
		//	GetMonsterKindString(self->mtype));
		return;
	}
	
	//gi.dprintf("couldnt find a better direction!\n");
	SV_NewChaseDir(self, self->goalentity, dist);
}

/*
===============
M_ChangeYaw

===============
*/
void M_ChangeYaw (edict_t *ent)
{
	float	ideal;
	float	current;
	float	move;
	float	speed;
	
	current = anglemod(ent->s.angles[YAW]);
	ideal = ent->ideal_yaw;

	if (current == ideal)
		return;

	move = ideal - current;
	speed = ent->yaw_speed * FRAMETIME * 10;
	if (ideal > current)
	{
		if (move >= 180)
			move = move - 360;
	}
	else
	{
		if (move <= -180)
			move = move + 360;
	}
	if (move > 0)
	{
		if (move > speed)
			move = speed;
	}
	else
	{
		if (move < -speed)
			move = -speed;
	}
	
	ent->s.angles[YAW] = anglemod (current + move);

}

/*
======================
SV_CloseEnough

======================
*/
qboolean SV_CloseEnough (edict_t *ent, edict_t *goal, float dist)
{
	int		i;
	
	for (i=0 ; i<3 ; i++)
	{
		if (goal->absmin[i] > ent->absmax[i] + dist)
			return false;
		if (goal->absmax[i] < ent->absmin[i] - dist)
			return false;
	}
	return true;
}

qboolean SV_CloseEnough1 (edict_t *ent, vec3_t goalpos, float dist)
{
	int		i;
	
	for (i=0 ; i<3 ; i++)
	{
		if (goalpos[i] > ent->absmax[i] + dist)
			return false;
		if (goalpos[i] < ent->absmin[i] - dist)
			return false;
	}
	return true;
}

// finds escape angle perpendicular to wall that moves us in the direction we were facing
// FIXME: modify to move closer to goal position/waypoint rather than relying on monster angles
// FIXME: how will this work if the "obstruction" is open space (i.e. we can't walk because we will fall!)
void M_AvoidObstruction(edict_t* self, vec3_t pos, float dist)
{
	vec3_t	angles, forward, start, end;
	float	cl_yaw, ideal_yaw, angle1, angle2, delta1, delta2;
	trace_t	tr;

	// calculate next move
	//yaw = self->ideal_yaw * M_PI * 2 / 360;
	//move[0] = cos(yaw) * dist;
	//move[1] = sin(yaw) * dist;
	//move[2] = 0;

	// get monster forward angle
	AngleVectors(self->s.angles, NULL, forward, NULL);
	vectoangles(forward, forward);

	// get monster yaw
	if (pos)
	{
		// yaw angle to pos (usually monster's intended destination)
		VectorSubtract(pos, self->s.origin, end);
		VectorNormalize(end);
		vectoangles(end, end);
		cl_yaw = end[YAW];
	}
	else
		cl_yaw = self->s.angles[YAW];
	AngleCheck(&cl_yaw);

	// trace from monster to wall
	G_EntMidPoint(self, start);
	VectorMA(start, 64, forward, end);
	tr = gi.trace(start, NULL, NULL, end, self, MASK_MONSTERSOLID);

	// did the trace hit something?
	//FIXME: if the trace hit nothing (air), then this function won't really help avoid anything!
	if (tr.fraction != 1)
	{
		// get monster angles in relation to wall
		VectorCopy(tr.plane.normal, angles);
		vectoangles(angles, angles);
	}

	// delta between monster yaw and wall yaw should be no more than 90 degrees
	// else, turn wall angles around 180 degrees
	if (fabs(cl_yaw - angles[YAW]) > 90)
		angles[YAW] += 180;
	ValidateAngles(angles);

	// possible escape angle 1
	angle1 = angles[YAW] + 90;
	AngleCheck(&angle1);
	delta1 = fabs(angle1 - cl_yaw);
	// possible escape angle 2
	angle2 = angles[YAW] - 90;
	AngleCheck(&angle2);
	delta2 = fabs(angle2 - cl_yaw);

	// take the shorter route
	if (delta1 > delta2)
	{
		ideal_yaw = angle2;
		//mult = 1.0 - delta2 / 90.0;
	}
	else
	{
		ideal_yaw = angle1;
		//mult = 1.0 - delta1 / 90.0;
	}

	// go full speed if we are turned at least half way towards ideal yaw
	//if (mult >= 0.5)
	//	mult = 1.0;

	// modify speed
	//dist *= mult;
	//if (dist < 5)
	//	dist = 5;

	// recalculate with new heading
	//yaw = ideal_yaw * M_PI * 2 / 360;
	/*
	move[0] = cos(yaw)*dist;
	move[1] = sin(yaw)*dist;
	move[2] = 0;
	*/

	//gi.dprintf("**can't walk dist %.1f, wall@%.1f you@%.1f ideal@%.1f tr.fraction=%f\n", dist, angles[YAW], cl_yaw, ideal_yaw, tr.fraction);

	if (!SV_StepDirection(self, pos, ideal_yaw, dist, false))
	{
		//gi.dprintf("couldn't avoid obstruction!\n");
		return;
	}
		

	if (!M_CheckBottom(self))
		SV_FixCheckBottom(self);
}

// NOTE: pos should be the final goal, because monster will stop moving after reaching it!
void M_MoveToPosition(edict_t* ent, vec3_t pos, float dist, qboolean stop_when_close)
{
	vec3_t v;

	// stay in-place for medic healing
	if (ent->holdtime > level.time) 
		return;

	// need to be touching the ground
	if (!ent->groundentity && !(ent->flags & (FL_FLY|FL_SWIM)) && !ent->waterlevel)
	{
		//gi.dprintf("not touching ground\n");
		return;
	}

	// if the next step touches our goal position, then stop
	if (stop_when_close && SV_CloseEnough1(ent, pos, dist))
	{
		//gi.dprintf("close enough %.0f\n", distance(ent->s.origin, pos));
		// look at the final position
		VectorSubtract(pos, ent->s.origin, v);
		VectorNormalize(v);
		ent->ideal_yaw = vectoyaw(v);
		M_ChangeYaw(ent);
		return;
	}

	// dont move so fast in the water
	if (!(ent->flags & (FL_FLY|FL_SWIM)) && (ent->waterlevel > 1))
		dist *= 0.5;

	// if we can't take a step, try moving in another direction
	if (!SV_StepDirection (ent, pos, ent->ideal_yaw, dist, true))
	{
		//gi.dprintf("couldn't step\n");
		// if the monster hasn't moved much, then increment
		// the number of frames it has been stuck
		if (distance(ent->s.origin, ent->monsterinfo.stuck_org) < 64)
			ent->monsterinfo.stuck_frames++;
		else
			ent->monsterinfo.stuck_frames = 0;
		//gi.dprintf("stuck frames=%d\n",ent->monsterinfo.stuck_frames);

		// record current position for comparison
		VectorCopy(ent->s.origin, ent->monsterinfo.stuck_org);

		// az: stuck for 10 seconds? begone, respawn!
		if (ent->inuse && ent->monsterinfo.stuck_frames > qf2sf(100) && !G_GetClient(ent) && invasion->value) {
			M_Remove(ent, true, true);
			return;
		}

		// attempt a course-correction
		// first, we attempt to find an angle perpendicular to wall to escape, and then we try a random direction
		if (ent->inuse && (level.time > ent->monsterinfo.bump_delay))
		{
			//gi.dprintf("trying course correction\n");
			// find a random path if we are stuck, or 50% of the time if we can't see the next waypoint
			if (ent->monsterinfo.stuck_frames > 2
				|| (random() > 0.5 && !G_IsClearPath(ent, MASK_MONSTERSOLID, ent->s.origin, pos)))
			{
				SV_MoveRandom(ent, pos, dist);
				ent->monsterinfo.bump_delay = level.time + FRAMETIME* GetRandom(3, 9);
				return;
			}
			//gi.dprintf("M_MoveToPosition() tried course correction\n");
			/*
			if (ent->flags & FL_FLY && !G_IsClearPath(ent, MASK_MONSTERSOLID, ent->s.origin, pos))
			{
				//SV_StepDirection(ent, pos, GetRandom(0, 360), dist, false);
				SV_MoveRandom(ent, pos, dist);
				ent->monsterinfo.bump_delay = level.time + FRAMETIME*GetRandom(5, 15);
				return;
			}*/

			//SV_NewChaseDir3(ent, pos, dist);
			M_AvoidObstruction(ent, pos, dist);//FIXME: this will cause a monster to continue to be stuck if there is no wall to escape!

			ent->monsterinfo.bump_delay = level.time + FRAMETIME * GetRandom(3, 9);
			return;
		}
	}
	//else
		//gi.dprintf("step OK %.0f\n", dist);
}

/*
======================
M_MoveToGoal
======================
*/
void M_MoveToGoal (edict_t *ent, float dist)
{
	edict_t		*goal;

	goal = ent->goalentity;

	if (ent->holdtime > level.time) // stay in-place for medic healing
		return;

	if (!ent->groundentity && !(ent->flags & (FL_FLY|FL_SWIM)) && !ent->waterlevel)
	{
		//gi.dprintf("not touching ground\n");
		return;
	}

// if the next step hits the enemy, return immediately
	if (ent->enemy && (ent->enemy->solid == SOLID_BBOX) 
		&& SV_CloseEnough (ent, ent->enemy, dist)
		&& !((ent->monsterinfo.aiflags & AI_ALTERNATE_FLY) && fly_use_alternate_step(ent, NULL)) )
//GHz START
	{
		vec3_t v;

		// we need to keep turning to avoid getting stuck doing nothing
		if (ent->goalentity && ent->goalentity->inuse) // 3.89 make sure the monster still has a goal!
		{
			VectorSubtract(ent->goalentity->s.origin, ent->s.origin, v);
			VectorNormalize(v);
			ent->ideal_yaw = vectoyaw(v);
			M_ChangeYaw(ent);
		}
		return;
	}
//GHz END

	// dont move so fast in the water
	if (!(ent->flags & (FL_FLY|FL_SWIM)) && (ent->waterlevel > 1))
		dist *= 0.5;

// bump around...
	// if we can't take a step, try moving in another direction
	if (!SV_StepDirection (ent, NULL, ent->ideal_yaw, dist, true))
	{
		//gi.dprintf("couldnt step\n");
		// if the monster hasn't moved much, then increment
		// the number of frames it has been stuck
		if (distance(ent->s.origin, ent->monsterinfo.stuck_org) < 64)
			ent->monsterinfo.stuck_frames++;
		else
			ent->monsterinfo.stuck_frames = 0;

		// record current position for comparison
		VectorCopy(ent->s.origin, ent->monsterinfo.stuck_org);

		// attempt a course-correction
		if (ent->inuse && (level.time > ent->monsterinfo.bump_delay))
		{
			//gi.dprintf("tried course correction %s\n", ent->goalentity?"true":"false");
			if ((ent->flags & FL_FLY) && (!goal || goal == world))
			{
				if (!SV_FlyIdleWallDodge(ent, dist))
					SV_MoveRandom(ent, NULL, dist);
			}
			else
			{
				SV_NewChaseDir(ent, goal, dist);
			}
			ent->monsterinfo.bump_delay = level.time + FRAMETIME*GetRandom(2, 5);
			return;
		}
	}
	//else
//	gi.dprintf("M_MoveToGoal() was successful %.0f\n",dist);
}


/*
===============
M_walkmove
===============
*/
qboolean M_walkmove (edict_t *ent, float yaw, float dist)
{
	const float	original_yaw=yaw;//GHz
	vec3_t	move;
	
	if (!ent->groundentity && !(ent->flags & (FL_FLY|FL_SWIM)))
		return false;

	yaw = yaw*M_PI*2 / 360;

	move[0] = cos(yaw)*dist;
	move[1] = sin(yaw)*dist;
	move[2] = 0;
	
	if (IsABoss(ent) || ent->mtype == P_TANK)
	{
		//return M_Move(ent, move, true);
		if (!M_Move(ent, move, true))
		{
			float	angle1, angle2, delta1, delta2, cl_yaw, ideal_yaw, mult;
			vec3_t	start, end, angles, right;
			trace_t tr;

			// get monster angles
			AngleVectors(ent->s.angles, NULL, right, NULL);
			//vectoangles(forward, forward);
			vectoangles(right, right);

			// get client yaw (FIXME: use mons yaw instead?)
			cl_yaw = ent->s.angles[YAW];
			AngleCheck(&cl_yaw);

			// trace from monster to wall
			VectorCopy(ent->s.origin, start);
			VectorMA(start, 64, move, end);
			tr = gi.trace(start, ent->mins, ent->maxs, end, ent, MASK_SHOT);

			// get monster angles in relation to wall
			VectorCopy(tr.plane.normal, angles);
			vectoangles(angles, angles);

			// monster is moving sideways, so use sideways vector instead
			if ((int)original_yaw==(int)right[YAW])
			{
				cl_yaw = right[YAW];
				AngleCheck(&cl_yaw);
			}

			// delta between monster yaw and wall yaw should be no more than 90 degrees
			// else, turn wall angles around 180 degrees
			if (fabs(cl_yaw-angles[YAW]) > 90)
				angles[YAW]+=180;
			ValidateAngles(angles);

			// possible escape angle 1
			angle1 = angles[YAW]+90;
			AngleCheck(&angle1);
			delta1 = fabs(angle1-cl_yaw);
			// possible escape angle 2
			angle2 = angles[YAW]-90;
			AngleCheck(&angle2);
			delta2 = fabs(angle2-cl_yaw);

			// take the shorter route
			if (delta1 > delta2)
			{
				ideal_yaw = angle2;
				mult = 1.0-delta2/90.0;
			}
			else
			{
				ideal_yaw = angle1;
				mult = 1.0-delta1/90.0;
			}
			
			// go full speed if we are turned at least half way towards ideal yaw
			if (mult >= 0.5)
				mult = 1.0;

			// modify speed
			dist*=mult;

			// recalculate with new heading
			yaw = ideal_yaw*M_PI*2 / 360;
			move[0] = cos(yaw)*dist;
			move[1] = sin(yaw)*dist;
			move[2] = 0;
				
			//gi.dprintf("can't walk wall@%.1f you@%.1f ideal@%.1f\n", angles[YAW], cl_yaw, ideal_yaw);

			return M_Move(ent, move, true);
		}
		return true;
	}
	else if (level.time > ent->holdtime) // stay in-place for medic healing
		return SV_movestep(ent, NULL, move, true);
	else
		return false;
}
