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


//==========================================
// AIDebug_ToogleBotDebug
//==========================================
void AIDebug_ToogleBotDebug(void)
{
	if (AIDevel.debugMode /* || !sv_cheats->integer */)
	{
		safe_bprintf(PRINT_MEDIUM, "BOT: Debug Mode Off\n");
		AIDevel.debugMode = false;
		return;
	}

	//Activate debug mode
	safe_bprintf(PRINT_MEDIUM, "\n======================================\n");
	safe_bprintf(PRINT_MEDIUM, "--==[ D E B U G ]==--\n");
	safe_bprintf(PRINT_MEDIUM, "======================================\n");
	safe_bprintf(PRINT_MEDIUM, "'addnode [nodetype]' -- Add [specified] node to players current location\n");
	safe_bprintf(PRINT_MEDIUM, "'movenode [node] [x y z]' -- Move [node] to [x y z] coordinates\n");
	safe_bprintf(PRINT_MEDIUM, "'findnode' -- Finds closest node\n");
	safe_bprintf(PRINT_MEDIUM, "'removelink [node1 node2]' -- Removes link between two nodes\n");
	safe_bprintf(PRINT_MEDIUM, "'addlink [node1 node2]' -- Adds a link between two nodes\n");
	safe_bprintf(PRINT_MEDIUM, "======================================\n\n");

	safe_bprintf(PRINT_MEDIUM, "BOT: Debug Mode On\n");
	gi.cvar_set("bot_showpath", "1");
	gi.cvar_set("bot_showcombat", "1");
	gi.cvar_set("bot_showlrgoal", "1");
	gi.cvar_set("bot_showsrgoal", "1");
	gi.cvar_set("bot_debugmonster", "1");
	AIDevel.debugMode = true;

}


//==========================================
// AIDebug_SetChased
// Theorically, only one client
//	at the time will chase. Otherwise it will
//	be a really fucked up situation.
//==========================================
void AIDebug_SetChased(edict_t *ent)
{
	int i;
	AIDevel.chaseguy = NULL;
	AIDevel.debugChased = false;

	if (!AIDevel.debugMode /* || !sv_cheats->integer*/)
		return;

	//find if anyone is chasing this bot
	for(i=0;i<game.maxclients+1;i++)
	{
//		AIDevel.chaseguy = game.edicts + i + 1;
		AIDevel.chaseguy = g_edicts + i + 1;
		if(AIDevel.chaseguy->inuse && AIDevel.chaseguy->client){
			if( AIDevel.chaseguy->client->chase_target &&
				AIDevel.chaseguy->client->chase_target == ent)
				break;
		}
		AIDevel.chaseguy = NULL;
	}

	if (!AIDevel.chaseguy)
		return;

	AIDevel.debugChased = true;

}



//=======================================================================
//							NODE TOOLS	
//=======================================================================



//==========================================
// AITools_DrawLine
// Just so I don't hate to write the event every time
//==========================================
/*
void AITools_DrawLine(vec3_t origin, vec3_t dest)
{

	edict_t		*event;
	
	event = G_SpawnEvent ( EV_BFG_LASER, 0, origin );
	event->svflags = SVF_FORCEOLDORIGIN;
	VectorCopy ( dest, event->s.origin2 );

}
*/


