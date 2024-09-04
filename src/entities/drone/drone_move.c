// m_move.c -- monster movement

#include "g_local.h"

#define	STEPSIZE	18

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

qboolean LandCloserToGoal (edict_t *self, vec3_t goal_pos, vec3_t landing_pos)
{
	// goal position path is obstructed by a wall
	landing_pos[2] += 8; // raise slightly off floor (navis are usually placed 8 units above floor height)
	if (!G_IsClearPath(self, MASK_SOLID, landing_pos, goal_pos))
		return false;
	// landing position places us farther from our goal
	//FIXME: subtracting 16 from landing position is a "cheap fix" for monsters with large hitboxes (i.e. bosses) who fail M_CheckBottom
	if (distance(landing_pos, goal_pos) > distance(self->s.origin, goal_pos))
		return false;
	return true;
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

void GetNodePosition (int nodenum, vec3_t pos);
int NearestWaypointNum(vec3_t start, int* wp);
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
		
		GetNodePosition(self->monsterinfo.waypoint[self->monsterinfo.nextWaypoint], v);
		if (!LandCloserToGoal(self, v, tr.endpos))
		{
			// is the landing position closer to the final waypoint?
			GetNodePosition(self->monsterinfo.waypoint[self->monsterinfo.numWaypoints-1], v);
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
	float		stepsize=STEPSIZE;

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

qboolean M_MoveVertical(edict_t* ent, vec3_t dest, vec3_t neworg)
{
	float        dz;                        // delta Z
	float        idealZ;                // ideal Z position, either a destination waypoint/node or a goal entity
	float        zSpeed = 8;        // Z movement speed
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
		if (ent->goalentity = world)
			return true;
		idealZ = ent->goalentity->s.origin[2] + 16;
		VectorCopy(ent->goalentity->s.origin, goalpos);
	}
	else
		return true; // no goal, no destination - probably idle
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
	if (idealZ < trace.endpos[2] + 16)
		idealZ = trace.endpos[2] + 16;
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
qboolean M_FlyMove(edict_t* ent, vec3_t dest, vec3_t move, qboolean relink)
{
	//float        dz;                        // delta Z
	//float        idealZ;                // ideal Z position, either a destination waypoint/node or a goal entity
	float        zSpeed = 8;        // Z movement speed
	trace_t        trace;
	vec3_t        neworg1, neworg2;
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
/*
	if (jump)
	{
		VectorCopy(oldorg, ent->s.origin);
		CanJumpDown(ent, trace.endpos, true);
		VectorCopy(trace.endpos, ent->s.origin);
	}
*/
	
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
		//ent->velocity[2] = 200;
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
	if (((rand() & 3) & 1) || abs(deltay) > abs(deltax))
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
		&& SV_CloseEnough (ent, ent->enemy, dist) )
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
			SV_NewChaseDir (ent, goal, dist);
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
	float	original_yaw=yaw;//GHz
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

