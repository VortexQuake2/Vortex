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

*/

#include "g_local.h"
#include "ai_local.h"

//ACE
ai_ability_t	AIAbilities[MAX_ABILITIES];//GHz
ai_weapon_t		AIWeapons[WEAP_TOTAL];
int	num_AIEnemies;
edict_t *AIEnemies[MAX_EDICTS];		// pointers to all players in the game

//==========================================
// Some CTF stuff
//==========================================
static gitem_t *redflag;
static gitem_t *blueflag;

//GHz: turn the bot away from current angles to avoid obstruction
void BOT_DMClass_TurnAway(edict_t* self)
{
	vec3_t angles, forward;

	//AI_DebugPrintf("turn away from obstruction\n");
	VectorCopy(self->s.angles, angles);
	angles[YAW] += random() * 180 - 90;
	AngleVectors(angles, forward, NULL, NULL);
	VectorCopy(forward, self->ai.move_vector);
	AI_ChangeAngle(self);
	self->monsterinfo.bump_delay = level.time + GetRandom(2, 5) * FRAMETIME;
}

//GHz: returns the velocity of the bot in a forward direction
float AI_ForwardVelocity(edict_t* self)
{
	vec3_t forward;

	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorNormalize(forward);
	return DotProduct(forward, self->velocity);
}

// returns dot product value (ranging from 1 to -1) by comparing move_vec to a vector pointing to the right of the bot
// a value of > 0 indicates move_vector points to the right of the bot, < 0 is to the left, 0 is parallel and pointing in the same direction, -1/1 is parallel in the opposite direction
// note: it's important to understand that the result indicates the relative direction the vectors are POINTING, and has nothing to do with their positions
float AI_MoveRight(edict_t* self, vec3_t move_vec)
{
	vec3_t	vec;
	float	dot, value;
	vec3_t	right;

	AngleVectors(self->s.angles, NULL, right, NULL);
	VectorNormalize(right);
	VectorCopy(move_vec, vec);
	VectorNormalize(vec);
	dot = DotProduct(right, vec);
	if (dot >= 0)
	{
		//gi.dprintf("AI_MoveRight: %f: move_vec points to the right of bot -->\n", dot);
		//return true;
	}
	else
	{
		//gi.dprintf("AI_MoveRight: %f: move_vec points to the left of bot <--\n", dot);
		//return false;
	}
	return dot;
}

// return true if move_vector is in front (forward) of the bot
qboolean AI_MoveForward(edict_t* self)
{
	vec3_t	vec;
	float	dot, value;
	vec3_t	forward;

	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorCopy(self->ai.move_vector, vec);
	VectorNormalize(vec);
	dot = DotProduct(forward, vec);
	if (dot >= 0)
		return true;
	return false;
}

// return true if move_vector is above the bot
qboolean AI_MoveUp(edict_t* self)
{
	vec3_t	vec;
	float	dot, value;
	vec3_t	up;

	AngleVectors(self->s.angles, NULL, NULL, up);
	VectorCopy(self->ai.move_vector, vec);
	VectorNormalize(vec);
	dot = DotProduct(up, vec);
	if (dot >= 0)
		return true;
	return false;
}

qboolean AI_IsProjectile(edict_t* ent);
// moves the bot towards move_vector using ucmds, returns true if the bot was able to move
qboolean BOT_DMclass_Ucmd_Move(edict_t* self, float movespeed, usercmd_t* ucmd, qboolean move_vertical, qboolean check_move)
{
	qboolean moved = false;
	vec3_t	move_vec;

	//if (VectorEmpty(self->ai.link_vector))
		VectorCopy(self->ai.move_vector, move_vec);
	//else
	//	VectorCopy(self->ai.link_vector, move_vec);

	if (move_vertical)
	{
		if (AI_MoveUp(self))
			ucmd->upmove = movespeed;
		else
			ucmd->upmove = -movespeed;
		moved = true;
	}

	if (AI_MoveForward(self))
	{
		if (!check_move || AI_CanMove(self, BOT_MOVE_FORWARD))
		{
			ucmd->forwardmove = movespeed;
			moved = true;
		}
	}
	else
	{
		if (!check_move || AI_CanMove(self, BOT_MOVE_BACK))
		{
			ucmd->forwardmove = -movespeed;
			moved = true;
		}
	}
	if (AI_MoveRight(self, move_vec) >= 0)
	{
		if (!check_move || AI_CanMove(self, BOT_MOVE_RIGHT))
		{
			ucmd->sidemove = movespeed;
			moved = true;
		}
	}
	else
	{
		if (!check_move || AI_CanMove(self, BOT_MOVE_LEFT))
		{
			ucmd->sidemove = -movespeed;
			moved = true;
		}
	}
	return moved;
}

void BOT_DMclass_AvoidDrowning(edict_t* self, usercmd_t *ucmd)
{
	// we're 3 seconds away from drowning--move up!
	if (level.time + 3.0 > self->air_finished)
		ucmd->upmove = 400;
}

void BOT_DMclass_AvoidObstacles(edict_t* self, usercmd_t* ucmd, int current_node_flags)
{
	if (random() > 0.1 && AI_SpecialMove(self, ucmd)) //jumps, crouches, turns...
		return;
	if (random() > 0.6 && current_node_flags & NODEFLAGS_PLATFORM) //GHz: standing on a platform--if we can't move, maybe it's for a reason?
	{
		self->ai.next_move_time = level.time + (GetRandom(2, 5) * FRAMETIME);
		return;
	}
	BOT_DMClass_TurnAway(self);
	ucmd->forwardmove = 400;
}

void BOT_DMclass_BunnyHop(edict_t* self, usercmd_t* ucmd, qboolean forwardmove);
//==========================================
// BOT_DMclass_MoveAttack
// derivative of BOT_DMclass_Move that allows the bot to move along a path (mostly) independent of view angles
// this allows the bot to attack an enemy while following a path to it (or another goal), allowing it to navigate around obstructions
//==========================================
void BOT_DMclass_MoveAttack(edict_t* self, usercmd_t* ucmd)
{
	int current_node_flags = 0;
	int next_node_flags = 0;
	int	current_link_type = 0;
	int i;

	//AI_DebugPrintf("BOT_DMclass_MoveAttack()\n");

	if (self->ai.next_move_time > level.time)
		return;//GHz

	current_node_flags = nodes[self->ai.current_node].flags;
	next_node_flags = nodes[self->ai.next_node].flags;

	//GHz: bot was stuck, so give it time to turn and move away from the obstruction
	if (self->monsterinfo.bump_delay > level.time)
	{
		//current_link_type = AI_PlinkMoveType(self->ai.current_node, self->ai.next_node);
		//AI_DebugPrintf("BUMP AROUND : % d(% s) -> % s -> % d(% s)\n",
		//	self->ai.current_node, AI_NodeString(current_node_flags),
		//	AI_LinkString(current_link_type), self->ai.next_node, AI_NodeString(next_node_flags));//GHz
		//if (!(current_node_flags & NODEFLAGS_PLATFORM))
		ucmd->forwardmove = 400;
		return;
	}

	// is there a path link between the current node we're on to the next one in the chain?
	if (AI_PlinkExists(self->ai.current_node, self->ai.next_node))
	{
		current_link_type = AI_PlinkMoveType(self->ai.current_node, self->ai.next_node);

		if (AIDevel.debugChased && current_link_type != self->ai.linktype)//GHz
			safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: CURRENT MOVE: %d (%s) -> %s -> %d (%s)\n",
				self->ai.pers.netname, self->ai.current_node, AI_NodeString(current_node_flags),
				AI_LinkString(current_link_type), self->ai.next_node, AI_NodeString(next_node_flags));//GHz
		//if (current_link_type && current_link_type != LINK_INVALID)//GHz
		self->ai.linktype = current_link_type;//GHz
	}

	// Platforms
	if (current_link_type == LINK_PLATFORM) // currently riding a platform
	{
		// Move to the center
		self->ai.move_vector[2] = 0; // kill z movement
		if (VectorLength(self->ai.move_vector) > 10)
			BOT_DMclass_Ucmd_Move(self, 200, ucmd, false, false);
			//ucmd->forwardmove = 200; // walk to center

		//AI_ChangeAngle(self);

		return; // No move, riding elevator
	}
	else if (next_node_flags & NODEFLAGS_PLATFORM) // the next node is a platform
	{
		//gi.dprintf("next node is a platform\n");
		// is lift down?
		for (i = 0;i < nav.num_ents;i++) {
			if (nav.ents[i].node == self->ai.next_node)
			{
				//gi.dprintf("found next node entity %s\n", nav.ents[i].ent->classname);
				//testing line
				//vec3_t	tPoint;
				//int		j;
				//for(j=0; j<3; j++)//center of the ent
				//	tPoint[j] = nav.ents[i].ent->s.origin[j] + 0.5*(nav.ents[i].ent->mins[j] + nav.ents[i].ent->maxs[j]);
				//tPoint[2] = nav.ents[i].ent->s.origin[2] + nav.ents[i].ent->maxs[2];
				//tPoint[2] += 8;
				//AITools_DrawLine( self->s.origin, tPoint );

				//if not reachable, wait for it (only height matters)
				/*
				if( (nav.ents[i].ent->s.origin[2] + nav.ents[i].ent->maxs[2])
					> (self->s.origin[2] + self->mins[2] + AI_JUMPABLE_HEIGHT) )
				{
					gi.dprintf("waiting for platform to arrive...\n");
					return; //wait for elevator
				}
				*/
				if (nav.ents[i].ent->moveinfo.state != STATE_BOTTOM)//GHz
				{
					//gi.dprintf("waiting for platform. state %d dist %f height %f\n", 
					//	nav.ents[i].ent->moveinfo.state, nav.ents[i].ent->moveinfo.remaining_distance, nav.ents[i].ent->s.origin[2]);
					return; //wait for it
				}
			}
		}
	}

	// Ladder movement
	if (self->ai.is_ladder) // climbing ladder
	{
		ucmd->forwardmove = 70;
		ucmd->upmove = 200;
		ucmd->sidemove = 0;
		return;
	}

	// Falling off ledge
	if (!self->groundentity && !self->ai.is_step && !self->ai.is_swim)
	{
		//gi.dprintf("falling off a ledge...\n");
		AI_ChangeAngle(self);
		if (current_link_type == LINK_JUMPPAD) {
			//gi.dprintf("walk forward\n");
			BOT_DMclass_Ucmd_Move(self, 100, ucmd, false, false);
			//ucmd->forwardmove = 100; // move forward slowly
		}
		else if (current_link_type == LINK_JUMP) {
			self->velocity[0] = self->ai.move_vector[0] * 280; // adjust x/y velocity to push us (rapidly) towards our move vector
			self->velocity[1] = self->ai.move_vector[1] * 280;
			//gi.dprintf("jump forward?\n");
		}
		else {
			self->velocity[0] = self->ai.move_vector[0] * 160; // same as above, but slower
			self->velocity[1] = self->ai.move_vector[1] * 160;
			//gi.dprintf("jump back?\n");
		}
		return;
	}

	// jumping over (keep fall before this)
	if (current_link_type == LINK_JUMP && self->groundentity) // feet are still on the ground, but we're supposed to jump somewhere along this path link
	{
		trace_t trace;
		vec3_t  v1, v2;
		//gi.dprintf("jump over...\n");
		//check floor in front, if there's none... Jump!
		VectorCopy(self->s.origin, v1);
		VectorCopy(self->ai.move_vector, v2);
		VectorNormalize(v2);
		VectorMA(v1, 12, v2, v1);
		v1[2] += self->mins[2];
		trace = gi.trace(v1, tv(-2, -2, -AI_JUMPABLE_HEIGHT), tv(2, 2, 0), v1, self, MASK_AISOLID);
		if (!trace.startsolid && trace.fraction == 1.0)
		{
			//gi.dprintf("jump!\n");
			//jump!
			BOT_DMclass_Ucmd_Move(self, 400, ucmd, false, false);
			//ucmd->forwardmove = 400;
			//prevent double jumping on crates
			VectorCopy(self->s.origin, v1);
			v1[2] += self->mins[2];
			trace = gi.trace(v1, tv(-12, -12, -8), tv(12, 12, 0), v1, self, MASK_AISOLID);
			if (trace.startsolid)
				ucmd->upmove = 400;
			return;
		}
	}

	// Move To Short Range goal (not following paths)
	// plats, grapple, etc have higher priority than SR Goals, cause the bot will 
	// drop from them and have to repeat the process from the beginning
	if (AI_MoveToGoalEntity(self, ucmd))
	{
		//AI_DebugPrintf("move to short range goal\n");
		return;
	}

	// swimming
	if (self->ai.is_swim)
	{
		// We need to be pointed up/down
		//AI_ChangeAngle(self);
		
		if (!(gi.pointcontents(nodes[self->ai.next_node].origin) & MASK_WATER)) // Exit water
			ucmd->upmove = 400;
		else
			BOT_DMclass_AvoidDrowning(self, ucmd);//GHz: exit water if we're running out of air

		BOT_DMclass_Ucmd_Move(self, 300, ucmd, true, false);
		//ucmd->forwardmove = 300;
		return;
	}

	// Check to see if stuck, and if so try to free us
	if (VectorLength(self->velocity) < 37)
	{
		//AI_DebugPrintf("STUCK : % d(% s) -> % s -> % d(% s)\n",
		//	self->ai.current_node, AI_NodeString(current_node_flags),
		//	AI_LinkString(current_link_type), self->ai.next_node, AI_NodeString(next_node_flags));//GHz
		BOT_DMclass_AvoidObstacles(self, ucmd, current_node_flags);
		return;
	}


	//AI_ChangeAngle(self);

	// Otherwise move as fast as we can... 
	//ucmd->forwardmove = 400;
	BOT_DMclass_Ucmd_Move(self, 400, ucmd, false, false);
	//BOT_DMclass_BunnyHop(self, ucmd, true);
}