//==========================================
// AITools_DrawPath
// Draws the current path (floods as hell also)
//==========================================
static int	drawnpath_timeout;
void AITools_DrawPath(edict_t *self, int node_from, int node_to)
{

	int			count = 0;
	int			pos = 0;

	//don't draw it every frame (flood)
	if (level.time < drawnpath_timeout)
		return;
	drawnpath_timeout = level.time + 4*FRAMETIME;
	if( self->ai.path.goalNode != node_to )
		return;

	//find position in stored path
	while( self->ai.path.nodes[pos] != node_from )
	{
		pos++;
		if( self->ai.path.goalNode == self->ai.path.nodes[pos] )
			return;	//failed
	}
	//gi.dprintf("AITools_DrawPath\n");
	// Now set up and display the path
	while( self->ai.path.nodes[pos] != node_to && count < 32)
	{
		//edict_t		*event;
		
		//event = G_SpawnEvent ( EV_BFG_LASER, 0, nodes[self->ai.path->nodes[pos]].origin );
		//event->svflags = SVF_FORCEOLDORIGIN;
		//VectorCopy ( nodes[self->ai.path->nodes[pos+1]].origin, event->s.origin2 );
		G_DrawDebugTrail(nodes[self->ai.path.nodes[pos + 1]].origin, nodes[self->ai.path.nodes[pos]].origin);//GHz
		//G_Spawn_Splash(TE_LASER_SPARKS, 20, 200, nodes[self->ai.path.nodes[pos]].origin, vec3_origin, nodes[self->ai.path.nodes[pos]].origin);//GHz
		pos++;
		count++;
	}
	//G_Spawn_Splash(TE_LASER_SPARKS, 5, 200, nodes[self->ai.path.nodes[node_to]].origin, vec3_origin, nodes[self->ai.path.nodes[node_to]].origin);//GHz


	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_BFG_EXPLOSION);
	gi.WritePosition(nodes[self->ai.path.nodes[pos]].origin);
	gi.multicast(nodes[self->ai.path.nodes[pos]].origin, MULTICAST_PVS);


}

//==========================================
// AITools_ShowPlinks
// Draws lines from the current node to it's plinks nodes
//==========================================
static int	debugdrawplinks_timeout;
void AITools_ShowPlinks( void )
{
	int		current_node;
	int		plink_node;
	int		i;

	if (AIDevel.plinkguy == NULL || !AIDevel.showPLinks)
	{
		//gi.dprintf("plinkguy invalid or showplinks is false\n");
		return;
	}

	if (!G_EntIsAlive(AIDevel.plinkguy))
	{
		//gi.dprintf("plink guy is dead!\n");
		AIDevel.plinkguy = NULL;
		AIDevel.showPLinks = false;
		debugdrawplinks_timeout = 0;
		return;
	}

	//don't draw it every frame (flood)
	if (level.time < debugdrawplinks_timeout)
	{
		//gi.dprintf("timeout\n");
		return;
	}
	debugdrawplinks_timeout = level.time + 2*FRAMETIME;

	//do it
	current_node = AI_FindClosestReachableNode(AIDevel.plinkguy->s.origin, AIDevel.plinkguy,NODE_DENSITY*1.5,NODE_ALL);
	if (!pLinks[current_node].numLinks)
	{
		//gi.dprintf("no links");
		return;
	}

	for (i=0; i<nav.num_items;i++){
		if (nav.items[i].node == current_node){
			if( !nav.items[i].ent->classname )
				safe_centerprintf(AIDevel.plinkguy, "no classname");
			else
				safe_centerprintf(AIDevel.plinkguy, "%s", nav.items[i].ent->classname);
			break;
		}
	}

	for (i=0; i<pLinks[current_node].numLinks; i++) 
	{
		vec3_t start, end;
		plink_node = pLinks[current_node].nodes[i];
		VectorCopy(nodes[current_node].origin, start);
		VectorCopy(nodes[plink_node].origin, end);
		//AITools_DrawLine(nodes[current_node].origin, nodes[plink_node].origin);
		//G_DrawDebugTrail(nodes[current_node].origin, nodes[plink_node].origin);//GHz
		G_Spawn_Trails(TE_BFG_LASER, start, end);//GHz
		G_Spawn_Splash(TE_LASER_SPARKS, 5, 200, start, vec3_origin, start);//GHz
	}
	if (!(level.framenum % 20))
		safe_centerprintf(AIDevel.plinkguy, "node %d (%s %d) @ %.0f has %d links", current_node, AI_NodeString(nodes[current_node].flags),
			nodes[current_node].area, distance(AIDevel.plinkguy->s.origin, nodes[current_node].origin), pLinks[current_node].numLinks);

}

int AI_AddNode(vec3_t origin, int flagsmask);
void AI_UpdateNodeEdge(int from, int to);
void AITools_SaveNodes(void);

