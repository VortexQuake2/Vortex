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
ai_devel_t	AIDevel;

//==========================================
// AI_Init
// Inits global parameters
//==========================================
void AI_Init(void)
{
	//Init developer mode
	bot_showpath = gi.cvar("bot_showpath", "0", CVAR_SERVERINFO);
	bot_showcombat = gi.cvar("bot_showcombat", "0", CVAR_SERVERINFO);
	bot_showsrgoal = gi.cvar("bot_showsrgoal", "0", CVAR_SERVERINFO);
	bot_showlrgoal = gi.cvar("bot_showlrgoal", "0", CVAR_SERVERINFO);
	bot_debugmonster = gi.cvar("bot_debugmonster", "0", CVAR_SERVERINFO|CVAR_ARCHIVE);

	AIDevel.debugMode = false;
	AIDevel.debugChased = false;
	AIDevel.chaseguy = NULL;
	AIDevel.showPLinks = false;
	AIDevel.plinkguy = NULL;
}

//==========================================
// AI_NewMap
// Inits Map local parameters
//==========================================
void AI_NewMap(void)
{
	//Load nodes
	AI_InitNavigationData();
	AI_InitAIWeapons ();
}


//==========================================
// AI_SetUpMoveWander
//==========================================
void AI_SetUpMoveWander( edict_t *ent )
{
	AI_DebugPrintf("AI_SetupMoveWander()\n");

	if (AIDevel.debugChased && bot_showlrgoal->value)
		safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: couldn't find a path. Bot will wander instead.\n", ent->client->pers.netname);//GHz

	ent->ai.state = BOT_STATE_WANDER;
	ent->ai.wander_timeout = level.time + GetRandom(1, 3);//GHz: randomized--previously 1.0
	ent->ai.nearest_node_tries = 0;
	
	ent->ai.next_move_time = level.time;
	ent->ai.bloqued_timeout = level.time + 15.0;
	
	ent->ai.goal_node = INVALID;
	ent->ai.current_node = INVALID;
	ent->ai.next_node = INVALID;
}


//==========================================
// AI_ResetWeights
// Init bot weights from bot-class weights.
//==========================================
void AI_ResetWeights(edict_t *ent)
{
	AI_DebugPrintf("AI_ResetWeights()\n");

	//restore defaults from bot persistant
	memset(ent->ai.status.inventoryWeights, 0, sizeof (ent->ai.status.inventoryWeights));
	memcpy(ent->ai.status.inventoryWeights, ent->ai.pers.inventoryWeights, sizeof(ent->ai.pers.inventoryWeights));
}


//==========================================
// AI_ResetNavigation
// Init bot navigation. Called at first spawn & each respawn
//==========================================
void AI_ResetNavigation(edict_t *ent)
{
	int		i;

	AI_DebugPrintf("AI_ResetNavigation()\n");

	ent->enemy = NULL;
	ent->movetarget = NULL;
	ent->ai.state_combat_timeout = 0.0;

	ent->ai.state = BOT_STATE_WANDER;
	ent->ai.wander_timeout = level.time;
	ent->ai.nearest_node_tries = 0;

	ent->ai.next_move_time = level.time;
	ent->ai.bloqued_timeout = level.time + 15.0;

	ent->ai.goal_node = INVALID;
	ent->ai.current_node = INVALID;
	ent->ai.next_node = INVALID;
	
	VectorSet( ent->ai.move_vector, 0, 0, 0 );

	//reset bot_roams timeouts
	for( i=0; i<nav.num_broams; i++)
		ent->ai.status.broam_timeouts[i] = 0.0;
}


