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


nav_plink_t pLinks[MAX_NODES];		// pLinks array - links or connections between nodes; used for pathfinding (determining distance/cost between nodes)
nav_node_t nodes[MAX_NODES];		// nodes array
ai_navigation_t	nav;

//ACE


typedef struct
{
	edict_t		*ent;

	qboolean	was_falling;
	int			last_node;

}player_dropping_nodes_t;
player_dropping_nodes_t	player;


void AI_CategorizePosition (edict_t *ent);


//===========================================================
//
//				EDIT NODES
//
//===========================================================

edict_t *AI_PlayerDroppingNodesPassent(void)
{
	return player.ent;
}


//==========================================
// AI_AddNode
// Add a node of normal navigation types.
// Not valid to add nodes from entities nor items
//==========================================
int AI_AddNode( vec3_t origin, int flagsmask )
{
	if (nav.num_nodes + 1 > MAX_NODES)
		return -1;

	if( flagsmask & NODEFLAGS_WATER )
		flagsmask |= NODEFLAGS_FLOAT;

	VectorCopy( origin, nodes[nav.num_nodes].origin );
	if( !(flagsmask & NODEFLAGS_FLOAT) )
		AI_DropNodeOriginToFloor( nodes[nav.num_nodes].origin, player.ent );

	//if( !(flagsmask & NODEFLAGS_NOWORLD) ) {	//don't spawn inside solids
	//	trace_t	trace;
	//	trace = gi.trace( nodes[nav.num_nodes].origin, tv(-15, -15, -8), tv(15, 15, 8), nodes[nav.num_nodes].origin, player.ent, MASK_NODESOLID );
	//	if( trace.startsolid )
	//		return -1;
	//}

	nodes[nav.num_nodes].flags = flagsmask;
	nodes[nav.num_nodes].flags |= AI_FlagsForNode( nodes[nav.num_nodes].origin, player.ent );

	//gi.dprintf("Dropped Node\n");
	//if( sv_cheats->value )
	//	Com_Printf("Dropped Node\n");

	nav.num_nodes++;
	return nav.num_nodes-1; // return the node added
}



//==========================================
// AI_UpdateNodeEdge
// Add/Update node connections (paths)
//==========================================
void AI_UpdateNodeEdge( int from, int to )
{
	int	link;

	if(from == -1 || to == -1 || from == to)
		return; // safety


	if(AI_PlinkExists(from, to))
	{
		link = AI_PlinkMoveType(from, to);
	} else
		link = AI_FindLinkType( from, to );

	//Com_Printf("Link: %d -> %d. ", from, to);
	//Com_Printf("%s\n", AI_LinkString(link) );
	//if( sv_cheats->value )
	//	Com_Printf("Link: %d -> %d. %s\n", from, to, AI_LinkString(link) );
}



//===========================================================
//
//			PLAYER DROPPING NODES
//
//===========================================================


//==========================================
// AI_DropLadderNodes
// drop nodes all along the ladder
//==========================================
void AI_DropLadderNodes( edict_t *self )
{
	vec3_t	torigin;
	vec3_t	borigin;
	vec3_t	droporigin;
	int		step;
	trace_t trace;

	AI_DebugPrintf("AI_DropLadderNodes\n");

	//find top & bottom of the ladder
	VectorCopy( self->s.origin, torigin );
	VectorCopy( self->s.origin, borigin );

	while( AI_IsLadder( torigin, self->client->ps.viewangles, self->mins, self->maxs, self) )
	{
		torigin[2]++;
	}
	torigin[2] += (self->mins[2] + 8);

	//drop node on top
	AI_AddNode( torigin, (NODEFLAGS_LADDER|NODEFLAGS_FLOAT) );

	//find bottom. Try simple first
	trace = gi.trace( borigin, tv(-15,-15,-24), tv(15,15,0), tv(borigin[0], borigin[1], borigin[2] - 2048), self, MASK_NODESOLID );
	if( !trace.startsolid && trace.fraction < 1.0 
		&& AI_IsLadder( trace.endpos, self->client->ps.viewangles, self->mins, self->maxs, self) ) 
	{
		VectorCopy( trace.endpos, borigin );

	} else {	//it wasn't so easy
		
		trace = gi.trace( borigin, tv(-15,-15,-25), tv(15,15,0), borigin, self, MASK_NODESOLID );
		while( AI_IsLadder( borigin, self->client->ps.viewangles, self->mins, self->maxs, self) 
			&& !trace.startsolid ) 
		{
			borigin[2]--;
			trace = gi.trace( borigin, tv(-15,-15,-25), tv(15,15,0), borigin, self, MASK_NODESOLID );
		}

		//if trace never reached solid, put the node on the ladder
		if( !trace.startsolid )
			borigin[2] -= self->mins[2];
	}

	//drop node on bottom
	AI_AddNode( borigin, (NODEFLAGS_LADDER|NODEFLAGS_FLOAT) );

	if( torigin[2] - borigin[2] < NODE_DENSITY )
		return;

	//make subdivisions and add nodes in between
	step = NODE_DENSITY*0.8;
	VectorCopy( borigin, droporigin );
	droporigin[2] += step;
	while ( droporigin[2] < torigin[2] - 32 )
	{
		AI_AddNode( droporigin, (NODEFLAGS_LADDER|NODEFLAGS_FLOAT) );
		droporigin[2] += step;
	}
}