void Cmd_AI_AddNode_f(edict_t* ent)
{
	if (!bot_enable->value)
		return;

	int node, type = 0;
	char* cmd;
	
	if (!ent->myskills.administrator)
	{
		safe_cprintf(ent, PRINT_HIGH, "You need to be an administrator to use this command.\n");
		return;
	}

	cmd = gi.argv(1);
	if (!Q_stricmp(cmd, "botroam") )
	{
		//gi.dprintf("set type to botroam\n");
		type = NODEFLAGS_BOTROAM;
	}
	//else
	//	gi.dprintf("no type\n");

	if ((node = AI_AddNode(ent->s.origin, type)) != -1)
	{
		safe_cprintf(ent, PRINT_HIGH, "Node %d (type: %d) added!\n", node, type);
		memset(pLinks, 0, sizeof(nav_plink_t) * MAX_NODES);//reset pLinks so that they can be regenerated
		AITools_SaveNodes();	
	}
}

// returns true if node at node_index is completely empty/clear (all zero values)
qboolean AI_IsEmptyNode(int node_index)
{
	return (!nodes[node_index].area && !nodes[node_index].flags && VectorEmpty(nodes[node_index].origin));
}

void AI_ClearNode(int node_index)
{
	nodes[node_index].area = 0;
	nodes[node_index].flags = 0;
	VectorClear(nodes[node_index].origin);
}

// returns the number of empty nodes found
int AI_Get_EmptyNodes()
{
	int count=0;

	for (int i = 0; i < nav.num_nodes; i++)
	{
		// found an empty row
		if (AI_IsEmptyNode(i))
			count++;
	}
	//gi.dprintf("there are %d empty nodes\n", count);
	return count;
}

// removes cleared/empty nodes from the nodes list
void AI_RemoveEmptyNodes()
{
	int i, j, count, moved=0;

	count = AI_Get_EmptyNodes();
	for (i = 0; i < nav.num_nodes; i++)
	{
		// found an empty row
		if (!nodes[i].area && !nodes[i].flags && VectorEmpty(nodes[i].origin))
		{
			for (j = i+1; j < nav.num_nodes; j++)
			{
				// found an occupied row
				if (!VectorEmpty(nodes[j].origin))
				{
					// copy data to empty row
					nodes[i].area = nodes[j].area;
					nodes[i].flags = nodes[j].flags;
					VectorCopy(nodes[j].origin, nodes[i].origin);
					//gi.dprintf("%d<-%d: %f %f %f\n", i, j, nodes[j].origin[0], nodes[j].origin[1], nodes[j].origin[2]);
					// then clear the occupied row
					AI_ClearNode(j);
					//nav.num_nodes--;
					moved++;
					break;
				}
			}
		}
	}
	nav.num_nodes -= count;
	//gi.dprintf("AI_RemoveEmptyNodes: removed %d empty nodes, moved %d\n", count, moved);
	AI_Get_EmptyNodes();
}

// return true if the node_index was generated dynamically at map load
// note: NODEFLAGS_SERVERLINK is used for doors, plats, and teleporters
// other entities will have to be identified separately via node flag or other list
qboolean AI_IsMapNode(int node_index)
{
	return nodes[node_index].flags && (nodes[node_index].flags & NODEFLAGS_SERVERLINK);
}

// remove all nodes that are generated dynamically at map load (vs. nodes generated by walking the map)
// note: AI_RemoveEmptyNodes() should be called afterwards to clean up the list (or another function that calls it)
void AI_RemoveMapNodes(void)
{
	int removed=0;
	for (int i = 0; i < nav.num_nodes; i++)
	{
		if (AI_IsMapNode(i))
		{
			AI_ClearNode(i);
			removed++;
		}
	}
	gi.dprintf("%s: Removed %d map-generated nodes\n", __func__, removed);
}