//==========================================
// AI_BotRoamForLRGoal
//
// Try assigning a bot roam node as LR Goal
//==========================================
qboolean AI_BotRoamForLRGoal(edict_t *self, int current_node)
{
	int		i;
	float	cost;
	float	weight, best_weight = 0.0;
	int		goal_node = INVALID;
	int		best_broam = INVALID;
	float	dist;

	if (!nav.num_broams)
		return false;

	AI_DebugPrintf("AI_BotRoamForLRGoal()\n");

	for( i=0; i<nav.num_broams; i++)
	{
		if( self->ai.status.broam_timeouts[i] > level.time)
			continue;

		//limit cost finding by distance
		dist = AI_Distance( self->s.origin, nodes[nav.broams[i].node].origin );
		if( dist > 10000 )
			continue;

		//find cost
		cost = AI_FindCost(current_node, nav.broams[i].node, self->ai.pers.moveTypesMask);
		if(cost == INVALID || cost < 3) // ignore invalid and very short hops
			continue;

		cost *= random(); // Allow random variations for broams
		weight = nav.broams[i].weight / cost;	// Check against cost of getting there

		if(weight > best_weight)
		{
			best_weight = weight;
			goal_node = nav.broams[i].node;
			best_broam = i;
		}
	}

	if(best_weight == 0.0 || goal_node == INVALID)
		return false;

	//set up the goal
	self->ai.state = BOT_STATE_MOVE;
	self->ai.tries = 0;	// Reset the count of how many times we tried this goal

	if(AIDevel.debugChased && bot_showlrgoal->value)
		safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: selected a bot roam of weight %f at node %d for LR goal.\n",self->ai.pers.netname, nav.broams[best_broam].weight, goal_node);

	AI_SetGoal(self,goal_node, true);

	return true;
}

// gitem_t->flags
#define	IT_WEAPON		1		// use makes active weapon
#define	IT_AMMO			2
#define IT_ARMOR		4
#define IT_STAY_COOP	8
#define IT_KEY			16
#define IT_POWERUP		32
//ZOID
#define IT_TECH			64
//ZOID

