/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
--------------------------------------------------------------
The ACE Bot is a product of Steve Yeager, and is available from
the ACE Bot homepage, at http://www.axionfx.com/ace.

This program is a modification of the ACE Bot, and is therefore
in NO WAY supported by Steve Yeager.
*/

#include "g_local.h"
#include "ai_local.h"

qboolean AI_IsProjectile(edict_t* ent);
qboolean BOT_DMclass_Ucmd_Move(edict_t* self, float movespeed, usercmd_t* ucmd, qboolean move_vertical, qboolean check_move);

//ACE

//GHz: FIXME: this function fails when the bot is against a box
// it only returns false when the bot encounters air/lava/slime, i.e. it's more of a hazard check (should I move?) than check if we can move
// something like AI_ClearWalkingPath would be a better check to determine if the bot can actually move (or the one used by monsters/drones)
//==========================================
// AI_CanMove
// Checks if bot can move (really just checking the ground)
// Also, this is not a real accurate check, but does a
// pretty good job and looks for lava/slime.  
//==========================================
qboolean AI_CanMove(edict_t* self, int direction)
{
	vec3_t forward, right;
	vec3_t offset, start, end;
	vec3_t angles;
	trace_t tr;

	//AI_DebugPrintf("AI_CanMove()\n");

	// Now check to see if move will move us off an edge
	VectorCopy(self->s.angles, angles);

	if (direction == BOT_MOVE_LEFT)
		angles[1] += 90;
	else if (direction == BOT_MOVE_RIGHT)
		angles[1] -= 90;
	else if (direction == BOT_MOVE_BACK)
		angles[1] -= 180;


	// Set up the vectors
	AngleVectors(angles, forward, right, NULL);

	//GHz: starting position is 36 units forward, 0 right, 24 up from origin
	VectorSet(offset, 36, 0, 24);
	G_ProjectSource(self->s.origin, offset, forward, right, start);

	//GHz: ending position is 36 units forward, 0 right, 100 below origin
	VectorSet(offset, 36, 0, -100);
	G_ProjectSource(self->s.origin, offset, forward, right, end);

	tr = gi.trace(start, NULL, NULL, end, self, MASK_AISOLID);

	if (tr.fraction == 1.0 || tr.contents & (CONTENTS_LAVA | CONTENTS_SLIME))
	{
		if (AIDevel.debugChased)	//jal: is too spammy. Temporary disabled
			safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: move blocked\n", self->ai.pers.netname);
		return false;
	}

	//if (VectorLength(self->velocity) < 36)
	//	gi.dprintf("WARNING: AI_CanMove says we can move, but velocity is near zero!\n");

	return true;// yup, can move
}

qboolean AI_CanMove1(edict_t *self, int direction)
{
	vec3_t forward, right;
	vec3_t offset,start,end;
	vec3_t angles;
	trace_t tr;

	//AI_DebugPrintf("AI_CanMove1()\n");

	// Now check to see if move will move us off an edge
	VectorCopy(self->s.angles,angles);

	if(direction == BOT_MOVE_LEFT)
		angles[1] += 90;
	else if(direction == BOT_MOVE_RIGHT)
		angles[1] -= 90;
	else if(direction == BOT_MOVE_BACK)
		angles[1] -=180;


	// Set up the vectors
	AngleVectors (angles, forward, right, NULL);

	VectorSet(offset, 36, 0, 24);
	G_ProjectSource (self->s.origin, offset, forward, right, start);

	VectorSet(offset, 36, 0, -100);
	G_ProjectSource (self->s.origin, offset, forward, right, end);

	tr = gi.trace( start, NULL, NULL, end, self, MASK_AISOLID );

	if(tr.fraction == 1.0 || tr.contents & (CONTENTS_LAVA|CONTENTS_SLIME))
	{
		if(AIDevel.debugChased)	//jal: is too spammy. Temporary disabled
			safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: move blocked\n", self->ai.pers.netname);
		return false;
	}

	// Try to not bump into a goddamn wall. -az

	VectorSet(offset, 36, 0, 0);
	G_ProjectSource (self->s.origin, offset, forward, right, start);

	VectorSet(offset, 36, 65, 15);
	G_ProjectSource (self->s.origin, offset, forward, right, end);

	tr = gi.trace ( start, NULL, NULL, end, self, MASK_AISOLID );

	if (tr.ent)
	{
		if (tr.ent->takedamage && tr.ent->solid != SOLID_NOT)
			self->enemy = tr.ent;
		AI_DebugPrintf("blocked1\n");
		return false;
	}
	if(tr.fraction != 1.0 || tr.contents & (CONTENTS_SOLID))
	{
		AI_DebugPrintf("blocked2\n");
		return false;
	}

	return true;// yup, can move
}


