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

// return true if point is behind us
qboolean isbehind(edict_t* self, vec3_t point)
{
	vec3_t	vec;
	float	dot;
	vec3_t	forward;

	AngleVectors(self->s.angles, forward, NULL, NULL);
	VectorSubtract(point, self->s.origin, vec);
	VectorNormalize(vec);
	dot = DotProduct(vec, forward);

	if (dot > 0.3)
		return true;

	return false;
}

//==========================================
// AI_FindFarthestHidingNode
// Find the farthest node from the enemy that isn't visible, i.e. a good place to hide!
//==========================================

int AI_FindFarthestHiddenNode(edict_t* ent, int range, int flagsmask)
{
	int			i;
	float		farthest = 0;
	float		dist;
	int			node = -1;
	vec3_t		origin, v;
	trace_t		tr;
	float		rng;
	vec3_t		maxs, mins;
	edict_t* e;

	if (ent->enemy && ent->enemy->inuse)
		e = ent->enemy;
	else
		e = ent;

	VectorSet(mins, -15, -15, -15);
	VectorSet(maxs, 15, 15, 15);
	VectorCopy(e->s.origin, origin);

	// For Ladders, do not worry so much about reachability
	if (flagsmask & NODEFLAGS_LADDER)
	{
		VectorCopy(vec3_origin, maxs);
		VectorCopy(vec3_origin, mins);
	}

	rng = (float)(range * range); // square range for distance comparison (eliminate sqrt)

	for (i = 0; i < nav.num_nodes; i++)
	{
		if (flagsmask == NODE_ALL || nodes[i].flags & flagsmask)
		{
			VectorSubtract(nodes[i].origin, origin, v);

			dist = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];

			if (dist > farthest && dist < rng)
			{
				// make sure the node isn't visible to our enemy!
				if (!G_IsClearPath(ent, MASK_BOTSOLID, e->s.origin, nodes[i].origin))
				{
					node = i;
					farthest = dist;
				}
			}
		}
	}
	return node;
}

//==========================================
// AI_FindClosestHidingNode
// Find the closest node behind the player that isn't currently visible to our enemy, i.e. a good place to hide!
//==========================================
int AI_FindClosestHiddenNode(edict_t *ent, int range, int flagsmask)
{
	int			i;
	float		closest = 99999;
	float		dist;
	int			node = -1;
	vec3_t		origin, v;
	trace_t		tr;
	float		rng;
	vec3_t		maxs, mins;
	edict_t* e;

	VectorSet(mins, -15, -15, -15);
	VectorSet(maxs, 15, 15, 15);
	VectorCopy(ent->s.origin, origin);

	// For Ladders, do not worry so much about reachability
	if (flagsmask & NODEFLAGS_LADDER)
	{
		VectorCopy(vec3_origin, maxs);
		VectorCopy(vec3_origin, mins);
	}

	rng = (float)(range * range); // square range for distance comparison (eliminate sqrt)

	for (i = 0; i < nav.num_nodes; i++)
	{
		if (flagsmask == NODE_ALL || nodes[i].flags & flagsmask)
		{
			VectorSubtract(nodes[i].origin, origin, v);

			dist = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];

			if (dist < closest && dist < rng)
			{
				if (ent->enemy && ent->enemy->inuse)
					e = ent->enemy;
				else
					e = ent;
				// make sure the node is behind us and isn't visible to our enemy!
				if (isbehind(ent, nodes[i].origin) && !G_IsClearPath(ent, MASK_BOTSOLID, e->s.origin, nodes[i].origin))
				{
					node = i;
					closest = dist;
				}
			}
		}
	}
	return node;
}

//==========================================
// AI_FindCost
// Determine cost of moving from one node to another
//==========================================
int AI_FindCost(int from, int to, int movetypes)
{
	astarpath_t	path;

	if (!AStar_GetPath(from, to, movetypes, &path))
		return -1;

	return path.numNodes;
}

//==========================================
// AI_FindClosestReachableNode
// Find the closest node to the player within a certain range
//==========================================
int AI_FindClosestReachableNode(vec3_t origin, edict_t *passent, int range, int flagsmask)
{
	int			i;
	float		closest = 99999;
	float		dist;
	int			node = -1;
	vec3_t		v;
	trace_t		tr;
	float		rng;
	vec3_t		maxs, mins;

	VectorSet(mins, -15, -15, -15);
	VectorSet(maxs, 15, 15, 15);

	// For Ladders, do not worry so much about reachability
	if (flagsmask & NODEFLAGS_LADDER)
	{
		VectorCopy(vec3_origin, maxs);
		VectorCopy(vec3_origin, mins);
	}

	rng = (float)(range * range); // square range for distance comparison (eliminate sqrt)

	for (i = 0; i<nav.num_nodes; i++)
	{
		if (flagsmask == NODE_ALL || nodes[i].flags & flagsmask)
		{
			VectorSubtract(nodes[i].origin, origin, v);

			dist = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];

			if (dist < closest && dist < rng)
			{
				// make sure it is visible
				tr = gi.trace(origin, mins, maxs, nodes[i].origin, passent, MASK_AISOLID);
				if (tr.fraction == 1.0)
				{
					node = i;
					closest = dist;
				}
			}
		}
	}
	return node;
}

//==========================================
// AI_SetupPath //jabot092
//==========================================
int AI_SetupPath(edict_t *self, int from, int to, int movetypes)
{
	if (!AStar_GetPath(from, to, movetypes, &self->ai.path))
		return -1;

	return self->ai.path.numNodes;
}