//==========================================
// BOT_DMclass_Move
// DMClass is generic bot class
// GHz: IMPORTANT: any changes made here should (most likely) be duplicated in BOT_DMclass_MoveAttack()
//==========================================
void BOT_DMclass_Move(edict_t *self, usercmd_t *ucmd)
{
	int current_node_flags = 0;
	int next_node_flags = 0;
	int	current_link_type = 0;
	int i;

	//AI_DebugPrintf("BOT_DMclass_Move()\n");

	if (self->ai.next_move_time > level.time)
		return;//GHz

	current_node_flags = nodes[self->ai.current_node].flags;
	next_node_flags = nodes[self->ai.next_node].flags;

	//GHz: bot was stuck, so give it time to turn and move away from the obstruction
	if (self->monsterinfo.bump_delay > level.time)
	{
		//current_link_type = AI_PlinkMoveType(self->ai.current_node, self->ai.next_node);
		//AI_DebugPrintf("BUMP AROUND : % d(% s) -> % s -> % d(% s)\n",
		//	self->ai.current_node, AI_NodeString(current_node_flags),
		//	AI_LinkString(current_link_type), self->ai.next_node, AI_NodeString(next_node_flags));//GHz
		//if (!(current_node_flags & NODEFLAGS_PLATFORM))
			ucmd->forwardmove = 400;
		return;
	}

	// is there a path link between the current node we're on to the next one in the chain?
	if( AI_PlinkExists( self->ai.current_node, self->ai.next_node ))
	{
		current_link_type = AI_PlinkMoveType( self->ai.current_node, self->ai.next_node );

		if (AIDevel.debugChased && current_link_type != self->ai.linktype)//GHz
			safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: CURRENT MOVE: %d (%s) -> %s -> %d (%s)\n", 
				self->ai.pers.netname, self->ai.current_node, AI_NodeString(current_node_flags), 
				AI_LinkString(current_link_type), self->ai.next_node, AI_NodeString(next_node_flags));//GHz
		//if (current_link_type && current_link_type != LINK_INVALID)//GHz
		self->ai.linktype = current_link_type;//GHz
	}

	// Platforms
	if( current_link_type == LINK_PLATFORM ) // currently riding a platform
	{
		//gi.dprintf("currently riding a platform...\n");
		// Move to the center
		self->ai.move_vector[2] = 0; // kill z movement
		if(VectorLength(self->ai.move_vector) > 10)
			ucmd->forwardmove = 200; // walk to center

		AI_ChangeAngle(self);

		return; // No move, riding elevator
	}
	else if( next_node_flags & NODEFLAGS_PLATFORM ) // the next node is a platform
	{
		//gi.dprintf("next node is a platform\n");
		// is lift down?
		for(i=0;i<nav.num_ents;i++){
			if( nav.ents[i].node == self->ai.next_node )
			{
				//gi.dprintf("found next node entity %s\n", nav.ents[i].ent->classname);
				//testing line
				//vec3_t	tPoint;
				//int		j;
				//for(j=0; j<3; j++)//center of the ent
				//	tPoint[j] = nav.ents[i].ent->s.origin[j] + 0.5*(nav.ents[i].ent->mins[j] + nav.ents[i].ent->maxs[j]);
				//tPoint[2] = nav.ents[i].ent->s.origin[2] + nav.ents[i].ent->maxs[2];
				//tPoint[2] += 8;
				//AITools_DrawLine( self->s.origin, tPoint );

				//if not reachable, wait for it (only height matters)
				/*
				if( (nav.ents[i].ent->s.origin[2] + nav.ents[i].ent->maxs[2])
					> (self->s.origin[2] + self->mins[2] + AI_JUMPABLE_HEIGHT) )
				{
					gi.dprintf("waiting for platform to arrive...\n");
					return; //wait for elevator
				}
				*/
				if (nav.ents[i].ent->moveinfo.state != STATE_BOTTOM)//GHz
				{
					//gi.dprintf("waiting for platform. state %d dist %f height %f\n", 
					//	nav.ents[i].ent->moveinfo.state, nav.ents[i].ent->moveinfo.remaining_distance, nav.ents[i].ent->s.origin[2]);
					return; //wait for it
				}
			}
		}
	}

	// Ladder movement
	if( self->ai.is_ladder )
	{
		ucmd->forwardmove = 70;
		ucmd->upmove = 200;
		ucmd->sidemove = 0;
		return;
	}

	// Falling off ledge
	if(!self->groundentity && !self->ai.is_step && !self->ai.is_swim && !self->ai.is_bunnyhop)//GHz: don't adjust velocity if we're bunnyhopping!
	{
		//gi.dprintf("%d falling off a ledge...\n", level.framenum);
		AI_ChangeAngle(self);
		if (current_link_type == LINK_JUMPPAD ) {
			//gi.dprintf("walk forward\n");
			ucmd->forwardmove = 100; // move forward slowly
		} else if( current_link_type == LINK_JUMP ) {
			self->velocity[0] = self->ai.move_vector[0] * 280; // adjust x/y velocity to push us (rapidly) towards our move vector
			self->velocity[1] = self->ai.move_vector[1] * 280;
			//gi.dprintf("jump forward?\n");
		} else {
			self->velocity[0] = self->ai.move_vector[0] * 160; // same as above, but slower
			self->velocity[1] = self->ai.move_vector[1] * 160;
			//gi.dprintf("jump back?\n");
		}
		return;
	}

	// jumping over (keep fall before this)
	if( current_link_type == LINK_JUMP && self->groundentity) // feet are still on the ground, but we're supposed to jump somewhere along this path link
	{
		trace_t trace;
		vec3_t  v1, v2;
		//gi.dprintf("jump over...\n");
		//check floor in front, if there's none... Jump!
		VectorCopy( self->s.origin, v1 );
		VectorCopy( self->ai.move_vector, v2 );
		VectorNormalize( v2 );
		VectorMA( v1, 12, v2, v1 );
		v1[2] += self->mins[2];
		trace = gi.trace( v1, tv(-2, -2, -AI_JUMPABLE_HEIGHT), tv(2, 2, 0), v1, self, MASK_AISOLID );
		if( !trace.startsolid && trace.fraction == 1.0 )
		{
			//gi.dprintf("jump!\n");
			//jump!
			ucmd->forwardmove = 400;
			//prevent double jumping on crates
			VectorCopy( self->s.origin, v1 );
			v1[2] += self->mins[2];
			trace = gi.trace( v1, tv(-12, -12, -8), tv(12, 12, 0), v1, self, MASK_AISOLID );
			if( trace.startsolid )
				ucmd->upmove = 400;
			return;
		}
	}

	// Move To Short Range goal (not following paths)
	// plats, grapple, etc have higher priority than SR Goals, cause the bot will 
	// drop from them and have to repeat the process from the beginning
	if (AI_MoveToGoalEntity(self, ucmd))
	{
		//AI_DebugPrintf("move to short range goal\n");
		return;
	}

	// swimming
	if( self->ai.is_swim )
	{
		// We need to be pointed up/down
		AI_ChangeAngle(self);

		if( !(gi.pointcontents(nodes[self->ai.next_node].origin) & MASK_WATER) ) // Exit water
			ucmd->upmove = 400;
		
		ucmd->forwardmove = 300;
		return;
	}

	// Check to see if stuck, and if so try to free us
	if (VectorLength(self->velocity) < 37)
	{
		//AI_DebugPrintf("STUCK : % d(% s) -> % s -> % d(% s)\n",
		//	self->ai.current_node, AI_NodeString(current_node_flags),
		//	AI_LinkString(current_link_type), self->ai.next_node, AI_NodeString(next_node_flags));//GHz
		BOT_DMclass_AvoidObstacles(self, ucmd, current_node_flags);
		return;
	}

	AI_ChangeAngle(self);

	// Otherwise move as fast as we can... 
	ucmd->forwardmove = 400;
	if (current_link_type == LINK_MOVE || current_link_type == LINK_STAIRS)
		BOT_DMclass_BunnyHop(self, ucmd, false);
}