//==========================================
// AI_PickLongRangeGoal
//
// Evaluate the best long range goal and send the bot on
// its way. This is a good time waster, so use it sparingly. 
// Do not call it for every think cycle.
//
// jal: I don't think there is any problem by calling it,
// now that we have stored the costs at the nav.costs table (I don't do it anyway)
//==========================================
void AI_PickLongRangeGoal(edict_t *self)
{
	int		i;
	int		node;
	float	weight,best_weight=0.0;
	int		current_node, goal_node = INVALID;
	edict_t *goal_ent = NULL;
	float	cost;
	float dist;

	AI_DebugPrintf("AI_PickLongRangeGoal()\n");

	// look for a target
	current_node = AI_FindClosestReachableNode(self->s.origin, self,((1+self->ai.nearest_node_tries)*NODE_DENSITY),NODE_ALL);
	self->ai.current_node = current_node;

	if(current_node == -1)	//failed. Go wandering :(
	{
		if (AIDevel.debugChased && bot_showlrgoal->value)
			safe_cprintf (AIDevel.chaseguy, PRINT_HIGH, "%s: LRGOAL: Closest node not found. Tries:%i\n", self->ai.pers.netname, self->ai.nearest_node_tries);

		if( self->ai.state != BOT_STATE_WANDER )
			AI_SetUpMoveWander( self );

		self->ai.wander_timeout = level.time + 1.0;
		self->ai.nearest_node_tries++;	//extend search radius with each try
		return;
	}
	self->ai.nearest_node_tries = 0;


	// Items
	for(i=0;i<nav.num_items;i++)
	{
		// Ignore items that are not there (solid)
		if(!nav.items[i].ent || nav.items[i].ent->solid == SOLID_NOT)
			continue;

		//ignore items wich can't be weighted (must have a valid item flag)
		if( !nav.items[i].ent->item || !(nav.items[i].ent->item->flags & (IT_AMMO|IT_TECH|IT_HEALTH|IT_ARMOR|IT_WEAPON|IT_POWERUP|IT_FLAG)) )
			continue;

		weight = AI_ItemWeight(self, nav.items[i].ent);
		if( weight == 0.0f )	//ignore zero weighted items
			continue;

		//limit cost finding distance
		dist = AI_Distance( self->s.origin, nav.items[i].ent->s.origin );

		//different distance limits for different types
		if( nav.items[i].ent->item->flags & (IT_AMMO|IT_TECH) && dist > 2000 )
			continue;

		if( nav.items[i].ent->item->flags & (IT_HEALTH|IT_ARMOR|IT_POWERUP) && dist > 4000 )
			continue;

		if( nav.items[i].ent->item->flags & (IT_WEAPON|IT_FLAG) && dist > 10000 )
			continue;

		cost = AI_FindCost(current_node, nav.items[i].node, self->ai.pers.moveTypesMask);
		if(cost == INVALID || cost < 3) // ignore invalid and very short hops
			continue;

		//weight *= random(); // Allow random variations
		weight /= cost; // Check against cost of getting there

		if(weight > best_weight)
		{
			best_weight = weight;
			goal_node = nav.items[i].node;
			goal_ent = nav.items[i].ent;
		}
	}


	// Players: This should be its own function and is for now just finds a player to set as the goal.
	for( i=0; i<num_AIEnemies; i++ )
	{
		//ignore self & spectators
		if( AIEnemies[i] == self || AIEnemies[i]->svflags & SVF_NOCLIENT)
			continue;

		//ignore zero weighted players
		if( self->ai.status.playersWeights[i] == 0.0f )
			continue;

		node = AI_FindClosestReachableNode( AIEnemies[i]->s.origin, AIEnemies[i], NODE_DENSITY, NODE_ALL);
		cost = AI_FindCost(current_node, node, self->ai.pers.moveTypesMask);

		if(cost == INVALID || cost < 4) // ignore invalid and very short hops
			continue;

		//precomputed player weights
		weight = self->ai.status.playersWeights[i];

		//weight *= random(); // Allow random variations
		weight /= cost; // Check against cost of getting there

		if(weight > best_weight)
		{		
			best_weight = weight;
			goal_node = node;
			goal_ent = AIEnemies[i];
		}
	}

	// If do not find a goal, go wandering....
	if(best_weight == 0.0 || goal_node == INVALID)
	{
		//BOT_ROAMS
		if (!AI_BotRoamForLRGoal(self, current_node))
		{
			self->ai.goal_node = INVALID;
			self->ai.state = BOT_STATE_WANDER;
			self->ai.wander_timeout = level.time + 1.0;
			if(AIDevel.debugChased && bot_showlrgoal->value)
				safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: did not find a LR goal, wandering.\n",self->ai.pers.netname);
		}
		return; // no path?
	}

	// OK, everything valid, let's start moving to our goal.
	self->ai.state = BOT_STATE_MOVE;
	self->ai.tries = 0;	// Reset the count of how many times we tried this goal

	if(goal_ent != NULL && AIDevel.debugChased && bot_showlrgoal->value)
		safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: selected a %s at node %d for LR goal.\n",self->ai.pers.netname, goal_ent->classname, goal_node);

	AI_SetGoal(self,goal_node, true);
}