//==========================================
// AI_CheckForLadder
// Check for adding ladder nodes
//==========================================
qboolean AI_CheckForLadder(edict_t *self)
{
	int			closest_node;

	AI_DebugPrintf("AI_CheckForLadder\n\n");

	// If there is a ladder and we are moving up, see if we should add a ladder node
	if ( self->velocity[2] < 5 )
		return false;

	if ( !AI_IsLadder(self->s.origin, self->client->ps.viewangles, self->mins, self->maxs, self) )
		return false;

	// If there is already a ladder node in here we've already done this ladder
	closest_node = AI_FindClosestReachableNode( self->s.origin, self, NODE_DENSITY, NODEFLAGS_LADDER );
	if(closest_node != -1)
		return false;

	//proceed:
	AI_DropLadderNodes( self );
	return true;
}


//==========================================
// AI_TouchWater
// Capture when players touches water for mapping purposes.
//==========================================
void AI_WaterJumpNode( void )
{
	int			closest_node;
	vec3_t		waterorigin;
	trace_t		trace;
	edict_t		ent;

	AI_DebugPrintf("AI_WaterJumpNode\n");

	//don't drop if player is riding elevator or climbing a ladder
	if( player.ent->groundentity && player.ent->groundentity != world) {
		if( player.ent->groundentity->classname ) {
			if( !strcmp( player.ent->groundentity->classname, "func_plat")
				|| !strcmp(player.ent->groundentity->classname, "trigger_push")
				|| !strcmp(player.ent->groundentity->classname, "func_train")
				|| !strcmp(player.ent->groundentity->classname, "func_rotate")
				|| !strcmp(player.ent->groundentity->classname, "func_bob")
				|| !strcmp(player.ent->groundentity->classname, "func_door") )
				return;
		}
	}
	if( AI_IsLadder( player.ent->s.origin, player.ent->client->ps.viewangles, player.ent->mins, player.ent->maxs, player.ent) )
		return;

	VectorCopy( player.ent->s.origin, waterorigin );

	//gi.dprintf("player[2] %0.f ", waterorigin[2]);

	//move the origin to water limit
	if( gi.pointcontents(waterorigin) & MASK_WATER )//GHz: player is in the water (at our origin)
	{
		//reverse
		trace = gi.trace( waterorigin, 
			vec3_origin, 
			vec3_origin,
			tv( waterorigin[0], waterorigin[1], waterorigin[2] + NODE_DENSITY*2 ),//GHz: trace up to 256 units above bot
			player.ent,
			MASK_ALL );
		
		VectorCopy( trace.endpos, waterorigin );
		if( gi.pointcontents(waterorigin) & MASK_WATER )//GHz: still in the water?
			return;
	}
	
	//gi.dprintf("water[2]+ %0.f ", waterorigin[2]);

	//find water limit
	trace = gi.trace( waterorigin, 
		vec3_origin, 
		vec3_origin,
		tv( waterorigin[0], waterorigin[1], waterorigin[2] - NODE_DENSITY*2 ),//GHz: trace down 256 units from upper limit
		player.ent,
		MASK_WATER );
	
	//gi.dprintf("water[2]- %0.f\n", trace.endpos[2]);

	if( trace.fraction == 1.0 )
		return;
	else
		VectorCopy( trace.endpos, waterorigin );
	
	//GHz: commented out lines below due to infinite loop
	// 
	//tmp test (should just move 1 downwards)
	if( !(gi.pointcontents(waterorigin) & MASK_WATER) ) {
	//	int	k = 0;
	//	while( !(gi.pointcontents(waterorigin) & MASK_WATER) ){
			waterorigin[2]--;
	//		k++;
	//	}
	}

	//gi.dprintf("water final[2] %0.f\n", waterorigin[2]);

	ent = *player.ent;
	VectorCopy( waterorigin, ent.s.origin);

	// Look for the closest node of type water
	closest_node = AI_FindClosestReachableNode( ent.s.origin, &ent, 32, NODEFLAGS_WATER);
	if( closest_node == -1 ) // we need to drop a node
	{
		closest_node = AI_AddNode( waterorigin, (NODEFLAGS_WATER|NODEFLAGS_FLOAT) );
		
		// Add an edge
		AI_UpdateNodeEdge( player.last_node, closest_node);
		player.last_node = closest_node;
	
	} else {

		AI_UpdateNodeEdge( player.last_node, closest_node );
		player.last_node = closest_node; // zero out so other nodes will not be linked
	}
}