//==========================================
// BOT_DMclass_Wander
// Wandering code (based on old ACE movement code) 
//==========================================
void BOT_DMclass_Wander(edict_t *self, usercmd_t *ucmd)
{
	vec3_t  temp;

	// Do not move
	if(self->ai.next_move_time > level.time)
		return;

	if (self->deadflag)
		return;

	//AI_DebugPrintf("BOT_DMclass_Wander()\n");

	//GHz: bot was stuck, so give it time to turn and move away from the obstruction
	if (self->monsterinfo.bump_delay > level.time)
	{
		ucmd->forwardmove = 400;
		return;
	}

	// Special check for elevators, stand still until the ride comes to a complete stop.
	if(self->groundentity != NULL && self->groundentity->use == Use_Plat)
	{
		if(self->groundentity->moveinfo.state == STATE_UP ||
		   self->groundentity->moveinfo.state == STATE_DOWN)
		{
			self->velocity[0] = 0;
			self->velocity[1] = 0;
			self->velocity[2] = 0;
			self->ai.next_move_time = level.time + 0.5;
			return;
		}
	}

	// Move To Goal (Short Range Goal, not following paths)
	if (AI_MoveToGoalEntity(self,ucmd))
		return;
	
	// Swimming?
	VectorCopy(self->s.origin,temp);
	temp[2]+=24;

//	if(trap_PointContents (temp) & MASK_WATER)
	if( gi.pointcontents (temp) & MASK_WATER)
	{
		// If drowning and no node, move up
		if( self->client && self->client->next_drown_time > 0 )	//jalfixme: client references must pass into botStatus
		{
			ucmd->upmove = 100;
			self->s.angles[PITCH] = -45;
		}
		else
			ucmd->upmove = 15;

		ucmd->forwardmove = 300;
	}
	// else self->client->next_drown_time = 0; // probably shound not be messing with this, but


	// Lava?
	temp[2]-=48;
	//if(trap_PointContents(temp) & (CONTENTS_LAVA|CONTENTS_SLIME))
	if( gi.pointcontents(temp) & (CONTENTS_LAVA|CONTENTS_SLIME) )
	{
		self->s.angles[YAW] += random() * 360 - 180;
		ucmd->forwardmove = 400;
		if(self->groundentity)
			ucmd->upmove = 400;
		else
			ucmd->upmove = 0;
		return;
	}


	// Check for special movement
 	if(VectorLength(self->velocity) < 37)
	{
		//AI_DebugPrintf("wandering stuck\n");
		if(random() > 0.1 && AI_SpecialMove(self,ucmd))	//jumps, crouches, turns...
			return;
		BOT_DMClass_TurnAway(self);
		//self->s.angles[YAW] += random() * 180 - 90;

		if (!self->ai.is_step)// if there is ground continue otherwise wait for next move
			AI_ResetNavigation(self);
		else if( AI_CanMove( self, BOT_MOVE_FORWARD))
			ucmd->forwardmove = 400;

		return;
	}


	// Otherwise move slowly, walking wondering what's going on
	if (AI_CanMove(self, BOT_MOVE_FORWARD))
	{
		//gi.dprintf("move forward\n");
		ucmd->forwardmove = 400;
	}
	else
	{
		BOT_DMClass_TurnAway(self);
		//gi.dprintf("move backward\n");
		//AI_PickLongRangeGoal( self ); //GHz: commented out because we wouldn't be here if we weren't supposed to wander!
		//ucmd->forwardmove = -400;
	}
}

void BOT_DMclass_FireWeapon(edict_t* self, usercmd_t* ucmd);
void BOT_DMclass_ChooseWeapon(edict_t* self);
qboolean BOT_DMclass_RunAway(edict_t* self, qboolean moveattack, usercmd_t *ucmd)
{
	int current_node, goal_node;

	if (self->ai.attack_delay > level.time)// || self->ai.evade_delay > level.time)
	{
		//gi.dprintf("already running away\n");
		return true;
	}

	if (self->ai.evade_delay > level.time)
	{
		//gi.dprintf("already evading\n");
		return false;
	}
	if (random() < 0.5)
	{
		//gi.dprintf("random fail to evade\n");
		return false;
	}
	//if ((self->ai.state == BOT_STATE_MOVEATTACK || self->ai.state == BOT_STATE_MOVE) && self->ai.goal_node)
	//	return true;

	// attempt to find starting node
	if ((current_node = AI_FindClosestReachableNode(self->s.origin, self, 2 * NODE_DENSITY, NODE_ALL)) == -1)
	{
		//gi.dprintf("no closest node\n");
		return false;
	}

	self->ai.current_node = current_node;

	// attempt to find ending node nearby that the enemy can't see
	if ((goal_node = AI_FindFarthestHiddenNode(self, 1024, NODE_ALL)) == -1)
	{
		//gi.dprintf("couldn't find a hidden node\n");
		return false;
	}

	//set up the goal
	if (moveattack)
	{
		self->ai.state = BOT_STATE_MOVEATTACK;
		//BOT_DMclass_ChooseWeapon(self); // chose the best weapon for the job
		//BOT_DMclass_FireWeapon(self, ucmd); // fire!
		self->ai.evade_delay = level.time + 15.0;// don't try to evade again for awhile
		//gi.dprintf("SUCCESS! bot will evade the enemy!\n");
	}
	else
	{
		self->ai.state = BOT_STATE_MOVE;
		self->ai.attack_delay = level.time + 5.0;// we're trying to run away, so don't try to attack for a bit
	}
	self->ai.tries = 0;	// Reset the count of how many times we tried this goal

	if (AIDevel.debugChased && bot_showlrgoal->value)
		safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: is trying to evade towards node %d!\n", self->ai.pers.netname, goal_node);

	AI_SetGoal(self, goal_node, true);
	//self->ai.state_combat_timeout = level.time + 1.0;//GHz: timeout for moveattack after it loses sight of enemy
	
	if (moveattack)
		return false;
	return true;
}

// tells the bot to go find its nearest summoned entity (for protection)
qboolean BOT_DMclass_FindSummons(edict_t* self, qboolean moveattack, usercmd_t* ucmd)
{
	int current_node, goal_node;
	edict_t* e = NULL;

	//gi.dprintf("BOT_DMclass_FindSummons()\n");

	// already evading, don't try to attack for awhile
	if (self->ai.attack_delay > level.time)
	{
		//gi.dprintf("already evading\n");
		return true;
	}
	// already evading, but continue attacking anyway
	if (self->ai.evade_delay > level.time)
	{
		//gi.dprintf("already moving to summons\n");
		return false;
	}
	// random chance to fail to evade
	if (random() < 0.5)
		return false;

	// attempt to find starting node
	if ((current_node = AI_FindClosestReachableNode(self->s.origin, self, 2 * NODE_DENSITY, NODE_ALL)) == -1)
		return false;
	//gi.dprintf("trying to find summons\n");
	self->ai.current_node = current_node;

	// find a summons that places us farthest away from danger
	float farthest_dist = entdist(self, self->enemy);
	edict_t *farthest_ent = NULL;
	for (e = g_edicts; e < &g_edicts[globals.num_edicts]; e++)
	{
		// sanity checks
		if (!e->inuse)
			continue;
		if (e->solid == SOLID_NOT)
			continue;
		if (!e->classname)
			continue;
		// is this a summoned entity that the bot owns?
		if (!AI_IsOwnedSummons(self, e))
			continue;

		float dist;
		// if the summons has an enemy, calculate distance to its enemy
		if (e->enemy && e->enemy->inuse)
			dist = entdist(e->enemy, e);
		// otherwise, calculate distance between it and our enemy
		else
			dist = entdist(self->enemy, e);
		// would moving closer to our summons place us further away from danger?
		if (dist > farthest_dist)
		{
			farthest_dist = dist;
			farthest_ent = e;
		}
	}

	// either we don't have any summons or moving closer to them would place us closer to the enemy
	if (!farthest_ent)
		return false;

	// attempt to find a node closest to the summoned entity
	if ((goal_node = AI_FindClosestReachableNode(farthest_ent->s.origin, farthest_ent, NODE_DENSITY, NODE_ALL)) == -1)
		return false;

	//set up the goal
	if (moveattack)
	{
		// continue to fight the enemy while moving toward our summons
		self->ai.state = BOT_STATE_MOVEATTACK;
		self->ai.evade_delay = level.time + 15.0;// don't try to evade again for awhile
		//gi.dprintf("%d: ** %s: bot is evading toward summons! **\n", (int)level.framenum, __func__);
	}
	else
	{
		self->ai.state = BOT_STATE_MOVE;
		self->ai.attack_delay = level.time + 5.0;// we're trying to run away, so don't try to attack for a bit
	}
	self->ai.tries = 0;	// Reset the count of how many times we tried this goal

	if (AIDevel.debugChased && bot_showlrgoal->value)
		safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: is evading to find summons at node %d!\n", self->ai.pers.netname, goal_node);

	AI_SetGoal(self, goal_node, true);
	self->goalentity = farthest_ent; // used in BOT_DMclassEvadeEnemy
	//self->ai.state_combat_timeout = level.time + 1.0;//GHz: timeout for moveattack after it loses sight of enemy

	if (moveattack)
		return false;
	return true;
}

qboolean BOT_DMclassEvadeEnemy(edict_t* self, usercmd_t *ucmd)
{
	// moving toward one of our summons?
	edict_t* goalent = NULL;
	if (self->movetarget && self->movetarget->inuse) // SR goal
		goalent = self->movetarget;
	else if (self->goalentity && self->goalentity->inuse) // LR goal
		goalent = self->goalentity;
	if (goalent && AI_IsOwnedSummons(self, goalent))
		return true;

	// do we have summons? if so, find the closest one and move to it!(LR goal)
	if (AI_NumSummons(self) > 0)
		return BOT_DMclass_FindSummons(self, true, ucmd);

	// low health?
	if (self->health > 0.4 * self->max_health)
		return false;

	// try to tball away
	if (BOT_DMclass_UseTball(self, true))
		return true;
	
	return BOT_DMclass_RunAway(self, true, ucmd);
}

