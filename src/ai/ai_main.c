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
	bot_enable = gi.cvar("bot_enable", "0", CVAR_SERVERINFO|CVAR_LATCH);//GHz
	bot_dropnodes = gi.cvar("bot_dropnodes", "0", CVAR_SERVERINFO);//GHz
	bot_autospawn = gi.cvar("bot_autospawn", "0", CVAR_SERVERINFO|CVAR_LATCH);//GHz
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

void BOT_AutoSpawn(void);
//==========================================
// AI_NewMap
// Inits Map local parameters
//==========================================
void AI_NewMap(void)
{
	if (!bot_enable->value)
	{
		gi.dprintf("AI: bots are disabled.\n");
		return;
	}
	//Load nodes
	AI_InitNavigationData();
	AI_InitAIWeapons();
	AI_InitEnemiesList();//GHz
	AI_InitAIAbilities();//GHz
	BOT_AutoSpawn();//GHz
}

qboolean AI_SetupMoveAttack(edict_t* self)
{
	int current_node, goal_node;

	// what are we doing here? nobody to attack/chase
	if (!self->enemy || !self->enemy->inuse)
		return false;

	// we're already on the move
	if (self->ai.state == BOT_STATE_MOVEATTACK && self->ai.goal_node)
		return false;

	// attempt to find starting node
	if ((current_node = AI_FindClosestReachableNode(self->s.origin, self, 2 * NODE_DENSITY, NODE_ALL)) == -1)
		return false;

	self->ai.current_node = current_node;

	// attempt to find ending node nearest to enemy
	if ((goal_node = AI_FindClosestReachableNode(self->enemy->s.origin, self, 2 * NODE_DENSITY, NODE_ALL)) == -1)
		return false;

	//set up the goal
	self->ai.state = BOT_STATE_MOVEATTACK;
	self->ai.tries = 0;	// Reset the count of how many times we tried this goal

	if (AIDevel.debugChased && bot_showlrgoal->value)
		safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: is trying to hunt towards node %d!\n", self->ai.pers.netname, goal_node);

	AI_SetGoal(self, goal_node, true);
	//self->ai.attack_delay = level.time + 5.0;// we're trying to run away, so don't try to attack for a bit
	return true;
	
}
//==========================================
// AI_SetUpCombatMovement
// called when AI_PickLongRangeGoal can't find the starting node to path to a LR goal
//==========================================
void AI_SetUpCombatMovement(edict_t* ent)
{
	//AI_DebugPrintf("AI_SetupCombatMovement()\n");
	if (ent->ai.state == BOT_STATE_ATTACK)
		return; // already attacking

	if (AIDevel.debugChased && bot_showlrgoal->value)
		safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: Bot will switch to combat movement.\n", ent->client->pers.netname);//GHz

	ent->ai.state = BOT_STATE_ATTACK;
	//ent->ai.wander_timeout = level.time + GetRandom(1, 3);//GHz: randomized--previously 1.0
	ent->ai.nearest_node_tries = 0;

	ent->ai.next_move_time = level.time;
	ent->ai.bloqued_timeout = level.time + 15.0;

	ent->ai.goal_node = INVALID;
	ent->ai.current_node = INVALID;
	ent->ai.next_node = INVALID;
	ent->ai.linktype = 0;//GHz
	VectorClear(ent->ai.link_vector);//GHz
}

//==========================================
// AI_SetUpMoveWander
//==========================================
void AI_SetUpMoveWander( edict_t *ent )
{
	//AI_DebugPrintf("AI_SetupMoveWander()\n");

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
	ent->ai.linktype = 0;//GHz
	VectorClear(ent->ai.link_vector);//GHz
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
	ent->ai.linktype = 0;//GHz
	VectorClear(ent->ai.link_vector);//GHz
	
	VectorSet( ent->ai.move_vector, 0, 0, 0 );

	//reset bot_roams timeouts
	for( i=0; i<nav.num_broams; i++)
		ent->ai.status.broam_timeouts[i] = 0.0;
}


//==========================================
// AI_BotRoamForLRGoal
//
// Try assigning a bot roam node as LR Goal
// BotRoams are used by the AI when no other LR goal is found
// and give the bot something to do, e.g. walk to different areas of the map
// this is preferrable to aimlessly bouncing from wall to wall like monsters!
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
		if (self->ai.status.broam_timeouts[i] > level.time)
		{
			//gi.dprintf("broam %d timeout\n", i, self->ai.status.broam_timeouts[i]);
			continue;
		}

		//limit cost finding by distance
		dist = AI_Distance( self->s.origin, nodes[nav.broams[i].node].origin );
		if (dist > 10000)
		{
			//gi.dprintf("broam %d too far\n", i);
			continue;
		}

		//find cost
		cost = AI_FindCost(current_node, nav.broams[i].node, self->ai.pers.moveTypesMask);
		if (cost == INVALID || cost < 3) // ignore invalid and very short hops
		{
			//gi.dprintf("broam %d cost %d\n", i, cost);
			continue;
		}

		cost *= random(); // Allow random variations for broams
		weight = nav.broams[i].weight / cost;	// Check against cost of getting there

		if(weight > best_weight)
		{
			best_weight = weight;
			goal_node = nav.broams[i].node;
			best_broam = i;
		}
	}

	if (best_weight == 0.0 || goal_node == INVALID)
	{
		//gi.dprintf("broam best weight %f goal %d\n", best_weight, goal_node);
		return false;
	}


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