//==========================================
// AI_SetGoal
// set the goal //jabot092
//==========================================
void AI_SetGoal(edict_t *self, int goal_node, qboolean LongRange)
{
	int			node;

	//AI_DebugPrintf("AI_SetGoal()\n");

	if (LongRange)
	{
		self->ai.lrgoal_node = goal_node;
	}

	self->ai.goal_node = goal_node;
	node = AI_FindClosestReachableNode(self->s.origin, self, NODE_DENSITY * 3, NODE_ALL);

	if (node == -1) {
		AI_SetUpMoveWander(self);
		return;
	}

	//------- ASTAR -----------
	if (!AI_SetupPath(self, node, goal_node, self->ai.pers.moveTypesMask))
	{
		AI_SetUpMoveWander(self);
		return;
	}
	self->ai.path_position = 0;
	self->ai.current_node = self->ai.path.nodes[self->ai.path_position];
	//-------------------------

		if(AIDevel.debugChased && bot_showlrgoal->value)
			safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: GOAL: new START NODE selected %d\n", self->ai.pers.netname, node);

	self->ai.next_node = self->ai.current_node; // make sure we get to the nearest node first
	self->ai.node_timeout = 0;
}


//==========================================
// AI_FollowPath
// Move closer to goal by pointing the bot to the next node
// that is closer to the goal //jabot092 (path-> to path.)
//==========================================
qboolean AI_FollowPath(edict_t *self)
{
	vec3_t			v;
	float			dist;
	float			goal_reached = 32;//GHz

	//AI_DebugPrintf("AI_FollowPath()\n");

	// Show the path
	if (bot_showpath->value)
	{
		if (AIDevel.debugChased)
			AITools_DrawPath(self, self->ai.current_node, self->ai.goal_node);
	}

	if (self->ai.goal_node == INVALID)
		return false;

	// Try again?
	if (self->ai.node_timeout++ > 30) // frames we've tried to reach next node
	{
		AI_DebugPrintf("AI_FollowPath: couldn't follow path, %d tries\n", self->ai.tries);
		if (self->ai.tries++ > 3) // number of attempts we've tried to grab a new start node
			return false; // after so many attempts, return false and wander instead because we can't reach the nearest node
		else
			AI_SetGoal(self, self->ai.goal_node, false);
	}

	//AI_DebugPrintf("AI_FollowPath: node_timeout %d tries %d\n", self->ai.node_timeout, self->ai.tries);

	// Are we there yet?
	VectorSubtract(self->s.origin, nodes[self->ai.next_node].origin, v);
	dist = VectorLength(v);

	//special lower plat reached check
	if (dist < 64
		&& nodes[self->ai.current_node].flags & NODEFLAGS_PLATFORM
		&& nodes[self->ai.next_node].flags & NODEFLAGS_PLATFORM
		&& self->groundentity && self->groundentity->use == Use_Plat)
		dist = 16;

	if (self->ai.is_bunnyhop && !self->groundentity)
		goal_reached = 64; //GHz: bot is likely to jump over node positions, so allow more room

	if ((dist < goal_reached && nodes[self->ai.next_node].flags != NODEFLAGS_JUMPPAD && nodes[self->ai.next_node].flags != NODEFLAGS_TELEPORTER_IN)
		|| (self->ai.status.jumpadReached && nodes[self->ai.next_node].flags & NODEFLAGS_JUMPPAD)
		|| (self->ai.status.TeleportReached && nodes[self->ai.next_node].flags & NODEFLAGS_TELEPORTER_IN))
	{
		// reset timeout
		self->ai.node_timeout = 0;

		if (self->ai.next_node == self->ai.goal_node)
		{
			self->ai.evade_delay = 0;//GHz: reset delay--we've (hopefully) successfully evaded the enemy

			if(AIDevel.debugChased && bot_showlrgoal->value)
				safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: GOAL REACHED!\n", self->ai.pers.netname);

			//if botroam, setup a timeout for it
			if (nodes[self->ai.goal_node].flags & NODEFLAGS_BOTROAM)
			{
				int		i;
				for (i = 0; i<nav.num_broams; i++) {	//find the broam
					if (nav.broams[i].node != self->ai.goal_node)
						continue;

					if(AIDevel.debugChased && bot_showlrgoal->value)
						safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: BotRoam Time Out set up for node %i\n", self->ai.pers.netname, nav.broams[i].node);
					//Com_Printf("%s: BotRoam Time Out set up for node %i\n", self->ai.pers.netname, nav.broams[i].node);
					self->ai.status.broam_timeouts[i] = level.time + 15.0;
					break;
				}
			}

			// Pick a new goal
			AI_PickLongRangeGoal(self);
		}
		else
		{
			self->ai.current_node = self->ai.next_node;
			self->ai.next_node = self->ai.path.nodes[self->ai.path_position++];
		}
	}


	if (self->ai.current_node == -1 || self->ai.next_node == -1)
		return false;

	// Set bot's movement vector
	VectorSubtract(nodes[self->ai.next_node].origin, self->s.origin, self->ai.move_vector);
	// save the vector between current and next node to later help us determine if the bot is on-course or not
	VectorSubtract(nodes[self->ai.next_node].origin, nodes[self->ai.current_node].origin, self->ai.link_vector);//GHz
	return true;
}