void BOT_DMclassJumpAttack(edict_t* self, usercmd_t* ucmd, float ideal_range)
{
	// we aren't on the ground or we're swimming
	if (!self->groundentity || self->waterlevel > 2)
		return;
	// enemy is on the ground or swimming
	if (self->enemy->groundentity || self->enemy->waterlevel > 2)
		return;
	// ignoring Z height, enemy is out of range
	if (Get2dDistance(self->s.origin, self->enemy->s.origin) > ideal_range)
		return;
	// enemy isn't over our head
	if (self->enemy->absmin[2] < self->absmax[2])
		return;
	//gi.dprintf("try jumpattack?\n");
	// jump if it gets us close enough to attack
	if (self->s.origin[2] + self->viewheight + ideal_range + AI_JUMPABLE_HEIGHT >= self->enemy->absmin[2])
	{
		//gi.dprintf("***JUMP ATTACK****\n");
		ucmd->upmove = 400;
	}
}

//float vectoanglediff2(vec3_t v1, vec3_t v2);
void BOT_DMclass_BunnyHop(edict_t* self, usercmd_t* ucmd, qboolean forwardmove)
{
	float fwd_spd, mv_spd, dot;
	vec3_t move_vec, link_vec, vel;
	qboolean straight_path;

	if (self->groundentity)// on the ground? reset state--we haven't decided yet if the bot should bunnyhop again
		self->ai.is_bunnyhop = false;

	// vector pointing in the direction we should be moving
	VectorCopy(self->ai.move_vector, move_vec);
	move_vec[2] = 0;
	VectorNormalize(move_vec);

	// vector pointing in the direction we're travelling
	VectorCopy(self->velocity, vel);
	vel[2] = 0;
	VectorNormalize(vel);

	// not following a path?
	if (VectorEmpty(self->ai.link_vector))
	{
		// airborne? randomly strafe left/right to build momentum
		if (!self->groundentity)
		{
			if (random() < 0.5)
				ucmd->sidemove = 400;
			else
				ucmd->sidemove = -400;
			return; // nothing else to do until we land
		}
		// note: we're using fabs() to handle cases where the bot is jumping backwards
		dot = fabsf(DotProduct(move_vec, vel)); // returns 1 if velocity is parallel and in the same direction as move_vec
		mv_spd = fabsf(DotProduct(move_vec, self->velocity));// speed in the direction of our goal
		//gi.dprintf("dot:%f mv_spd:%f fwd_spd:%f\n", dot, mv_spd, AI_ForwardVelocity(self));
		if (dot > 0.9 && mv_spd > 280)
		{
			ucmd->upmove = 400;
			self->ai.is_bunnyhop = true;
		}
		return;
	}
	else
		VectorCopy(self->ai.link_vector, link_vec); // vector from current node to next node
	link_vec[2] = 0;
	VectorNormalize(link_vec);
	

	//if (self->groundentity) 
	//	self->ai.is_bunnyhop = false;
	if (!self->groundentity) // steer the bot towards move_vector to stay on-course
	{
		if (forwardmove)
		{
			// move along x-y axis while staying close to move_vector
			BOT_DMclass_Ucmd_Move(self, 400, ucmd, false, false);
		}
		else
		{
			//vec3_t cross;
			//CrossProduct(move_vec, vel, cross);
			//gi.dprintf("cross: [0]%f [1]%f [2]%f\n", cross[0], cross[1], cross[2]);
			//gi.dprintf("angle diff:%f\n", vectoanglediff2(self->ai.move_vector, move_vec));
			//if (toright(self, nodes[self->ai.current_node].origin))
			//	gi.dprintf("current node is to the RIGHT");
			//else
			//	gi.dprintf("current node is to the LEFT");
			// need to strafe side-to-side while airborne to pick up extra speed, regardless of viewing angles
			if (AI_MoveRight(self, link_vec) > 0)
			{
				//gi.dprintf(", so move LEFT!\n");
				ucmd->sidemove = -400;// viewing angles/vector points left of move_vec (move_vec is right of view), i.e. we've swung too far right of path, so compensate by moving left
			}
			else
			{
				//gi.dprintf(", so move RIGHT!\n");
				ucmd->sidemove = 400;
			}
		}
		return;// nothing else to do until we touch the ground again
	}

	// can't bunnyhop in the water!
	if (self->waterlevel > 2)
		return;

	//gi.dprintf("trying to bunnyhop while following a path...\n");
	//VectorCopy(self->ai.move_vector, move_vec);//FIXME: testing--this should become a parameter
	//move_vec[2] = 0;// we only care about x-y values
	fwd_spd = AI_ForwardVelocity(self); // speed in the direction we are facing
	
	mv_spd = DotProduct(link_vec, self->velocity);// speed in the direction of our goal

	//dot = DotProduct(move_vec, vel); // returns 1 if velocity is parallel and in the same direction as move_vec
	dot = DotProduct(link_vec, move_vec); // returns 1 if the direction we're trying to move in aligns (is parallel) with the direction of our goal

	// note: AI_JUMPABLE_DISTANCE is very conservative and likely assumes no running start and no strafe jumping, hence the extra distance added (200+ is common)
	// this distance keeps the bot from potentially overshooting the path, forcing it to turn around and get back on-track
	straight_path = AI_StraightPath(self, AI_JUMPABLE_DISTANCE + 60, 0.8); // returns 1 if the nodes ahead of us are (mostly) in a straight line

	// is the bot moving towards the path?
	if (dot < 0.95) // 1: vectors pointing in the same direction, 0: vectors are perpendicular, -1: vectors are pointing in the opposite direction
		return;
	// is the bot going fast enough towards the path?
	if (mv_spd <= 280)
		return;
	// is the path ahead mostly straight?
	if (!straight_path)
		return;

	//gi.dprintf("%d: ", level.framenum);
	//gi.dprintf("bunnyhop fwd_spd: %f mv_spd: %f dot: %f", fwd_spd, mv_spd, dot);

	//if (self->ai.is_step)
	//	gi.dprintf("[step:true]");
	//else
	//	gi.dprintf("[step:false]");

	// velocity needs to be mostly parallel with move_vec and speed needs to be 280+
	//if (dot > 0.9)
	//	gi.dprintf("[parallel: OK!]");
	//else
	//	gi.dprintf("[parallel: fail]");
	//if (mv_spd > 280)
	//	gi.dprintf("[speed: OK!]");
	//else
	//	gi.dprintf("[speed: fail]");
	//if (straight_path)
	//	gi.dprintf("[straight:TRUE]\n");
	//else
	//	gi.dprintf("[straight:FALSE]\n");

	self->ai.is_bunnyhop = true;

	//if (self->groundentity)
	//{
		//VectorCopy(self->s.origin, self->monsterinfo.spot1);//GHz: for testing to determine maximum jump distance
		// jump immediately after touching down in order to maintain momentum
		ucmd->upmove = 400;
	//}
	
	
}

//==========================================
// BOT_DMclass_CombatMovement
//
// NOTE: Very simple for now, just a basic move about avoidance.
//       Change this routine for more advanced attack movement.
//==========================================
void BOT_DMclass_CombatMovement( edict_t *self, usercmd_t *ucmd )
{
	int		weapon;
	float	c;
	vec3_t	attackvector;
	float	dist, ideal, forward_speed;

	//jalToDo: Convert CombatMovement to a BOT_STATE, allowing
	//it to dodge, but still follow paths, chasing enemy or
	//running away... hmmm... maybe it will need 2 different BOT_STATEs

	//AI_DebugPrintf("BOT_DMclass_CombatMovement()\n");

	if(!self->enemy) {

		//do whatever (tmp move wander)
		if( AI_FollowPath(self) )
			BOT_DMclass_Move(self, ucmd);
		return;
	}

	// Move To Short Range goal (not following paths)
	if (AI_MoveToGoalEntity(self, ucmd))
	{
		//AI_DebugPrintf("move to short range goal\n");
		return;
	}

	// determine weapmodel index for equipped weapon
	if (self->client->pers.weapon)
		weapon = (self->client->pers.weapon->weapmodel & 0xff);
	else
		weapon = 0;
	// calculate aiming vector to enemy
	//VectorSubtract( self->s.origin, self->enemy->s.origin, attackvector);
	VectorSubtract(self->enemy->s.origin, self->s.origin, attackvector);
	// calculate enemy distance
	dist = VectorLength( attackvector);
	// get ideal range of equipped weapon
	ideal = AIWeapons[weapon].idealRange;
	if (ideal > AI_RANGE_LONG)
		ideal = AI_RANGE_LONG;
	
	// if distance is greater than ideal range, get closer; otherwise, move back
	if (dist > ideal)
	{
		ucmd->forwardmove += 400;
		//gi.dprintf("%s: ideal: %.0f dist: %.0f -- closing distance\n", __func__, ideal, dist);
	}
	else
	{
		ucmd->forwardmove -= 400;
		//gi.dprintf("%s: ideal: %.0f dist: %.0f -- moving away\n", __func__, ideal, dist);
	}

	// calculate forward movement velocity
	//forward_speed = AI_ForwardVelocity(self);

	//gi.dprintf("combat: weapon: %d ideal range: %f actual: %f spd:%f\n", weapon, ideal, dist, forward_speed);
	/*
	// if we're moving rapidly forward and are greater than 1 jump away from ideal distance, jump
	if (forward_speed > 280 && dist - ideal > AI_JUMPABLE_DISTANCE)
	{
		BOT_DMclass_BunnyHop(self, ucmd, false);
		//gi.dprintf("bunny hop FORWARD: fspd: %f velocity: %f dist: %f ideal: %f\n", forward_speed, VectorLength(self->velocity), dist, ideal);
	}
	// if we're moving rapidly backward and are greater than 1 jump away from ideal distance, jump
	else if (forward_speed < -280 && ideal - dist > AI_JUMPABLE_DISTANCE)
	{
		BOT_DMclass_BunnyHop(self, ucmd, false);
		//gi.dprintf("bunny hop BACKWARD: fspd: %f velocity: %f dist: %f ideal: %f\n", forward_speed, VectorLength(self->velocity), dist, ideal);
	}
	
	else*/
	qboolean strafe = true;
	// if the ideal distance is greater than jump distance, then try to bunnyhop
	if (fabsf(dist - ideal) > AI_JUMPABLE_DISTANCE)
	{
		BOT_DMclass_BunnyHop(self, ucmd, false);
		if (self->ai.is_bunnyhop) // note: is_bunnyhop is set/reset by preceding function--don't rely on an accurate value without calling it beforehand!
			strafe = false;
	}

	if (strafe)
	{
		// Randomly choose a movement direction
		c = random();
		if (c < 0.4 && AI_CanMove(self, BOT_MOVE_LEFT))
			ucmd->sidemove -= 400;
		else if (c < 0.8 && AI_CanMove(self, BOT_MOVE_RIGHT))
			ucmd->sidemove += 400;
		//gi.dprintf("moving side to side\n");
		//gi.dprintf("fspd bot: %f fspd enemy: %f\n", forward_speed, AI_ForwardVelocity(self->enemy));
	}

	// while using melee weapons, try jumping if the enemy is at close range and overhead
	if (AIWeapons[weapon].idealRange == AI_RANGE_MELEE && dist < AI_RANGE_SHORT)
		BOT_DMclassJumpAttack(self, ucmd, ideal);
}