qboolean BOT_DMclass_FindSummons(edict_t* self, qboolean moveattack, usercmd_t* ucmd);

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

	//AI_DebugPrintf("AI_PickLongRangeGoal()\n");

	// look for a target
	current_node = AI_FindClosestReachableNode(self->s.origin, self,((1+self->ai.nearest_node_tries)*NODE_DENSITY),NODE_ALL);
	self->ai.current_node = current_node;

	if(current_node == -1)	//failed. Go wandering :(
	{
		if (AIDevel.debugChased && bot_showlrgoal->value)
			safe_cprintf (AIDevel.chaseguy, PRINT_HIGH, "%s: LRGOAL: Closest node not found. Tries:%i\n", self->ai.pers.netname, self->ai.nearest_node_tries);

		if (self->ai.state == BOT_STATE_MOVEATTACK)
			AI_SetUpCombatMovement(self);//GHz
		else if( self->ai.state != BOT_STATE_WANDER && self->ai.state != BOT_STATE_ATTACK)
			AI_SetUpMoveWander( self );

		self->ai.wander_timeout = level.time + 1.0;
		self->ai.nearest_node_tries++;	//extend search radius with each try
		return;
	}
	self->ai.nearest_node_tries = 0;
	/*
	// do we have an enemy? are we a summoner?
	float farthest_dist = 0;
	if (self->enemy && self->enemy->inuse && AI_NumSummons(self) > 0)
	{
		edict_t* farthest_ent = NULL;
		// find a summons that places us farthest away from danger
		float farthest_dist = entdist(self, self->enemy);
		for (edict_t *e = g_edicts; e < &g_edicts[globals.num_edicts]; e++)
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
			// not a long-range goal
			if (visible(self, e) && entdist(self, e) < AI_RANGE_LONG)
				continue;

			// if the summons has an enemy, calculate distance to its enemy
			if (e->enemy && e->enemy->inuse)
				dist = entdist(e->enemy, e);
			// otherwise, calculate distance between it and our enemy
			else
				dist = entdist(self->enemy, e);
			// would moving closer to our summons place us further away from danger?
			if (dist > farthest_dist)
			{
				best_weight = 0.5 + (dist / AI_RANGE_LONG);
				//goal_ent = e;
				farthest_dist = dist;
				farthest_ent = e;
			}
		}
		// found a summons further away from the enemy?
		if (farthest_ent)
		{
			// is there a node nearby to pathfind to?
			if ((goal_node = AI_FindClosestReachableNode(farthest_ent->s.origin, farthest_ent, NODE_DENSITY, NODE_ALL)) != -1)
			{
				// this summons is our goal entity
				goal_ent = farthest_ent;
			}
			else
			{
				// no nearby node, so clear goal entity & weight
				best_weight = 0;
				goal_ent = NULL;
			}
		}
	}*/

	// Items
	for(i=0;i<nav.num_items;i++)
	{
		// Ignore items that are not there (solid)
		if(!nav.items[i].ent || nav.items[i].ent->solid == SOLID_NOT)
			continue;

		//ignore items which can't be weighted (must have a valid item flag)
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
		//sanity check
		if (AIEnemies[i] == NULL)
			continue;
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

		//gi.dprintf("LR enemy goal %s weight %f\n", AIEnemies[i]->classname, weight);

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
			if (self->ai.state == BOT_STATE_MOVEATTACK)//GHz
			{
				AI_SetUpCombatMovement(self);
				return;
			}
			self->ai.goal_node = INVALID;
			self->ai.state = BOT_STATE_WANDER;
			self->ai.wander_timeout = level.time + 1.0;
			if(AIDevel.debugChased && bot_showlrgoal->value)
				safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: did not find a LR goal, wandering.\n",self->ai.pers.netname);
		}
		return; // no path?
	}

	// OK, everything valid, let's start moving to our goal.
	if (self->ai.state != BOT_STATE_MOVEATTACK)//GHz
		self->ai.state = BOT_STATE_MOVE;
	self->ai.tries = 0;	// Reset the count of how many times we tried this goal

	if(goal_ent != NULL && AIDevel.debugChased && bot_showlrgoal->value)
		safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: selected %s (wt %f) @ node %d for LR goal.\n",self->ai.pers.netname, goal_ent->classname, best_weight, goal_node);
	/*if (goal_ent)
	{
		if (AI_IsOwnedSummons(self, goal_ent))
			gi.dprintf("** %d: %s: selected SUMMONS (%s) (wt %f) @ node %d for LR goal.**\n", (int)level.framenum, self->ai.pers.netname, goal_ent->classname, best_weight, goal_node);
		else
			gi.dprintf("%d: %s: selected %s (wt %f) @ node %d for LR goal.\n", (int)level.framenum, self->ai.pers.netname, goal_ent->classname, best_weight, goal_node);
	}*/
	AI_SetGoal(self,goal_node, true);
	self->goalentity = goal_ent;//GHz: used in AI_PickShortRangeGoal to prevent overriding LR goal pathfinding/movement
}