//===================
//  AI_IsStep
//  Checks the floor one step below the player. Used to detect
//  if the player is really falling or just walking down a stair.
//===================
qboolean AI_IsStep (edict_t *ent)
{
	vec3_t		point;
	trace_t		trace;
	
	//AI_DebugPrintf("AI_IsStep()\n");

	//determine a point below
	point[0] = ent->s.origin[0];
	point[1] = ent->s.origin[1];
	point[2] = ent->s.origin[2] - (1.6*AI_STEPSIZE);
	
	//trace to point
//	trap_Trace (&trace, ent->s.origin, ent->mins, ent->maxs, point, ent, MASK_PLAYERSOLID);
	trace = gi.trace( ent->s.origin, ent->mins, ent->maxs, point, ent, MASK_PLAYERSOLID);
	
	if (trace.plane.normal[2] < 1 && !trace.startsolid) // 0.7   [2] < 0.7
		return false;
	
	//found solid.
	return true;
}


//==========================================
// AI_IsLadder
// check if entity is touching in front of a ladder
//==========================================
qboolean AI_IsLadder(vec3_t origin, vec3_t v_angle, vec3_t mins, vec3_t maxs, edict_t *passent)
{
	vec3_t	spot;
	vec3_t	flatforward, zforward;
	trace_t	trace;

	//AI_DebugPrintf("AI_IsLadder()\n");

	AngleVectors( v_angle, zforward, NULL, NULL);

	// check for ladder
	flatforward[0] = zforward[0];
	flatforward[1] = zforward[1];
	flatforward[2] = 0;
	VectorNormalize (flatforward);

	VectorMA ( origin, 1, flatforward, spot);

//	trap_Trace(&trace, self->s.origin, self->mins, self->maxs, spot, self, MASK_AISOLID);
	trace = gi.trace( origin, mins, maxs, spot, passent, MASK_AISOLID);
	
//	if ((trace.fraction < 1) && (trace.surfFlags & SURF_LADDER))
	if ((trace.fraction < 1) && (trace.contents & CONTENTS_LADDER))
		return true;

	return false;
}


//==========================================
// AI_CheckEyes
// Helper for ACEMV_SpecialMove. 
// Tries to turn when in front of obstacle
//==========================================
qboolean AI_CheckEyes(edict_t *self, usercmd_t *ucmd)
{
	vec3_t  forward, right;
	vec3_t  leftstart, rightstart,focalpoint;
	vec3_t  dir,offset;
	trace_t traceRight;
	trace_t traceLeft;

	AI_DebugPrintf("AI_CheckEyes()\n");

	// Get current angle and set up "eyes"
	VectorCopy(self->s.angles,dir);
	AngleVectors (dir, forward, right, NULL);

	if(!self->movetarget)
		VectorSet(offset,200,0,self->maxs[2]*0.5); // focalpoint
	else
		VectorSet(offset,64,0,self->maxs[2]*0.5); // wander focalpoint 
	
	G_ProjectSource (self->s.origin, offset, forward, right, focalpoint);

	VectorSet(offset, 0, 18, self->maxs[2]*0.5);
	G_ProjectSource (self->s.origin, offset, forward, right, leftstart);
	offset[1] -= 36; //VectorSet(offset, 0, -18, self->maxs[2]*0.5);
	G_ProjectSource (self->s.origin, offset, forward, right, rightstart);

//	trap_Trace(&traceRight, rightstart, NULL, NULL, focalpoint, self, MASK_AISOLID);
//	trap_Trace(&traceLeft, leftstart, NULL, NULL, focalpoint, self, MASK_AISOLID);
	traceRight = gi.trace( rightstart, NULL, NULL, focalpoint, self, MASK_AISOLID);
	traceLeft = gi.trace( leftstart, NULL, NULL, focalpoint, self, MASK_AISOLID);

	// Find the side with more open space and turn
	if(traceRight.fraction != 1 || traceLeft.fraction != 1)
	{
		if(traceRight.fraction > traceLeft.fraction)
			self->s.angles[YAW] += (1.0 - traceLeft.fraction) * 45.0;
		else
			self->s.angles[YAW] += -(1.0 - traceRight.fraction) * 45.0;
		
		ucmd->forwardmove = 400;
		return true;
	}
				
	return false;
}