//==========================================
// BOT_DMclass_CheckShot
// Checks if shot is blocked or if too far to shoot
//==========================================
qboolean BOT_DMclass_CheckShot(edict_t *ent, vec3_t	point)
{
	trace_t tr;
	vec3_t	start, forward, right, offset;

	//AI_DebugPrintf("BOT_DMclass_CheckShot()\n");

	AngleVectors (ent->client->v_angle, forward, right, NULL);

	VectorSet(offset, 8, 8, ent->viewheight-8);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	//bloqued, don't shoot
	tr = gi.trace( start, vec3_origin, vec3_origin, point, ent, MASK_AISOLID);
//	trap_Trace( &tr, self->s.origin, vec3_origin, vec3_origin, point, self, MASK_AISOLID);
	if (tr.fraction < 0.3) //just enough to prevent self damage (by now)
		return false;

	if (tr.surface && !tr.ent) // we hit solid. we're blocked
		return false;

	return true;
}

// GHz note: probably want to switch to findclosestradius_targets in the future!
//==========================================
// BOT_DMclass_FindEnemy
// Scan for enemy (simplifed for now to just pick any visible enemy)
//==========================================
qboolean BOT_DMclass_FindEnemy(edict_t* self)
{
	int i;

	edict_t* bestenemy = NULL;
	float		bestweight = 99999;
	float		weight;
	vec3_t		dist;

	if (self->ai.attack_delay > level.time)
		return false;

	//FIXME: it's probably worth recalculating every frame, but maybe we need an aggro timer so the bot doesn't change targets too often
	// especially when the bot is hurt by an enemy (even if they are further away)
	// we already set up an enemy this frame (reacting to attacks)
	//if (self->enemy && self->enemy->inuse && visible(self, self->enemy))//GHz: don't bother finding a new enemy if the last one is still visible
	//	return true;

	if (level.time < pregame_time->value) // No enemies in pregame lol
		return false;

	//AI_DebugPrintf("BOT_DMclass_FindEnemy()\n");

	// Find Enemy
	for (i = 0;i < num_AIEnemies;i++)
	{
		if (AIEnemies[i] == NULL || AIEnemies[i] == self)
			continue;

		if (!G_ValidTargetEnt(AIEnemies[i], true))
			continue;
		if (OnSameTeam(self, AIEnemies[i]))
			continue;

		//Ignore players with 0 weight (was set at botstatus)
		if (self->ai.status.playersWeights[i] == 0)
			continue;

		if (!visible(self, AIEnemies[i]))
			continue;
		if (!infront(self, AIEnemies[i]))
			continue;

		//(weight enemies from fusionbot) Is enemy visible, or is it too close to ignore 
		VectorSubtract(self->s.origin, AIEnemies[i]->s.origin, dist);
		weight = VectorLength(dist);

		//modify weight based on precomputed player weights
		weight *= (1.0 - self->ai.status.playersWeights[i]);

		if (weight < 500)
		{
			// Check if best target, or better than current target
			if (weight < bestweight)
			{
				bestweight = weight;
				bestenemy = AIEnemies[i];
			}
		}
	}

	// If best enemy, set up
	if (bestenemy)
	{
		//if (bestenemy->ai.is_bot)
		//	gi.dprintf("%s: is angry at %s\n", self->ai.pers.netname, bestenemy->ai.pers.netname);
		//else if (bestenemy->mtype)
		//	gi.dprintf("%s: is angry at %s\n", self->ai.pers.netname, V_GetMonsterName(bestenemy));
		//else
		//	gi.dprintf("%s: is angry at %s\n", self->ai.pers.netname, bestenemy->classname);
		if (AIDevel.debugChased && bot_showcombat->value)
		{
			if (bestenemy->ai.is_bot)
				safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: selected %s as enemy.\n",
					self->ai.pers.netname, bestenemy->ai.pers.netname);
			else if (bestenemy->client)
				safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: selected %s as enemy.\n",
					self->ai.pers.netname, bestenemy->client->pers.netname);
			else
				safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: selected %s as enemy.\n",
					self->ai.pers.netname, bestenemy->classname);
		}

		//if (bestenemy->takedamage != DAMAGE_NO //GHz: enemy can take damage
		//	&& !strstr(bestenemy->classname, "tech_")) //GHz: and isn't a tech? why would the enemy be a tech??
		//{
			//gi.dprintf("bot set enemy\n");
			self->enemy = bestenemy;
			return true;
		//}
	}

	return false;	// NO enemy
}
/*
qboolean BOT_DMclass_FindEnemy(edict_t *self)
{
	int i;

	edict_t		*bestenemy = NULL;
	float		bestweight = 99999;
	float		weight;
	vec3_t		dist;

	// we already set up an enemy this frame (reacting to attacks)
	if(self->enemy != NULL)
		return true;

	if (level.time < pregame_time->value) // No enemies in pregame lol
		return false;

	// Find Enemy
	for(i=0;i<num_AIEnemies;i++)
	{
		if( AIEnemies[i] == NULL || AIEnemies[i] == self || AIEnemies[i]->solid == SOLID_NOT)
			continue;

		//Ignore players with 0 weight (was set at botstatus)
		if(self->ai.status.playersWeights[i] == 0)
			continue;

		if (OnSameTeam(AIEnemies[i], self)) // vortex chile 3.0
			continue;

		if( !AIEnemies[i]->deadflag && gi.inPVS(self->s.origin, AIEnemies[i]->s.origin))
		{
			//(weight enemies from fusionbot) Is enemy visible, or is it too close to ignore 
			VectorSubtract(self->s.origin, AIEnemies[i]->s.origin, dist);
			weight = VectorLength( dist );

			//modify weight based on precomputed player weights
			weight *= (1.0 - self->ai.status.playersWeights[i]);

			// wall between me and my potential enemy?
			if (!visible1(self, AIEnemies[i]))
			{
				weight = 299; // Low weight.
			}

			if(weight < 500)
			{
				// Check if best target, or better than current target
				if (weight < bestweight)
				{
					bestweight = weight;
					bestenemy = AIEnemies[i];
				}
			}
		}
	}

	// If best enemy, set up
	if(bestenemy)
	{
		if (AIDevel.debugChased && bot_showcombat->value)
		{
			if (bestenemy->ai.is_bot)
				safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: selected %s as enemy.\n", 
					self->ai.pers.netname, bestenemy->ai.pers.netname);
			else if (bestenemy->client)
				safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: selected %s as enemy.\n",
					self->ai.pers.netname, bestenemy->client->pers.netname);
			else
				safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: selected %s as enemy.\n",
					self->ai.pers.netname, bestenemy->classname);
		}

		if (bestenemy->takedamage != DAMAGE_NO //GHz: enemy can take damage
			&& !strstr(bestenemy->classname,"tech_")) //GHz: and isn't a tech? why would the enemy be a tech??
		{
			gi.dprintf("bot set enemy\n");
			self->enemy = bestenemy;
			return true;
		}
	}

	return false;	// NO enemy
}
*/

//==========================================
// BOT_DMClass_ChangeWeapon
//==========================================
qboolean BOT_DMClass_ChangeWeapon (edict_t *ent, gitem_t *item)
{
	int			ammo_index;
	gitem_t		*ammo_item;

	//AI_DebugPrintf("BOT_DMclass_ChangeWeapon()\n");

	// see if we're already using it
	if (!item || item == ent->client->pers.weapon)
		return true;

	// Has not picked up weapon yet
	if(!ent->client->pers.inventory[ITEM_INDEX(item)])
		return false;

	// Do we have ammo for it?
	if (item->ammo)
	{
		ammo_item = FindItem(item->ammo);
		ammo_index = ITEM_INDEX(ammo_item);
		if ( !ent->client->pers.inventory[ammo_index] && !g_select_empty->value )
			return false;
	}

	// Change to this weapon
	ent->client->newweapon = item;
	ent->ai.changeweapon_timeout = level.time + 6.0;

	return true;
}


//==========================================
// BOT_DMclass_ChooseWeapon
// Choose weapon based on range & weights
//==========================================
void BOT_DMclass_ChooseWeapon(edict_t *self)
{
	float	dist;
	vec3_t	v;
	int		i;
	float	best_weight = 0.0;
	gitem_t	*best_weapon = NULL;
	int		weapon_range = 0;

	// if no enemy, then what are we doing here?
	if(!self->enemy)
		return;

	if( self->ai.changeweapon_timeout > level.time )
		return;

	if (self->myskills.class_num == CLASS_KNIGHT)
		return;

	//AI_DebugPrintf("BOT_DMclass_ChooseWeapon()\n");

	// Base weapon selection on distance: 
	VectorSubtract (self->s.origin, self->enemy->s.origin, v);
	dist = VectorLength(v);

	//if(dist < 150)
	if (dist < AI_RANGE_MELEE)
		weapon_range = AIWEAP_MELEE_RANGE;

	//else if(dist < 500)	//Medium range limit is Grenade Laucher range
	else if (dist < AI_RANGE_SHORT)
		weapon_range = AIWEAP_SHORT_RANGE;

	//else if(dist < 900)
	else if (dist < AI_RANGE_MEDIUM)
		weapon_range = AIWEAP_MEDIUM_RANGE;

	else if (dist < AI_RANGE_LONG)
		weapon_range = AIWEAP_LONG_RANGE;

	else 
		weapon_range = AIWEAP_SNIPER_RANGE;

	for(i=0; i<WEAP_TOTAL; i++)
	{
		if (!AIWeapons[i].weaponItem)
			continue;

		//ignore those we don't have
		if (!self->client->pers.inventory[ITEM_INDEX(AIWeapons[i].weaponItem)] )
			continue;
		
		//gi.dprintf("%s: weight: ", AIWeapons[i].weaponItem->classname);
		//ignore those we don't have ammo for
		if (AIWeapons[i].ammoItem != NULL	//excepting for those not using ammo
			&& !self->client->pers.inventory[ITEM_INDEX(AIWeapons[i].ammoItem)] )
			continue;
		
		//compare range weights
		float weight = AIWeapons[i].RangeWeight[weapon_range] + self->ai.status.weaponWeights[i];//GHz
		//gi.dprintf("%f\n", weight);
		if (weight > best_weight) {
			best_weight = weight;
			best_weapon = AIWeapons[i].weaponItem;
		}
		//jal: enable randomnes later
		else if (weight == best_weight && random() > 0.2) {	//allow some random for equal weights
			best_weight = weight;
			best_weapon = AIWeapons[i].weaponItem;
		}
	}
	
	//do the change (same weapon, or null best_weapon is covered at ChangeWeapon)
	BOT_DMClass_ChangeWeapon( self, best_weapon );
	return;
}