//==========================================
// AI_PickShortRangeGoal
// Pick best goal based on importance and range. This function
// overrides the long range goal selection for items that
// are very close to the bot and are reachable.
//==========================================
void AI_PickShortRangeGoal(edict_t* self)
{
	int weapon;
	float dist, weight, best_weight=0.0;
	edict_t* e;
	edict_t* best = NULL;
	float disttolr = 8192;
	qboolean in_weapon_range=true;

	if (!self->client)
		return;
	// don't pick a new SR target until we've reached the one we're already pursuing
	if (AI_ValidMoveTarget(self, self->movetarget, true))
		return;

	//AI_DebugPrintf("AI_PickShortRangeGoal()\n");

	self->movetarget = NULL;

	if (self->client->pers.weapon)
		weapon = (self->client->pers.weapon->weapmodel & 0xff);
	else
		weapon = 0;

	for (e = g_edicts; e < &g_edicts[globals.num_edicts]; e++)
	{
		if (!e->inuse)
			continue;
		if (e->solid == SOLID_NOT && e->mtype != M_LIGHTNINGSTORM)
			continue;
		if (!e->classname)
			continue;

		dist = entdist(self, e);

		// is this really a short-range goal?
		if (dist > AI_RANGE_LONG)
			continue;

		// is this a weapon projectile?
		if (AI_IsProjectile(e))
		{
			// does it have an owner that is a valid target that isn't a teammate (excluding self)?
			if (e->owner && G_ValidTargetEnt(e->owner, true) && ((e->owner == self && e->radius_dmg) || !OnSameTeam(self, e->owner)))
			{
				//gi.dprintf("%d: AI_PickShortRangeGoal: Detected incoming projectile %s\n", (int)level.framenum, e->classname);
				if (AIDevel.debugChased && bot_showcombat->value)
					safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: WEAPON FIRE INCOMING: %s!\n", self->ai.pers.netname, e->classname);
				// make the bot angry at the owner of the projectile
				if (e->owner != self)
					self->enemy = e->owner;
				self->movetarget = e;
				return; // done searching for short-range goals for now
			}
		}

		weight = 0;

		// are we attacking?
		if (self->ai.state == BOT_STATE_ATTACK && self->enemy && self->enemy->inuse)
		{
			// calculate distance between entity and our enemy
			float enemy_dist = entdist(e, self->enemy);

			// bots should move toward their summons in combat
			if (AI_NumSummons(self) > 0 && AI_IsOwnedSummons(self, e) && dist > AI_RANGE_SHORT && visible(self, e) 
				&& AI_ClearWalkingPath(self, self->s.origin, e->s.origin))
			{
				// assign higher weight to the summons that places us farthest from danger
				weight = 0.5 + enemy_dist / AI_RANGE_LONG;
				//gi.dprintf("found summons that we own, weight: %.1f\n", weight);
			}
			// don't move towards items that place us outside of (usable) weapon range
			else if (AI_GetWeaponRangeWeightByDistance(weapon, enemy_dist) <= 0.1)
				in_weapon_range = false;
		}

		// is this an item we aren't already pursuing (as a long-range goal)?
		if (e != self->goalentity && AI_IsItem(e) && AI_ItemIsReachable(self, e->s.origin) && dist <= AI_GOAL_SR_RADIUS && in_weapon_range)
		{
			weight = AI_ItemWeight(self, e);

			// modify the weight for proximity to long-range goal.
			if (self->ai.lrgoal_node != INVALID)
			{
				if (AI_Distance(e->s.origin, nodes[self->ai.lrgoal_node].origin) < disttolr)
					disttolr = AI_Distance(e->s.origin, nodes[self->ai.lrgoal_node].origin);
				float d2lr = AI_Distance(e->s.origin, nodes[self->ai.lrgoal_node].origin);
				//gi.dprintf("%s: dist %f d2lr %f weight %f\n", target->classname, entdist(self, target), d2lr, weight);
				weight *= (1 / disttolr) * 2;
			}
		}

		if (weight > best_weight)
		{
			best_weight = weight;
			best = e;
		}
	}
	//jalfixme (what's goalentity doing here?)
	if (best_weight > 0)//GHz: was a short-range goal found?
	{
		self->movetarget = best;
		self->goalentity = best;
		if (AIDevel.debugChased && bot_showsrgoal->value)
			safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: selected a %s for SR goal.\n", self->ai.pers.netname, self->movetarget->classname);
		//gi.dprintf("%d: %s: selected %s for SR goal\n", (int)level.framenum, self->ai.pers.netname, self->movetarget->classname);

	}
}