//==========================================
// AI_SpecialMove
// Handle special cases of crouch/jump
// If the move is resolved here, this function returns true.
//==========================================
qboolean AI_SpecialMove(edict_t *self, usercmd_t *ucmd)
{
	vec3_t forward;
	trace_t tr;
	vec3_t	boxmins, boxmaxs, boxorigin;
	qboolean moveattack = false;//GHz

	AI_DebugPrintf("AI_SpecialMove()\n");

	// Get current direction
	if (self->ai.state == BOT_STATE_MOVEATTACK)//GHz: use move_vec instead when using MOVEATTACK state, since we're moving independent of view angles
	{
		moveattack = true;
		VectorCopy(self->ai.move_vector, forward);
		forward[2] = 0;
		VectorNormalize(forward);
	}
	else
		AngleVectors( tv(0, self->s.angles[YAW], 0), forward, NULL, NULL );

	//make sure we are bloqued
	VectorCopy( self->s.origin, boxorigin );
	VectorMA( boxorigin, 8, forward, boxorigin ); //move box by 8 to front
	tr = gi.trace( self->s.origin, self->mins, self->maxs, boxorigin, self, MASK_AISOLID);
	if (!tr.startsolid && tr.fraction == 1.0) // not bloqued
	{
		//gi.dprintf("not blocked?\n");
		return false;
	}

	if( self->ai.pers.moveTypesMask & LINK_CROUCH || self->ai.is_swim )
	{
		//crouch box
		VectorCopy( self->s.origin, boxorigin );
		VectorCopy( self->mins, boxmins );
		VectorCopy( self->maxs, boxmaxs );
		boxmaxs[2] = 14;	//crouched size
		VectorMA( boxorigin, 8, forward, boxorigin ); //move box by 8 to front
		//see if bloqued
		tr = gi.trace( boxorigin, boxmins, boxmaxs, boxorigin, self, MASK_AISOLID);
		if( !tr.startsolid ) // can move by crouching
		{
			if (moveattack)//GHz
				BOT_DMclass_Ucmd_Move(self, 400, ucmd, false, false);
			else
				ucmd->forwardmove = 400;
			ucmd->upmove = -400;
			return true;
		}
	}

	if( self->ai.pers.moveTypesMask & LINK_JUMP && self->groundentity )
	{
		//jump box
		VectorCopy( self->s.origin, boxorigin );
		VectorCopy( self->mins, boxmins );
		VectorCopy( self->maxs, boxmaxs );
		VectorMA( boxorigin, 8, forward, boxorigin );	//move box by 8 to front
		//
		boxorigin[2] += ( boxmins[2] + AI_JUMPABLE_HEIGHT );	//put at bottom + jumpable height
		boxmaxs[2] = boxmaxs[2] - boxmins[2];	//total player box height in boxmaxs
		boxmins[2] = 0;
		if (boxmaxs[2] > AI_JUMPABLE_HEIGHT) //the player is smaller than AI_JUMPABLE_HEIGHT
		{
			boxmaxs[2] -= AI_JUMPABLE_HEIGHT;
			tr = gi.trace(boxorigin, boxmins, boxmaxs, boxorigin, self, MASK_AISOLID);
			if (!tr.startsolid)	//can move by jumping
			{
				//gi.dprintf("jump!\n");
				if (moveattack)
					BOT_DMclass_Ucmd_Move(self, 400, ucmd, false, false);
				else
					ucmd->forwardmove = 400;
				ucmd->upmove = 400;

				return true;
			}
		}
		//else
		//	gi.dprintf("jump height %.0f\n", boxmaxs[2]);
	}

	//nothing worked, check for turning
	return AI_CheckEyes(self, ucmd);
}