//==========================================
// BOT_DMclass_FireWeapon
// Fire if needed
//==========================================
void BOT_DMclass_FireWeapon (edict_t *self, usercmd_t *ucmd)
{
	//float	c;
	float	dist, velocity, firedelay;
	float projectile_speed=0;//GHz
	vec3_t  target;
	vec3_t  angles;
	int		weapon;
	//vec3_t	attackvector;
	//float	dist;
	qboolean use_prediction = false;//GHz
	qboolean override_pitch = false;//GHz

	if (!self->enemy)
		return;

	//AI_DebugPrintf("BOT_DMclass_FireWeapon()\n");

	//weapon = self->s.skinnum & 0xff;
	if (self->client->pers.weapon)
		weapon = (self->client->pers.weapon->weapmodel & 0xff);
	else
		weapon = 0;

	//gi.dprintf("fireweapon %d\n", weapon);
	//jalToDo: Add different aiming types (explosive aim to legs, hitscan aim to body)

	//was find range. I might use it later
	//VectorSubtract( self->s.origin, self->enemy->s.origin, attackvector);
	//dist = VectorLength( attackvector);

	// Aim
	//VectorCopy(self->enemy->s.origin,target);
	G_EntMidPoint(self->enemy, target);

	// get enemy distance and velocity
	dist = entdist(self, self->enemy);
	velocity = VectorLength(self->enemy->velocity);

	//GHz: switch to lance if enemy is outside sword range
	if (weapon == WEAP_SWORD && dist > AI_RANGE_SHORT)
	{
		self->client->weapon_mode = 1; //switch to lance mode--FIXME: shouldn't this be done @ chooseweapon?
		//target[2] = self->enemy->absmin[2]-32;//aim at the feet
		use_prediction = true;
		override_pitch = true;
	}
	else
		self->client->weapon_mode = 0;

	// find out our weapon AIM style
	if( AIWeapons[weapon].aimType == AI_AIMSTYLE_PREDICTION_EXPLOSIVE )
	{
		//aim to the feets when enemy isn't higher
		//if( self->s.origin[2] + self->viewheight > target[2] + (self->enemy->mins[2] * 0.8) )
		//	target[2] += self->enemy->mins[2];
		if (self->enemy->groundentity && (self->s.origin[2] + self->viewheight > self->enemy->absmin[2]))//GHz
			target[2] = self->enemy->absmin[2];//GHz
		use_prediction = true;
		
	}
	
	if ( AIWeapons[weapon].aimType == AI_AIMSTYLE_PREDICTION || use_prediction)
	{
		//jalToDo
		
		// move our target point based on projectile and enemy velocity
		if (projectile_speed = AI_GetWeaponProjectileVelocity(self, weapon))
			VectorMA(target, (float)dist / projectile_speed, self->enemy->velocity, target);
		//gi.dprintf("projectile speed: %f\n", projectile_speed);
	}
	else if ( AIWeapons[weapon].aimType == AI_AIMSTYLE_DROP )
	{
		//jalToDo

	} else { //AI_AIMSTYLE_INSTANTHIT

	}

	// modify attack angles based on accuracy (mess this up to make the bot's aim not so deadly)
	target[0] += (random()-0.5) * ((MAX_BOT_SKILL - self->ai.pers.skillLevel) *2);
	target[1] += (random()-0.5) * ((MAX_BOT_SKILL - self->ai.pers.skillLevel) *2);

	// Set direction
	if (self->ai.state == BOT_STATE_MOVEATTACK)//GHz: don't set/modify move_vector in this state
	{
		VectorSubtract(target, self->s.origin, angles);
		vectoangles(angles, angles);
	}
	else
	{
		VectorSubtract(target, self->s.origin, self->ai.move_vector);
		vectoangles(self->ai.move_vector, angles);
	}
	VectorCopy(angles,self->s.angles); // set aiming/viewing angles
	VectorCopy(angles, self->client->v_angle);
	if (override_pitch && projectile_speed)
	{
		if ((self->s.angles[PITCH] = BOT_DMclass_ThrowingPitch1(self, projectile_speed)) < -90)
		{
			self->s.angles[PITCH] = angles[PITCH];
			self->client->v_angle[PITCH] = angles[PITCH];
		}
	}

	//GHz: don't bother firing if we're unlikely to hit anything
	if (AI_GetWeaponRangeWeightByDistance(weapon, dist) < 0.1)
		return;

	// Set the attack 
	firedelay = random()*(MAX_BOT_SKILL*1.8);
	if (firedelay > (MAX_BOT_SKILL - self->ai.pers.skillLevel) && BOT_DMclass_CheckShot(self, target))
		ucmd->buttons = BUTTON_ATTACK;

	//if(AIDevel.debugChased && bot_showcombat->value)
	//	safe_cprintf (AIDevel.chaseguy, PRINT_HIGH, "%s: attacking %s\n",self->ai.pers.netname ,self->enemy->classname);
}

//==========================================
// BOT_DMclass_WeightPlayers
// weight players based on game state
//==========================================
void BOT_DMclass_WeightPlayers(edict_t *self)
{
	int i;

	//AI_DebugPrintf("BOT_DMclass_WeightPlayers()\n");

	//clear
	memset(self->ai.status.playersWeights, 0, sizeof (self->ai.status.playersWeights));

	for( i=0; i<num_AIEnemies; i++ )
	{
		if( AIEnemies[i] == NULL || !AIEnemies[i]->inuse)
			continue;

		if( AIEnemies[i] == self )
			continue;

		//ignore spectators and dead players
		if( AIEnemies[i]->svflags & SVF_NOCLIENT || AIEnemies[i]->deadflag ) {
			self->ai.status.playersWeights[i] = 0.0f;
			continue;
		}

		if( ctf->value )
		{
			if( AIEnemies[i]->teamnum != self->teamnum )
			{
				//being at enemy team gives a small weight, but weight afterall
				self->ai.status.playersWeights[i] = 0.2;

				if (!AIEnemies[i]->client)
					continue;

				//enemy has redflag
				if( redflag && AIEnemies[i]->client->pers.inventory[ITEM_INDEX(redflag)]
					&& (self->teamnum == CTF_TEAM1) )
				{
					if( !self->client->pers.inventory[ITEM_INDEX(blueflag)] ) //don't hunt if you have the other flag, let others do
						self->ai.status.playersWeights[i] = 0.9;
				}
				
				//enemy has blueflag
				if( blueflag && AIEnemies[i]->client->pers.inventory[ITEM_INDEX(blueflag)]
					&& (self->teamnum == CTF_TEAM2) )
				{
					if( !self->client->pers.inventory[ITEM_INDEX(redflag)] ) //don't hunt if you have the other flag, let others do
						self->ai.status.playersWeights[i] = 0.9;
				}
			} 
		}
		else
		{
			// summoners prefer to hang around their own summons or allies for safety
			if (AI_NumSummons(self) > 0)
			{
				if (AI_IsOwnedSummons(self, AIEnemies[i]))
					self->ai.status.playersWeights[i] = 0.4;
				else if (IsAlly(self, AIEnemies[i]))
					self->ai.status.playersWeights[i] = 0.3;
				else
					self->ai.status.playersWeights[i] = 0.2;
			}
			else
			{
				// GHz: chase enemies!
				if (self->enemy && self->enemy->inuse && AIEnemies[i] == self->enemy)
					self->ai.status.playersWeights[i] = 0.9;
				else
					//if not at ctf every player has some value
					self->ai.status.playersWeights[i] = 0.3;
			}
		}
	
	}
}


//==========================================
// BOT_DMclass_WantedFlag
// find needed flag
//==========================================
gitem_t	*BOT_DMclass_WantedFlag (edict_t *self)
{
	qboolean	hasflag;

	if (!ctf->value)
		return NULL;
	
	if (!self->client || !self->teamnum)
		return NULL;
	
	//find out if the player has a flag, and what flag is it
	if (redflag && self->client->pers.inventory[ITEM_INDEX(redflag)])
		hasflag = true;
	else if (blueflag && self->client->pers.inventory[ITEM_INDEX(blueflag)])
		hasflag = true;
	else
		hasflag = false;

	//jalToDo: see if our flag is at base

	if (!hasflag)//if we don't have a flag we want other's team flag
	{
		if (self->teamnum == CTF_TEAM1)
			return blueflag;
		else
			return redflag;
	}
	else	//we have a flag
	{
		if (self->teamnum == CTF_TEAM1)
			return redflag;
		else
			return blueflag;
	}

	return NULL;
}

int AI_GetPSlevel(edict_t* ent)
{
	// use power shield level or brain level, whichever is highest
	int pslevel = ent->myskills.abilities[POWER_SHIELD].current_level;
	if (ent->mtype == MORPH_BRAIN && ent->myskills.abilities[BRAIN].current_level > pslevel)
		pslevel = ent->myskills.abilities[BRAIN].current_level;
	if (ent->mtype == MORPH_BERSERK && ent->myskills.abilities[BERSERK].current_level > pslevel)
		pslevel = ent->myskills.abilities[BERSERK].current_level;
	return pslevel;
}

// adjusts the weight of ammo (downward) based on bot's current needs
void AI_AdjustAmmoNeedFactor(edict_t *self, gitem_t *ammoItem, ...)
{
	int weapIndex;
	qboolean has_weapon = false;
	qboolean is_fighting = false;

	va_list list;
	va_start(list, ammoItem);

	int ammo_index = ITEM_INDEX(ammoItem);

	// if we can't pick it up, reduce the weight to 0
	if (!AI_CanPick_Ammo(self, ammoItem))
		self->ai.status.inventoryWeights[ammo_index] = 0.0;

	// does the bot need cells for power screen/shield?
	if (ammo_index == cell_index && AI_GetPSlevel(self))
		return; // don't reduce cells weight

	// are we fighting?
	if (self->ai.state == BOT_STATE_ATTACK || self->ai.state == BOT_STATE_MOVEATTACK)
		is_fighting = true;

	// if we don't have the weapon, reduce the weight
	while (1)
	{
		// get the next argument in the list
		weapIndex = va_arg(list, int);
		// reached the end of the weapons list (no additional parameters)
		if (!weapIndex)
			break;
		// is the bot fighting and is this weapon our respawn weapon?
		if (is_fighting && weapIndex != AI_RespawnWeaponToWeapIndex(self->myskills.respawn_weapon))
			continue; // nope, check the next weapon on the list
		gitem_t* weaponItem = AIWeapons[weapIndex].weaponItem;
		// does the bot have this weapon?
		if (self->client->pers.inventory[ITEM_INDEX(weaponItem)])
		{
			has_weapon = true;
			break;
		}
	}
	// clean up the argument list
	va_end(list);

	// if the bot doesn't have any weapons that use this ammo type, then reduce the ammo weight
	if (!has_weapon)
	{
		// if the bot is fighting, only our respawn weapon's ammo matters
		if (is_fighting)
			self->ai.status.inventoryWeights[ammo_index] = 0.0;
		else
			self->ai.status.inventoryWeights[ammo_index] *= 0.5;
	}

}