//==========================================
// AI_PathMap
// This routine is called to hook in the pathing code and sets
// the current node if valid.
//==========================================
static float last_update=0;
#define NODE_UPDATE_DELAY	0.10;
void AI_PathMap( void )
{
	int			 closest_node;

	AI_DebugPrintf("AI_PathMap\n");

	//DROP WATER JUMP NODE (not limited by delayed updates)
	if ( !player.ent->ai.is_swim && player.last_node != -1 
		&& player.ent->ai.is_swim != player.ent->ai.was_swim) 
	{
		AI_WaterJumpNode();
		last_update = level.time + NODE_UPDATE_DELAY; // slow down updates a bit
		return;
	}

	if( level.time < last_update )
		return;

	last_update = level.time + NODE_UPDATE_DELAY; // slow down updates a bit

	//don't drop nodes when riding movers
	if( player.ent->groundentity && player.ent->groundentity != world) {
		if( player.ent->groundentity->classname ) {
			if( !strcmp( player.ent->groundentity->classname, "func_plat")
				|| !strcmp(player.ent->groundentity->classname, "trigger_push")
				|| !strcmp(player.ent->groundentity->classname, "func_train")
				|| !strcmp(player.ent->groundentity->classname, "func_rotate")
				|| !strcmp(player.ent->groundentity->classname, "func_bob")
				|| !strcmp(player.ent->groundentity->classname, "func_door") )
				return;
		}
	}

	// Special check for ladder nodes
	if(AI_CheckForLadder(player.ent))
		return;

	// Not on ground, and not in the water, so bail (deeper check by using a splitmodels function)
	if (!player.ent->ai.is_step )
	{
		if ( !player.ent->ai.is_swim ){
			player.was_falling = true;
			return;
		}
		else if ( player.ent->ai.is_swim )
			player.was_falling = false;
	}

	//player just touched the ground
	if ( player.was_falling == true)
	{
		if ( !player.ent->groundentity ) //not until it REALLY touches ground
			return;

		//normal nodes

		//check for duplicates (prevent adding too many)
		closest_node = AI_FindClosestReachableNode( player.ent->s.origin, player.ent, 64, NODE_ALL);
		
		//otherwise, add a new node
		if(closest_node == INVALID) 
			closest_node = AI_AddNode( player.ent->s.origin, 0 ); //no flags = normal movement node
		
		// Now add link
		if( player.last_node != -1 && closest_node != -1)
			AI_UpdateNodeEdge( player.last_node, closest_node);

		if( closest_node != -1 )
			player.last_node = closest_node; // set visited to last
		
		player.was_falling = false;
		return;
	}


	//jal: I'm not sure of not having nodes in lava/slime
	//being actually a good idea. When the bots incidentally
	//fall inside it they don't know how to get out
	// Lava/Slime


	// Iterate through all nodes to make sure far enough apart
	closest_node = AI_FindClosestReachableNode( player.ent->s.origin, player.ent, NODE_DENSITY, NODE_ALL );

	// Add Nodes as needed
	if( closest_node == INVALID )
	{
		// Add nodes in the water as needed
		if( player.ent->ai.is_swim )
			closest_node = AI_AddNode( player.ent->s.origin, (NODEFLAGS_WATER|NODEFLAGS_FLOAT) );
		else
			closest_node = AI_AddNode( player.ent->s.origin, 0 );

		// Now add link
		if( player.last_node != -1)
			AI_UpdateNodeEdge( player.last_node, closest_node );
	}
	else if( closest_node != player.last_node && player.last_node != INVALID )
		AI_UpdateNodeEdge( player.last_node, closest_node );

	if( closest_node != -1 )
		player.last_node = closest_node; // set visited to last
}