//GHz: nodes for entities are regenerated at map load, so remove them from the list before saving
void AI_RemoveEntNodes()
{
	int i, current_node, removed = 0;
	if (!nav.loaded)
		return; // don't run unless AI_InitNavigationData() cleared nav.ents/broams/items and generated new data

	//gi.dprintf("clearing %d map entity nodes...\n", nav.num_ents);
	for (i = 0; i < nav.num_ents;i++)
	{
		current_node = nav.ents[i].node;
		if (!AI_IsEmptyNode(current_node))
		{
			// clear node data for map entities
			AI_ClearNode(current_node);
			//gi.dprintf("cleared map entity node %d\n", current_node);
			removed++;
		}
	}
	//gi.dprintf("clearing %d broam nodes...\n", nav.num_broams);
	for (i = 0; i < nav.num_broams;i++)
	{
		current_node = nav.broams[i].node;
		if (!AI_IsEmptyNode(current_node))
		{
			// clear node data for map entities
			AI_ClearNode(nav.broams[i].node);
			//gi.dprintf("cleared broam node %d\n", current_node);
			removed++;
		}
	}
	//gi.dprintf("clearing %d item nodes...\n", nav.num_items);
	// note: nodes for items are only generated when a nearby node isn't found, so those nodes may have already been cleared
	for (i = 0; i < nav.num_items;i++)
	{
		current_node = nav.items[i].node;
		if (!AI_IsEmptyNode(current_node)) // check if the node was already cleared so that we don't count duplicates
		{
			// clear node data for map entities
			AI_ClearNode(nav.items[i].node);
			//gi.dprintf("cleared item node %d\n", current_node);
			removed++;
		}
	}
	gi.dprintf("AI_RemoveEntNodes: cleared %d nodes\n", removed);
	AI_RemoveEmptyNodes();
}

void Cmd_AI_RemoveNode_f(edict_t* ent)
{
	int current_node;
	int plink_node;

	if (!bot_enable->value)
		return;

	if ((current_node = AI_FindClosestReachableNode(ent->s.origin, ent, NODE_DENSITY, NODE_ALL)) != -1)
	{
		int i;
		//vec3_t current_org;

		//gi.dprintf("%d nodes in list, last: %d @ %f %f %f\n", 
		//	nav.num_nodes, nav.num_nodes-1, nodes[nav.num_nodes - 1].origin[0], nodes[nav.num_nodes - 1].origin[1], nodes[nav.num_nodes - 1].origin[2]);
		//VectorCopy(nodes[current_node].origin, current_org);
		AI_ClearNode(current_node);
		//gi.dprintf("Removed node %d %f %f %f\n", current_node, current_org[0], current_org[1], current_org[2]);
		safe_cprintf(ent, PRINT_HIGH, "Removed node %d\n", current_node);
		memset(pLinks, 0, sizeof(nav_plink_t) * MAX_NODES);//reset pLinks so that they can be regenerated
		//gi.dprintf("%d nodes in list, last %d @: %f %f %f\n",
		//	nav.num_nodes, nav.num_nodes-1, nodes[nav.num_nodes - 1].origin[0], nodes[nav.num_nodes - 1].origin[1], nodes[nav.num_nodes - 1].origin[2]);
		// note: we don't call AI_RemoveEmptyNodes because it would modify the index #s of map ent/item/etc. nodes
		// instead, we leave the nodes table rows clear and let AITools_SaveNodes call AI_RemoveEmptyNodes after it purges the map ent/items/etc. nodes
		AITools_SaveNodes();
		AI_InitNavigationData();
	}
}
void Cmd_ShowPlinks_f(edict_t* ent)
{
	if (!bot_enable->value)
		return;

	if (!ent->myskills.administrator)
	{
		safe_cprintf(ent, PRINT_HIGH, "You need to be an administrator to use this command.\n");
		return;
	}

	if (AIDevel.showPLinks)
	{
		AIDevel.plinkguy = NULL;
		AIDevel.showPLinks = false;
		safe_cprintf(ent, PRINT_HIGH, "Show Plinks OFF\n");
		return;
	}

	AIDevel.plinkguy = ent;
	AIDevel.showPLinks = true;
	debugdrawplinks_timeout = 0;
	safe_cprintf(ent, PRINT_HIGH, "Show Plinks ON\n");
}


//=======================================================================
//=======================================================================


//==========================================
// AITools_Frame
// Gives think time to the debug tools found
// in this archive (those witch need it)
//==========================================
void AITools_Frame(void)
{
	//debug
	if( AIDevel.showPLinks )
		AITools_ShowPlinks();
}