//==========================================
// AI_ChangeAngle
// Make the change in angles a little more gradual, not so snappy
// Subtle, but noticeable.
// 
// Modified from the original id ChangeYaw code...
//==========================================
void AI_ChangeAngle (edict_t *ent)
{
	float	ideal_yaw;
	float   ideal_pitch;
	float	current_yaw;
	float   current_pitch;
	float	move;
	float	speed;
	vec3_t  ideal_angle;

	// Normalize the move angle first
	VectorNormalize(ent->ai.move_vector);

	current_yaw = anglemod(ent->s.angles[YAW]);
	current_pitch = anglemod(ent->s.angles[PITCH]);

	vectoangles (ent->ai.move_vector, ideal_angle);

	ideal_yaw = anglemod(ideal_angle[YAW]);
	ideal_pitch = anglemod(ideal_angle[PITCH]);


	// Yaw
	if (current_yaw != ideal_yaw)
	{
		move = ideal_yaw - current_yaw;
		speed = ent->yaw_speed;
		if (ideal_yaw > current_yaw)
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
		ent->s.angles[YAW] = anglemod (current_yaw + move);
	}


	// Pitch
	if (current_pitch != ideal_pitch)
	{
		move = ideal_pitch - current_pitch;
		speed = ent->yaw_speed;
		if (ideal_pitch > current_pitch)
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
		ent->s.angles[PITCH] = anglemod (current_pitch + move);
	}
}