//==========================================
// AI_ClientPathMap
// Clients try to create new nodes while walking the map
//==========================================
void AITools_AddBotRoamNode(void)
{
	if( nav.loaded )
		return;

	AI_AddNode( player.ent->s.origin, NODEFLAGS_BOTROAM );
}


//==========================================
// AI_ClientPathMap
// Clients try to create new nodes while walking the map
//==========================================
void AITools_DropNodes(edict_t *ent)
{
	/*if( nav.loaded )
		return;
	*/
	AI_DebugPrintf("AITools_DropNodes\n");
	AI_CategorizePosition (ent);
	player.ent = ent;
	AI_PathMap();
}


//==========================================
//
//==========================================
void AITools_EraseNodes( void )
{
	//Init nodes arrays
	nav.num_nodes = 0;
	memset( nodes, 0, sizeof(nav_node_t) * MAX_NODES );
	memset( pLinks, 0, sizeof(nav_plink_t) * MAX_NODES );

	nav.num_ents = 0;
	memset( nav.ents, 0, sizeof(nav_ents_t) * MAX_EDICTS );

	nav.num_broams = 0;
	memset( nav.broams, 0, sizeof(nav_broam_t) * MAX_BOT_ROAMS );

	nav.num_items = 0;
	memset( nav.items, 0, sizeof(nav_item_t) * MAX_EDICTS );

	nav.loaded = false;
}

void AITools_InitEditnodes( void )
{
	if(nav.loaded) {
		AITools_EraseNodes();
		AI_LoadPLKFile( level.mapname );
		//delete everything but nodes
		memset( pLinks, 0, sizeof(nav_plink_t) * MAX_NODES );
		
		nav.num_ents = 0;
		memset( nav.ents, 0, sizeof(nav_ents_t) * MAX_EDICTS );
		
		nav.num_broams = 0;
		memset( nav.broams, 0, sizeof(nav_broam_t) * MAX_BOT_ROAMS );
		
		nav.num_items = 0;
		memset( nav.items, 0, sizeof(nav_item_t) * MAX_EDICTS );
		nav.loaded = false;
	}
	
	Com_Printf("EDITNODES: on\n");
}

void AITools_InitMakenodes( void )
{
	if(nav.loaded)
		AITools_EraseNodes();

	Com_Printf("EDITNODES: on\n");
}


//-------------------------------------------------------------


//==========================================
// AI_SavePLKFile
// save nodes and plinks to file.
// Only navigation nodes are saved. Item nodes aren't
//==========================================
qboolean AI_SavePLKFile( char *mapname )
{
	FILE		*pOut;
	char		filename[MAX_OSPATH];
	int			i;
	int			version = NAV_FILE_VERSION;
	cvar_t *game_dir = gi.cvar("game_dir", "vortex", 0);

	Com_sprintf (filename, sizeof(filename), "%s/%s/%s.%s", AI_MOD_FOLDER, AI_NODES_FOLDER, mapname, NAV_FILE_EXTENSION );
	pOut = fopen (filename, "wb");
	if (!pOut)
		return false;

	fwrite(&version,sizeof(int),1,pOut);
	fwrite(&nav.num_nodes,sizeof(int),1,pOut);

	// write out nodes
	for(i=0; i<nav.num_nodes;i++)
		fwrite(&nodes[i],sizeof(nav_node_t),1,pOut);

	// write out plinks array
	for(i=0; i<nav.num_nodes;i++)
		fwrite(&pLinks[i],sizeof(nav_plink_t),1,pOut);

	fclose(pOut);

	return true;
}


//===========================================================
//
//				EDITOR
//
//===========================================================

int AI_LinkCloseNodes_JumpPass( int start );
//==================
// AITools_SaveNodes
//==================
void AITools_SaveNodes( void )
{
	int newlinks;
	int	jumplinks;

	if( !nav.num_nodes ) {
		Com_Printf("CGame AITools: No nodes to save\n");
		return;
	}

	//find links
	newlinks = AI_LinkCloseNodes();
	Com_Printf ("       : Added %i new links\n", newlinks);

	//find jump links
	jumplinks = AI_LinkCloseNodes_JumpPass(0);
	Com_Printf ("       : Added %i new jump links\n", jumplinks);

	if( !AI_SavePLKFile( level.mapname ) )
		Com_Printf ("       : Failed: Couldn't create the nodes file\n");
	else
		Com_Printf ("       : Nodes files saved\n");

	//restart navigation
	AITools_EraseNodes();
	AI_InitNavigationData();
}