//==========================================
// BOT_DMclass_WeightInventory
// weight items up or down based on bot needs
//==========================================
void BOT_DMclass_WeightInventory(edict_t *self)
{
	float		LowNeedFactor = 0.5;
	gclient_t	*client;
	int			i;

	//AI_DebugPrintf("BOT_DMclass_WeightInventory()\n");

	client = self->client;

	//reset with persistant values
	memcpy(self->ai.status.inventoryWeights, self->ai.pers.inventoryWeights, sizeof(self->ai.pers.inventoryWeights));
	

	//weight ammo down if bot doesn't have the weapon for it,
	//or denny weight for it, if bot is packed up.
	//------------------------------------------------------

	AI_AdjustAmmoNeedFactor(self, Fdi_BULLETS, WEAP_CHAINGUN, WEAP_MACHINEGUN, 0);
	AI_AdjustAmmoNeedFactor(self, Fdi_SHELLS, WEAP_SHOTGUN, WEAP_SUPERSHOTGUN, WEAP_20MM, 0);
	AI_AdjustAmmoNeedFactor(self, Fdi_ROCKETS, WEAP_ROCKETLAUNCHER, 0);
	AI_AdjustAmmoNeedFactor(self, Fdi_GRENADES, WEAP_GRENADELAUNCHER, 0);
	AI_AdjustAmmoNeedFactor(self, Fdi_CELLS, WEAP_HYPERBLASTER, WEAP_BFG, 0);
	AI_AdjustAmmoNeedFactor(self, Fdi_SLUGS, WEAP_RAILGUN, 0);
	//gi.dprintf("b:%.1f c:%.1f s:%.1f r:%.1f g:%.1f s:%.1f\n", self->ai.status.inventoryWeights[bullet_index], self->ai.status.inventoryWeights[cell_index], 
	//	self->ai.status.inventoryWeights[shell_index], self->ai.status.inventoryWeights[rocket_index], self->ai.status.inventoryWeights[grenade_index], 
	//	self->ai.status.inventoryWeights[slug_index]);
	//AMMO_BULLETS
	/*
	if (!AI_CanPick_Ammo (self, AIWeapons[WEAP_MACHINEGUN].ammoItem) )
		self->ai.status.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_MACHINEGUN].ammoItem)] = 0.0;
	//find out if it has a weapon for this amno
	else if (!client->pers.inventory[ITEM_INDEX(AIWeapons[WEAP_CHAINGUN].weaponItem)]
		&& !client->pers.inventory[ITEM_INDEX(AIWeapons[WEAP_MACHINEGUN].weaponItem)] )
		self->ai.status.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_MACHINEGUN].ammoItem)] *= LowNeedFactor;

	//AMMO_SHELLS:

	//find out if it's packed up
	if (!AI_CanPick_Ammo (self, AIWeapons[WEAP_SHOTGUN].ammoItem) )
		self->ai.status.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_SHOTGUN].ammoItem)] = 0.0;
	//find out if it has a weapon for this amno
	else if (!client->pers.inventory[ITEM_INDEX(AIWeapons[WEAP_SHOTGUN].weaponItem)]
		&& !client->pers.inventory[ITEM_INDEX(AIWeapons[WEAP_SUPERSHOTGUN].weaponItem)] )
		self->ai.status.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_SHOTGUN].ammoItem)] *= LowNeedFactor;

	//AMMO_ROCKETS:

	//find out if it's packed up
	if (!AI_CanPick_Ammo (self, AIWeapons[WEAP_ROCKETLAUNCHER].ammoItem))
		self->ai.status.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_ROCKETLAUNCHER].ammoItem)] = 0.0;
	//find out if it has a weapon for this amno
	else if (!client->pers.inventory[ITEM_INDEX(AIWeapons[WEAP_ROCKETLAUNCHER].weaponItem)] )
		self->ai.status.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_ROCKETLAUNCHER].ammoItem)] *= LowNeedFactor;

	//AMMO_GRENADES: 

	//find if it's packed up
	if (!AI_CanPick_Ammo (self, AIWeapons[WEAP_GRENADES].ammoItem))
		self->ai.status.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_GRENADES].ammoItem)] = 0.0;
	//grenades are also weapons, and are weighted down by LowNeedFactor in weapons group
	
	//AMMO_CELLS:

	//find out if it's packed up
	if (!AI_CanPick_Ammo (self, AIWeapons[WEAP_HYPERBLASTER].ammoItem))
		self->ai.status.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_HYPERBLASTER].ammoItem)] = 0.0;
	//find out if it has a weapon for this amno
	else if (!client->pers.inventory[ITEM_INDEX(AIWeapons[WEAP_HYPERBLASTER].weaponItem)]
		&& !client->pers.inventory[ITEM_INDEX(AIWeapons[WEAP_BFG].weaponItem)]
		&& !client->pers.inventory[ITEM_INDEX(FindItemByClassname("item_power_shield"))])
		self->ai.status.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_HYPERBLASTER].ammoItem)] *= LowNeedFactor;

	//AMMO_SLUGS:

	//find out if it's packed up
	if (!AI_CanPick_Ammo (self, AIWeapons[WEAP_RAILGUN].ammoItem))
		self->ai.status.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_RAILGUN].ammoItem)] = 0.0;
	//find out if it has a weapon for this amno
	else if (!client->pers.inventory[ITEM_INDEX(AIWeapons[WEAP_RAILGUN].weaponItem)] )
		self->ai.status.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_RAILGUN].ammoItem)] *= LowNeedFactor;
	*/

	//WEAPONS
	//-----------------------------------------------------

	//weight weapon down if bot already has it
	for (i=0; i<WEAP_TOTAL; i++) {
		if ( AIWeapons[i].weaponItem && client->pers.inventory[ITEM_INDEX(AIWeapons[i].weaponItem)])
			self->ai.status.inventoryWeights[ITEM_INDEX(AIWeapons[i].weaponItem)] = 0;
	}

	//ARMOR
	//-----------------------------------------------------
	//shards are ALWAYS accepted but still...
	if (!AI_CanUseArmor ( FindItemByClassname("item_armor_shard"), self ))
		self->ai.status.inventoryWeights[ITEM_INDEX(FindItemByClassname("item_armor_shard"))] = 0.0;

	if (!AI_CanUseArmor ( FindItemByClassname("item_armor_jacket"), self ))
		self->ai.status.inventoryWeights[ITEM_INDEX(FindItemByClassname("item_armor_jacket"))] = 0.0;

	if (!AI_CanUseArmor ( FindItemByClassname("item_armor_combat"), self ))
		self->ai.status.inventoryWeights[ITEM_INDEX(FindItemByClassname("item_armor_combat"))] = 0.0;

	if (!AI_CanUseArmor ( FindItemByClassname("item_armor_body"), self ))
		self->ai.status.inventoryWeights[ITEM_INDEX(FindItemByClassname("item_armor_body"))] = 0.0;

	
	//TECH :
	//-----------------------------------------------------
	if ( self->client->pers.inventory[ITEM_INDEX( FindItemByClassname("tech_resistance"))] 
		|| self->client->pers.inventory[ITEM_INDEX( FindItemByClassname("tech_strength"))] 
		|| self->client->pers.inventory[ITEM_INDEX( FindItemByClassname("tech_regeneration"))] 
		|| self->client->pers.inventory[ITEM_INDEX( FindItemByClassname("tech_haste"))] )
	{
		self->ai.status.inventoryWeights[ITEM_INDEX( FindItemByClassname("tech_resistance"))] = 0.0; 
		self->ai.status.inventoryWeights[ITEM_INDEX( FindItemByClassname("tech_strength"))] = 0.0; 
		self->ai.status.inventoryWeights[ITEM_INDEX( FindItemByClassname("tech_regeneration"))] = 0.0;
		self->ai.status.inventoryWeights[ITEM_INDEX( FindItemByClassname("tech_haste"))] = 0.0;
	}

	//CTF: 
	//-----------------------------------------------------
	if( ctf->value )
	{
		gitem_t		*wantedFlag;

		wantedFlag = BOT_DMclass_WantedFlag( self ); //Returns the flag gitem_t
		
		//flags have weights defined inside persistant inventory. Remove weight from the unwanted one/s.
		if (blueflag && blueflag != wantedFlag)
			self->ai.status.inventoryWeights[ITEM_INDEX(blueflag)] = 0.0;
		if (redflag && redflag != wantedFlag)
			self->ai.status.inventoryWeights[ITEM_INDEX(redflag)] = 0.0;
	}
}


//==========================================
// BOT_DMclass_UpdateStatus
// update ai.status values based on bot state,
// so ai can decide based on these settings
//==========================================
void BOT_DMclass_UpdateStatus( edict_t *self )
{
	//AI_DebugPrintf("BOT_DMclass_UpdateStatus()\n");

	if (!G_EntIsAlive(self->enemy))//GHz: stay angry at this entity--used for LR goal setting
		self->enemy = NULL;
	//if (self->movetarget && (!self->movetarget->inuse || self->movetarget->solid == SOLID_NOT))//GHz: need to clear picked up items
	//	self->movetarget = NULL;

	// Set up for new client movement: jalfixme
	VectorCopy(self->client->ps.viewangles,self->s.angles);
	VectorSet (self->client->ps.pmove.delta_angles, 0, 0, 0);

	//JALFIXMEQ2
/*
	if (self->client->jumppad_time)
		self->ai.status.jumpadReached = true;	//jumpad time from client to botStatus
	else
		self->ai.status.jumpadReached = false;
*/
	if (self->client->ps.pmove.pm_flags & PMF_TIME_TELEPORT)
		self->ai.status.TeleportReached = true;
	else
		self->ai.status.TeleportReached = false;

	//set up AI status for the upcoming AI_frame
	BOT_DMclass_WeightInventory( self );	//weight items
	BOT_DMclass_WeightPlayers( self );		//weight players
}


//==========================================
// BOT_DMClass_BloquedTimeout
// the bot has been bloqued for too long
//==========================================
void BOT_DMClass_BloquedTimeout( edict_t *self )
{
	AI_DebugPrintf("BOT_DMclass_BloquedTimeout()\n");

	if (!BOT_DMclass_UseTball(self, false))
	{
		AI_DebugPrintf("bot suicide!\n");
		self->health = 0;
		self->ai.bloqued_timeout = level.time + 15.0;
		self->die(self, self, self, 100000, vec3_origin);
	}
	self->nextthink = level.time + FRAMETIME;
}


//==========================================
// BOT_DMclass_DeadFrame
// ent is dead = run this think func
//==========================================
void BOT_DMclass_DeadFrame( edict_t *self )
{
	usercmd_t	ucmd;

	//AI_DebugPrintf("BOT_DMclass_DeadFrame()\n");
	// ask for respawn
	self->client->buttons = 0;
	ucmd.buttons = BUTTON_ATTACK;
	ClientThink (self, &ucmd);
	self->nextthink = level.time + FRAMETIME;
}