/*
void AI_PickShortRangeGoal(edict_t *self)
{
	edict_t *target;
	float	weight,best_weight=0.0;
	edict_t *best = NULL;
	float disttolr = 8192;

	if( !self->client )
		return;

	//GHz: don't pick a new SR target until we've reached the one we're already pursuing
	if (AI_ValidMoveTarget(self, self->movetarget, true))
		return;
	self->movetarget = NULL;
	//if (self->movetarget && self->movetarget->inuse && entdist(self, self->movetarget) <= AI_GOAL_SR_RADIUS)
	//	return;

	//AI_DebugPrintf("AI_PickShortRangeGoal()\n");

	// look for a target (should make more efficent later)
	target = findradius(NULL, self->s.origin, AI_GOAL_SR_RADIUS);//FIXME: expand radius to 512 for projectiles, then check distance < 200 for items and other SR goals

	while(target)
	{
		//if (!target->inuse)
		//	return;
		//if (target == self)
		//	return;
		if(!target->classname)
			return;
		//if (target == self->goalentity) //GHz: already pursuing this entity as LR goal
		//	return;

		// Missile detection code
		if((strcmp(target->classname,"rocket")==0 || strcmp(target->classname,"grenade")==0) && target->owner 
			&& target->owner->inuse && !OnSameTeam(self, target->owner)) // GHz: don't get angry at teammates
		{
			//if player who shoot is a potential enemy
			if (self->ai.status.playersWeights[target->owner->s.number-1])
			{
				if(AIDevel.debugChased && bot_showcombat->value)
					safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: ROCKET ALERT!\n", self->ai.pers.netname);
				
				self->enemy = target->owner;	// set who fired the rocket as enemy
				self->movetarget = target;//GHz
				return;
			}
		}
		
		//gi.dprintf("%s: ", target->classname);
		//if (AI_IsItem(target))
		//	gi.dprintf("(item=true) ");
		//else
		//	gi.dprintf("(item=false) ");
		//if (AI_ItemIsReachable(self, target->s.origin))
		//	gi.dprintf("(reachable=true)\n");
		//else
		//	gi.dprintf("(reachable=false)\n");

		//GHz: search for nearby items
		if (target != self->goalentity && AI_IsItem(target) && AI_ItemIsReachable(self,target->s.origin))
		{
			//gi.dprintf("found reachable item %s\n", target->classname);
			//if (infront(self, target))
			{
				weight = AI_ItemWeight(self, target);

				// We now modify the weight for proximity to LR goal.

				if (self->ai.lrgoal_node != INVALID)
				{
					if (AI_Distance(target->s.origin, nodes[self->ai.lrgoal_node].origin) < disttolr)
						disttolr = AI_Distance(target->s.origin, nodes[self->ai.lrgoal_node].origin);
					float d2lr = AI_Distance(target->s.origin, nodes[self->ai.lrgoal_node].origin);
					//gi.dprintf("%s: dist %f d2lr %f weight %f\n", target->classname, entdist(self, target), d2lr, weight);

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
			 //&& (self->goalentity != self->movetarget)) //GHz: not sure this makes sense w/preceding line
			)
			safe_cprintf(AIDevel.chaseguy, PRINT_HIGH, "%s: selected a %s for SR goal.\n",self->ai.pers.netname, self->movetarget->classname);
	}
}
*/

//===================
//  AI_CategorizePosition
//  Categorize waterlevel and groundentity/stepping
//===================
void AI_CategorizePosition (edict_t *ent)
{
	qboolean stepping = AI_IsStep(ent);

	//AI_DebugPrintf("AI_CategorizePosition()\n");

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
		self->ai.pers.bloquedTimeout(self); //GHz: do something about it!
		return;
	}

	//update status information to feed up ai
	self->ai.pers.UpdateStatus(self);

	//update position in path, set up move vector
	if( self->ai.state == BOT_STATE_MOVE || self->ai.state == BOT_STATE_MOVEATTACK) {//GHz
		
		if( !AI_FollowPath(self) )
		{
			if (self->ai.state == BOT_STATE_MOVEATTACK)
				AI_SetUpCombatMovement(self);
			else
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