//==========================================
// AI_DodgeProjectiles
// attempts to move bot away from projectiles and other hazardous entities
// returns true if the bot is trying to dodge, or false otherwise
// works pretty well but the escape vectors aren't validated, which could push the bot into a wall, off a cliff, into another hazard, etc.
//==========================================
qboolean AI_DodgeProjectiles(edict_t* self, usercmd_t* ucmd)
{
	vec3_t start, end, movedir, v, angles;
	trace_t tr;
	float speed, eta, mv_spd;

	// bot is locked in an escape vector for a period of time
	if (self->ai.locked_movetime > level.time)
	{
		//gi.dprintf("%d: %s: continuing to dodge...\n", (int)level.framenum, __func__);
		ucmd->forwardmove = self->ai.locked_forwardmove;
		ucmd->sidemove = self->ai.locked_sidemove;
		return true;
	}

	// copy projectile's origin to start
	VectorCopy(self->movetarget->s.origin, start);
	// is it moving?
	if ((speed = VectorLength(self->movetarget->velocity)) > 0)
	{
		// calculate likely trajectory and point of impact
		if (VectorEmpty(self->movetarget->movedir))
		{
			// the move direction wasn't set, so use the velocity instead
			VectorCopy(self->movetarget->velocity, movedir);
			VectorNormalize(movedir);
		}
		else
			VectorCopy(self->movetarget->movedir, movedir);
		// calculate possible ending position if we follow the movedir as far as we can in a straight line
		// note: this could be improved by taking the projectile's physics into account, but this is "good enough"
		VectorMA(start, 8192, movedir, end);
		// trace it until it hits something
		tr = gi.trace(start, NULL, NULL, end, self->movetarget, MASK_SHOT);
		// will it hit us?
		if (tr.ent == self)
		{
			// when?
			VectorSubtract(tr.endpos, start, v);
			eta = VectorLength(v) / speed;
			// time check: don't bother if it's too close or too far away
			if (eta > 0.1 && eta < 1.0)
			{
				// calculate escape vector perpendicular to normal
				// (x, y) rotated 90 degrees around (0, 0) is (-y, x)
				v[0] = tr.plane.normal[1] * -1;
				v[1] = tr.plane.normal[0];
				v[2] = tr.plane.normal[2];
				// copy escape vector to bot's move_vector
				VectorCopy(v, self->ai.move_vector);
				// GTFO! move!
				BOT_DMclass_Ucmd_Move(self, 400, ucmd, false, false);
				// calculate speed in the direction of the escape vector
				mv_spd = fabsf(DotProduct(self->ai.move_vector, self->velocity));
				// if we're going fast enough in that direction, jump to gain additional speed and distance
				if (mv_spd > 250 && self->groundentity)
					ucmd->upmove = 400;
				//gi.dprintf("AI_DodgeProjectiles: *** INCOMING %s! eta:%.2f GET OUTTA THE WAY! ***\n", self->movetarget->classname, eta);
				gi.sound(self, CHAN_VOICE, gi.soundindex("speech/yell/lookout.wav"), 1, ATTN_NORM, 0);
				gi.WriteByte(svc_temp_entity);
				gi.WriteByte(TE_TELEPORT_EFFECT);
				gi.WritePosition(self->s.origin);
				gi.multicast(self->s.origin, MULTICAST_PVS);
				// hold this direction for awhile
				self->ai.locked_movetime = level.time + 0.5;
				self->ai.locked_forwardmove = ucmd->forwardmove;
				self->ai.locked_sidemove = ucmd->sidemove;
				return true;
			}
			//gi.dprintf("AI_DodgeProjectiles: %s incoming (speed: %.0f) will hit in %.2f seconds\n", self->movetarget->classname, speed, eta);
			return false;//not going to hit...yet
		}
		// not going to hit us directly
		VectorCopy(tr.endpos, start); // record the position for further analysis
		// draw debug trail between projectile origin and estimated landing position
		//G_DrawDebugTrail(self->movetarget->s.origin, start);
		//gi.dprintf("%d: %s: projectile won't hit directly\n", (int)level.framenum, __func__);
	}

	// we've arrived here because: 1) the projectile isn't moving OR 2) it's moving but won't hit us directly
	if (self->movetarget->dmg_radius > 0 && (self->movetarget->radius_dmg || self->movetarget->dmg))
	{
		float dist;
		// get vector pointing to the origin or impact point of the projectile
		//VectorSubtract(start, self->s.origin, v);
		VectorSubtract(self->s.origin, start, v);

		// show BFG explosion at projectile origin
		//gi.WriteByte(svc_temp_entity);
		//gi.WriteByte(TE_BFG_EXPLOSION);
		//gi.WritePosition(start);
		//gi.multicast(start, MULTICAST_PVS);

		// draw debug trail between the projectile origin and us
		//G_DrawDebugTrail(self->s.origin, start);
		//G_DrawLaser(self, start, self->s.origin, 0xd0d1d2d3, 1);

		// are we within its damage radius?
		dist = VectorLength(v);
		if (dist > self->movetarget->dmg_radius)
		{
			//gi.dprintf("%d: %s: %s explosive out of range: %f\n", (int)level.framenum, __func__, self->movetarget->classname, dist);
			return false;
		}
		//GTFO!
		VectorNormalize(v);
		//VectorInverse(v); // reverse the direction of the vector to point away from the point of impact
		VectorCopy(v, self->ai.move_vector);
		BOT_DMclass_Ucmd_Move(self, 400, ucmd, false, false);//move!
		mv_spd = fabsf(DotProduct(self->ai.move_vector, self->velocity));// speed in the direction of escape vector
		if (mv_spd > 250 && self->groundentity)
			ucmd->upmove = 400;//jump!
		//gi.dprintf("%d: %s: *** EXPLOSIVE %s NEARBY! GET OUTTA THE WAY! ***\n", (int)level.framenum, __func__, self->movetarget->classname);
		gi.sound(self, CHAN_VOICE, gi.soundindex("speech/yell/firehole.wav"), 1, ATTN_NORM, 0);
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_TELEPORT_EFFECT);
		gi.WritePosition(self->s.origin);
		gi.multicast(self->s.origin, MULTICAST_PVS);
		self->ai.locked_forwardmove = ucmd->forwardmove;
		self->ai.locked_sidemove = ucmd->sidemove;
		self->ai.locked_movetime = level.time + 0.5;
		return true;
	}
	// no danger
	//gi.dprintf("%d: AI_DodgeProjectiles: no danger\n", (int)level.framenum);
	return false;
}