//==========================================
// AI_PickShortRangeGoal
// Pick best goal based on importance and range. This function
// overrides the long range goal selection for items that
// are very close to the bot and are reachable.
//==========================================
void AI_PickShortRangeGoal(edict_t *self)
{
	edict_t *target;
	float	weight,best_weight=0.0;
	edict_t *best = NULL;
	float disttolr = 8192;

	if( !self->client )
		return;

	AI_DebugPrintf("AI_PickShortRangeGoal()\n");

	// look for a target (should make more efficent later)
	target = findradius(NULL, self->s.origin, AI_GOAL_SR_RADIUS);

	while(target)
	{
		if(target->classname == NULL)
			return;
		
		// Missile detection code
		if(strcmp(target->classname,"rocket")==0 || strcmp(target->classname,"grenade")==0)
		{
			//if player who shoot is a potential enemy
			if (self->ai.status.playersWeights[target->owner->s.number-1])
			{
				if(AIDevel.debugChased && bot_showcombat->value)
					safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: ROCKET ALERT!\n", self->ai.pers.netname);
				
				self->enemy = target->owner;	// set who fired the rocket as enemy
				return;
			}
		}
		
		//GHz: search for nearby items
		if (AI_IsItem(target) && AI_ItemIsReachable(self,target->s.origin))
		{
			//if (infront(self, target))
			{
				weight = AI_ItemWeight(self, target);

				// We now modify the weight for proximity to LR goal.

				if (self->ai.lrgoal_node != INVALID)
				{
					if (AI_Distance(target->s.origin, nodes[self->ai.lrgoal_node].origin) < disttolr)
						disttolr = AI_Distance(target->s.origin, nodes[self->ai.lrgoal_node].origin);

					weight *= (1 / disttolr) * 2;
				}
				
				if(weight > best_weight)
				{
					best_weight = weight;
					best = target;
				}
			}
		}
		
		// next target
		target = findradius(target, self->s.origin, AI_GOAL_SR_RADIUS);	
	}
	
	//jalfixme (what's goalentity doing here?)
	if(best_weight)//GHz: was a short-range goal found?
	{
		self->movetarget = best;
		self->goalentity = best;
		if(AIDevel.debugChased && bot_showsrgoal->value 
			/* && (self->goalentity != self->movetarget)*/) //GHz: not sure this makes sense w/preceding line
			safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: selected a %s for SR goal.\n",self->ai.pers.netname, self->movetarget->classname);
	}
}


//===================
//  AI_CategorizePosition
//  Categorize waterlevel and groundentity/stepping
//===================
void AI_CategorizePosition (edict_t *ent)
{
	qboolean stepping = AI_IsStep(ent);

	AI_DebugPrintf("AI_CategorizePosition()\n");

	ent->ai.was_swim = ent->ai.is_swim;
	ent->ai.was_step = ent->ai.is_step;

	ent->ai.is_ladder = AI_IsLadder( ent->s.origin, ent->s.angles, ent->mins, ent->maxs, ent );

	M_CatagorizePosition(ent);
	if (ent->waterlevel > 2 || ent->waterlevel && !stepping) {
		ent->ai.is_swim = true;
		ent->ai.is_step = false;
		return;
	}

	ent->ai.is_swim = false;
	ent->ai.is_step = stepping;
}


//==========================================
// AI_Think
// think funtion for AIs
//==========================================
void AI_Think (edict_t *self)
{
	AIDebug_SetChased(self);	//jal:debug shit
	AI_CategorizePosition(self);

	//freeze AI when dead
	if( self->deadflag ) {
		self->ai.pers.deadFrame(self);
		return;
	}

	//if completely stuck somewhere
	if(VectorLength(self->velocity) > 37)
		self->ai.bloqued_timeout = level.time + 10.0;

	if( self->ai.bloqued_timeout < level.time ) {
		self->ai.pers.bloquedTimeout(self);
		return;
	}

	//update status information to feed up ai
	self->ai.pers.UpdateStatus(self);

	//update position in path, set up move vector
	if( self->ai.state == BOT_STATE_MOVE ) {
		
		if( !AI_FollowPath(self) )
		{
			AI_SetUpMoveWander( self );
			//gi.dprintf("will wander instead\n");
			//self->ai.wander_timeout = level.time - 1;	//do it now //GHz: commented this line out because it conflicts with timeout set in preceding func call
		}
	}

	//pick a new long range goal
	if (self->ai.state == BOT_STATE_WANDER && self->ai.wander_timeout < level.time)
	{
		//gi.dprintf("searching for long range goal\n");
		AI_PickLongRangeGoal(self);
	}

	//Find any short range goal
	AI_PickShortRangeGoal(self);

	//run class based states machine
	self->ai.pers.RunFrame(self);
}