qboolean BOT_DMclass_ChooseMoveAttack(edict_t* self)
{
	// use moveattack if...
	// enemy isn't visible
	if (!visible(self, self->enemy))
		return true;
	// enemy is on different Z plane, isn't flying, and is beyond medium range
	//if (fabs(self->absmin[2] - self->enemy->absmin[2]) > (AI_JUMPABLE_HEIGHT + AI_STEPSIZE) && !(self->enemy->flags & FL_FLY) && entdist(self, self->enemy) > AI_RANGE_MEDIUM)
	//	return true;
	if (!AI_ClearWalkingPath(self, self->s.origin, self->enemy->s.origin))
		return true;
	// enemy should be easily reachable, so pathfinding isn't needed, just attack!
	return false;
}

qboolean AI_SetupMoveAttack(edict_t* self);//GHz
void AI_SetUpCombatMovement(edict_t* ent);//GHz
//==========================================
// BOT_DMclass_RunFrame
// States Machine & call client movement
//==========================================
void BOT_DMclass_RunFrame( edict_t *self )
{
	usercmd_t	ucmd;
	memset( &ucmd, 0, sizeof(ucmd) );

	// Look for enemies
	if( BOT_DMclass_FindEnemy(self) ) // find an enemy
	{
		if (!BOT_DMclassEvadeEnemy(self, &ucmd)) // run away if we have to
		{
			//gi.dprintf("attack enemy!\n");
			BOT_DMclass_ChooseWeapon(self); // chose the best weapon for the job
			BOT_DMclass_FireWeapon(self, &ucmd); // fire!

			BOT_DMclass_UseBoost(self); // if we have boost, use it to get closer to the enemy
			BOT_DMclass_UseBlinkStrike(self); // if we have blinkstrike, use it to get closer & behind the enemy
			int attack_ability = BOT_DMclass_ChooseAbility(self); // chose the best ability to attack with
			BOT_DMclass_FireAbility(self, attack_ability); // fire!

			if (level.time > self->ai.evade_delay) // not a tactical retreat (aka run away while attacking)
			{
				if (BOT_DMclass_ChooseMoveAttack(self)) // do we need pathfinding to get to the enemy?
					AI_SetupMoveAttack(self);
				else
					AI_SetUpCombatMovement(self); // nope, use simple combat movement instead
			}
			self->ai.state_combat_timeout = level.time + 1.0;
		}
	
	} else if( (self->ai.state == BOT_STATE_ATTACK || self->ai.state == BOT_STATE_MOVEATTACK) &&
		level.time > self->ai.state_combat_timeout)
	{
		if (self->ai.state == BOT_STATE_MOVEATTACK && self->ai.evade_delay > level.time)//bot was evading
		{
			//gi.dprintf("ENEMY CLEARED!\n");
			self->enemy = NULL;//GHz:  clear enemy so that the bot looks for a different goal
		}
		//Jalfixme: change to: AI_SetUpStateMove(self);
		self->ai.state = BOT_STATE_MOVE;
		//GHz: is the bot evading?

		self->ai.evade_delay = 0;
	}
	else
	{
		BOT_DMclass_UseSkeleton(self); // try to spawn skeletons if upgraded and we have less than max
	}

	// Execute the move, or wander
	if (self->ai.state == BOT_STATE_MOVE)
		BOT_DMclass_Move(self, &ucmd); // use pathfinding to reach a goal

	else if (self->ai.state == BOT_STATE_ATTACK)
		BOT_DMclass_CombatMovement(self, &ucmd);

	else if (self->ai.state == BOT_STATE_MOVEATTACK)
		BOT_DMclass_MoveAttack(self, &ucmd);//GHz

	else if ( self->ai.state == BOT_STATE_WANDER ) // bot wanders when it has no goal or path to it
		BOT_DMclass_Wander( self, &ucmd );

	//set up for pmove
	ucmd.angles[PITCH] = ANGLE2SHORT(self->s.angles[PITCH]);
	ucmd.angles[YAW] = ANGLE2SHORT(self->s.angles[YAW]);
	ucmd.angles[ROLL] = ANGLE2SHORT(self->s.angles[ROLL]);

	// set approximate ping and show values
	ucmd.msec = 75 + floor (random () * 25) + 1;
	self->client->ping = ucmd.msec;

	// send command through id's code
	ClientThink( self, &ucmd );
	self->nextthink = level.time + FRAMETIME;
}


//==========================================
// BOT_DMclass_InitPersistant
// Persistant after respawns. 
//==========================================
void BOT_DMclass_InitPersistant(edict_t *self)
{
	self->classname = "dmbot";

	//copy name
	if (self->client->pers.netname)
		self->ai.pers.netname = self->client->pers.netname;
	else
		self->ai.pers.netname = "dmBot";

	//set 'class' functions
	self->ai.pers.RunFrame = BOT_DMclass_RunFrame;
	self->ai.pers.UpdateStatus = BOT_DMclass_UpdateStatus;
	self->ai.pers.bloquedTimeout = BOT_DMClass_BloquedTimeout;
	self->ai.pers.deadFrame = BOT_DMclass_DeadFrame;

	//available moveTypes for this class
	self->ai.pers.moveTypesMask = (LINK_MOVE|LINK_STAIRS|LINK_FALL|LINK_WATER|LINK_WATERJUMP|LINK_JUMPPAD|LINK_PLATFORM|LINK_TELEPORT|LINK_LADDER|LINK_JUMP|LINK_CROUCH);

	//Persistant Inventory Weights (0 = can not pick)
	memset(self->ai.pers.inventoryWeights, 0, sizeof (self->ai.pers.inventoryWeights));

	//weapons
	self->ai.pers.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_BLASTER].weaponItem)] = 0.0;
	self->ai.pers.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_SWORD].weaponItem)] = 0.0;
	//self->bot.pers.inventoryWeights[ITEM_INDEX(FindItemByClassname("weapon_blaster"))] = 0.0; //it's the same thing
	self->ai.pers.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_20MM].weaponItem)] = 0.0;
	self->ai.pers.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_SHOTGUN].weaponItem)] = 0.5;
	self->ai.pers.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_SUPERSHOTGUN].weaponItem)] = 0.7;
	self->ai.pers.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_MACHINEGUN].weaponItem)] = 0.5;
	self->ai.pers.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_CHAINGUN].weaponItem)] = 0.7;
	self->ai.pers.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_GRENADES].weaponItem)] = 0.5;
	self->ai.pers.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_GRENADELAUNCHER].weaponItem)] = 0.6;
	self->ai.pers.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_ROCKETLAUNCHER].weaponItem)] = 0.8;
	self->ai.pers.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_HYPERBLASTER].weaponItem)] = 0.7;
	self->ai.pers.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_RAILGUN].weaponItem)] = 0.8;
	self->ai.pers.inventoryWeights[ITEM_INDEX(AIWeapons[WEAP_BFG].weaponItem)] = 0.5;

	//ammo
	self->ai.pers.inventoryWeights[shell_index] = 0.5;
	self->ai.pers.inventoryWeights[bullet_index] = 0.5;
	self->ai.pers.inventoryWeights[cell_index] = 0.5;
	self->ai.pers.inventoryWeights[rocket_index] = 0.5;
	self->ai.pers.inventoryWeights[slug_index] = 0.5;
	self->ai.pers.inventoryWeights[grenade_index] = 0.5;
	
	//armor
	self->ai.pers.inventoryWeights[body_armor_index] = 0.9;
	self->ai.pers.inventoryWeights[ITEM_INDEX(FindItemByClassname("item_armor_combat"))] = 0.8;
	self->ai.pers.inventoryWeights[ITEM_INDEX(FindItemByClassname("item_armor_jacket"))] = 0.5;
	self->ai.pers.inventoryWeights[ITEM_INDEX(FindItemByClassname("item_armor_shard"))] = 0.2;

	//techs
	self->ai.pers.inventoryWeights[ITEM_INDEX(FindItemByClassname("tech_resistance"))] = 0.5;
	self->ai.pers.inventoryWeights[ITEM_INDEX(FindItemByClassname("tech_strength"))] = 0.5;
	self->ai.pers.inventoryWeights[ITEM_INDEX(FindItemByClassname("tech_regeneration"))] = 0.5;
	self->ai.pers.inventoryWeights[ITEM_INDEX(FindItemByClassname("tech_haste"))] = 0.5;

	//GHz: misc
	self->ai.pers.inventoryWeights[ITEM_INDEX(FindItemByClassname("item_pack"))] = 0.6;

	if( ctf->value ) {
		redflag = FindItemByClassname("item_flag_team1");	// store pointers to flags gitem_t, for 
		blueflag = FindItemByClassname("item_flag_team2");// simpler comparisons inside this archive
		self->ai.pers.inventoryWeights[ITEM_INDEX(FindItemByClassname("item_flag_team1"))] = 3.0;
		self->ai.pers.inventoryWeights[ITEM_INDEX(FindItemByClassname("item_flag_team2"))] = 3.0;
	}
}

//==========================================
// BOT_DMclass_Pain
//==========================================
void BOT_DMclass_Pain(edict_t* self, edict_t* other, float kick, int damage)
{
	if (!self->ai.is_bot)
		return;
	if (!G_EntIsAlive(other))
		return;
	if (other->flags & FL_GODMODE)
		return;
	if (OnSameTeam(self, other))
		return;

	self->enemy = other;

	if (self->health < (0.4 * self->max_health) && level.time > self->pain_debounce_time)
	{
		float r = random();
		if (r < 0.33)
			gi.sound(self, CHAN_VOICE, gi.soundindex("speech/yell/saveme1.wav"), 1, ATTN_NORM, 0);
		else if (r < 0.66)
			gi.sound(self, CHAN_VOICE, gi.soundindex("speech/yell/saveme1.wav"), 1, ATTN_NORM, 0);
		else
			gi.sound(self, CHAN_VOICE, gi.soundindex("speech/call911.wav"), 1, ATTN_NORM, 0);
		gi.WriteByte(svc_temp_entity);
		gi.WriteByte(TE_TELEPORT_EFFECT);
		gi.WritePosition(self->s.origin);
		gi.multicast(self->s.origin, MULTICAST_PVS);
	}

	if (AIDevel.debugChased && bot_showcombat->value)
	{
		if (other->ai.is_bot)
			safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: is angry at %s!\n",
				self->ai.pers.netname, other->ai.pers.netname);
		else if (other->client)
			safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: is angry at %s!\n",
				self->ai.pers.netname, other->client->pers.netname);
		else
			safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: is angry at %s!\n",
				self->ai.pers.netname, other->classname);
	}
}