void BOT_DMclass_AvoidObstacles(edict_t* self, usercmd_t* ucmd, int current_node_flags);//GHz
//==========================================
// AI_MoveToGoalEntity
// Set bot to move to it's movetarget. Short range goals
//==========================================
qboolean AI_MoveToGoalEntity(edict_t *self, usercmd_t *ucmd)
{
	if (!self->movetarget || !self->client)
	{
		//gi.dprintf("AI_MoveToGoalEntity invalid\n");
		return false;
	}

	//AI_DebugPrintf("AI_MoveToGoalEntity()\n");
	//gi.dprintf("movetarget: %s\n", self->movetarget->classname);

	// If a rocket or grenade is around deal with it
	// Simple, but effective (could be rewritten to be more accurate)
	/*
	if(!Q_stricmp(self->movetarget->classname,"rocket") ||
	   !Q_stricmp(self->movetarget->classname,"grenade") ||
	   !Q_stricmp(self->movetarget->classname,"hgrenade"))
	{
		VectorSubtract (self->movetarget->s.origin, self->s.origin, self->ai.move_vector);
		AI_ChangeAngle(self);
		if(AIDevel.debugChased && bot_showcombat->value)
			safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: Oh crap a rocket!\n",self->ai.pers.netname);

		// strafe left/right
		if(randomMT()%1 && AI_CanMove(self, BOT_MOVE_LEFT))
				ucmd->sidemove = -400;
		else if(AI_CanMove(self, BOT_MOVE_RIGHT))
				ucmd->sidemove = 400;
		return true;

	}*/

	if (AI_IsProjectile(self->movetarget))
		return AI_DodgeProjectiles(self, ucmd);

	// is our movetarget entity a summons that we own?
	if (AI_IsOwnedSummons(self, self->movetarget))
	{
		vec3_t start, end, dest, v;
		trace_t tr;

		// stop hiding if there is no enemy or he isn't visible
		if (!self->enemy || !self->enemy->inuse || !visible(self, self->enemy))
		{
			self->movetarget = NULL;
			return false;
		}

		VectorCopy(self->movetarget->s.origin, start);
		// get vector pointing behind our summons
		VectorSubtract(start, self->enemy->s.origin, v);
		// normalize it
		VectorNormalize(v);
		// find a point behind the summons
		VectorMA(start, 256, v, end);
		tr = gi.trace(start, NULL, NULL, end, self->movetarget, MASK_AISOLID);
		VectorCopy(tr.endpos, dest);
		// set movement direction toward hiding spot
		// this places the summons safely between the bot and our enemy
		VectorSubtract(dest, self->s.origin, self->ai.move_vector);
	}
	else
		// Set bot's movement direction
		VectorSubtract (self->movetarget->s.origin, self->s.origin, self->ai.move_vector);

	if (self->ai.state == BOT_STATE_MOVEATTACK || self->ai.state == BOT_STATE_ATTACK)//GHz
	{
		if (!BOT_DMclass_Ucmd_Move(self, 400, ucmd, false, true))
		{
			//gi.dprintf("bot can't move\n");
			self->movetarget = NULL;
			ucmd->forwardmove = -400;
			return false;
		}
		return true;
	}

	AI_ChangeAngle(self);
	if(!AI_CanMove(self, BOT_MOVE_FORWARD) ) 
	{
		//gi.dprintf("bot can't move forward\n");//GHz
		self->movetarget = NULL;
		ucmd->forwardmove = -400;
		return false;
	}

	// Check to see if stuck, and if so try to free us
	if (VectorLength(self->velocity) < 37 && self->ai.bloqued_timeout < level.time + 9.0)
	{
		//AI_DebugPrintf("STUCK : % d(% s) -> % s -> % d(% s)\n",
		//	self->ai.current_node, AI_NodeString(current_node_flags),
		//	AI_LinkString(current_link_type), self->ai.next_node, AI_NodeString(next_node_flags));//GHz
		//gi.dprintf("bot can't move forward\n");
		BOT_DMclass_AvoidObstacles(self, ucmd, 0);
		self->movetarget = NULL;
		return false;
	}

	//Move
	ucmd->forwardmove = 400;
	return true;
}


