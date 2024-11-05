
#include "g_local.h"
#include "ai_local.h"

//==========================================
// Related data:
// pLinks - links or connections between nodes loaded from file that are used for pathfinding (determining distance/cost between nodes)
// nodes - data structures loaded from file that have map coordinates of nodes
//==========================================

static int	alist[MAX_NODES];	//list contains indexes (node numbers) of all studied nodes, Open and Closed together
static int	alist_numNodes;		//number of nodes in list

enum {
	NOLIST,		//unprocessed nodes to be added to OPENLIST
	OPENLIST,	//nodes to explore and then move to CLOSEDLIST
	CLOSEDLIST	//nodes explored--stop when end (target/destination) node is added
};

typedef struct
{
	int		parent;	//previous link in path chain (start<-end)
	int		G;		//(pLink) distance between current node and the start node
	int		H;		//heuristic - estimated distance from the current node to the end node

	int		list;	//which list node is currently in, i.e. NOLIST/OPENLIST/CLOSEDLIST

} astarnode_t;

astarnode_t	astarnodes[MAX_NODES]; //working list of nodes being processed by A* algo

// note: Apath is populated by AStar_ListsToPath and this is copied to (astarpath_t)path to determine the cost of path traversal via AI_FindCost
static int Apath[MAX_NODES]; //array of node#s (node indexes) from start to end
static int Apath_numNodes; //number of nodes in path

//==========================================
// 
// 
//==========================================
static int originNode; //starting node closest to origin of search and beginning of path
static int goalNode; //ending node closest to destination
static int currentNode; //current node being processed by A* algo

int ValidLinksMask;
#define DEFAULT_MOVETYPES_MASK (LINK_MOVE|LINK_STAIRS|LINK_FALL|LINK_WATER|LINK_WATERJUMP|LINK_JUMPPAD|LINK_PLATFORM|LINK_TELEPORT);
//==========================================
// 
// 
// 
//==========================================

int	AStar_nodeIsInPath( int node )
{
	int	i;

	if( !Apath_numNodes )
		return 0;

	for (i=0; i<Apath_numNodes; i++) 
	{
		if(node == Apath[i])
			return 1;
	}

	return 0;
}

int	AStar_nodeIsInClosed( int node )
{
	if( astarnodes[node].list == CLOSEDLIST )
		return 1;

	return 0;
}

int	AStar_nodeIsInOpen( int node )
{
	if( astarnodes[node].list == OPENLIST )
		return 1;

	return 0;
}

//==========================================
// Reset values of astarnodes, Apath, and alist
// Moves all nodes to NOLIST
// Run before a new path is generated
//==========================================
static void AStar_InitLists (void)
{
	int i;

	for ( i=0; i<MAX_NODES; i++ )
	{
		Apath[i] = 0;

		astarnodes[i].G = 0;
		astarnodes[i].H = 0;
		astarnodes[i].parent = 0;
		astarnodes[i].list = NOLIST;
	}
	Apath_numNodes = 0;

	alist_numNodes = 0;
	memset( alist, -1, sizeof(alist));//jabot092
}

static int AStar_PLinkDistance(int n1, int n2)
{
	int	i;

	for ( i=0; i<pLinks[n1].numLinks; i++)
	{
		if( pLinks[n1].nodes[i] == n2 )
			return (int)pLinks[n1].dist[i];
	}

	return -1;
}

static int	Astar_HDist_ManhatanGuess( int node )
{
	vec3_t	DistVec;
	int		i;
	int		HDist;

	//teleporters are exceptional
	if( nodes[node].flags & NODEFLAGS_TELEPORTER_IN )
		node++; //it's tele out is stored in the next node in the array

	for (i=0 ; i<3 ; i++)
	{
		DistVec[i] = fabs(nodes[goalNode].origin[i] - nodes[node].origin[i]);
	}

	HDist = (int)(DistVec[0] + DistVec[1] + DistVec[2]);
	return HDist;
}

static void AStar_PutInClosed( int node )
{
	if( !astarnodes[node].list ) {
		alist[alist_numNodes] = node;
		alist_numNodes++;
	}

	astarnodes[node].list = CLOSEDLIST;
}

static void AStar_PutAdjacentsInOpen(int node)
{
	int	i;

	for ( i=0; i<pLinks[node].numLinks; i++)
	{
		int	addnode;

		//ignore invalid links
		if( !(ValidLinksMask & pLinks[node].moveType[i]) )
			continue;

		addnode = pLinks[node].nodes[i];

		//ignore self
		if( addnode == node )
			continue;

		//ignore if it's already in closed list
		if( AStar_nodeIsInClosed( addnode ) )
			continue;

		//if it's already inside open list
		if( AStar_nodeIsInOpen( addnode ) )
		{
			int plinkDist;
			
			plinkDist = AStar_PLinkDistance( node, addnode );
			if( plinkDist == -1)
				printf("WARNING: AStar_PutAdjacentsInOpen - Couldn't find distance between nodes\n");
			
			//compare G distances and choose best parent
			else if( astarnodes[addnode].G > (astarnodes[node].G + plinkDist) )
			{
				astarnodes[addnode].parent = node;
				astarnodes[addnode].G = astarnodes[node].G + plinkDist;
			}
			
		} else {	//just put it in

			int plinkDist;

			plinkDist = AStar_PLinkDistance( node, addnode );
			if( plinkDist == -1)
			{
				plinkDist = AStar_PLinkDistance( addnode, node );
				if( plinkDist == -1) 
					plinkDist = 999;//jalFIXME

				//ERROR
				printf("WARNING: AStar_PutAdjacentsInOpen - Couldn't find distance between nodes\n");
			}

			//put in global list
			if( !astarnodes[addnode].list ) {
				alist[alist_numNodes] = addnode;
				alist_numNodes++;
			}

			astarnodes[addnode].parent = node;
			astarnodes[addnode].G = astarnodes[node].G + plinkDist;
			astarnodes[addnode].H = Astar_HDist_ManhatanGuess( addnode );
			astarnodes[addnode].list = OPENLIST;
		}
	}
}

static int AStar_FindInOpen_BestF ( void )
{
	int	i;
	int	bestF = -1;
	int best = -1;

	for ( i=0; i<alist_numNodes; i++ )
	{
		int node = alist[i];

		if( astarnodes[node].list != OPENLIST )
			continue;

		if ( bestF == -1 || bestF > (astarnodes[node].G + astarnodes[node].H) ) {
			bestF = astarnodes[node].G + astarnodes[node].H;
			best = node;
		}
	}
	//gi.dprintf("BEST:%i\n", best);
	return best;
}

//==========================================
// Called after A* path has been computed on astarnodes[]
// Works backwards from end (goalNode) and copies parent node indices to Apath[]
// Apath[] will then contain a list of node#s (indexes) for the path from start to end
//==========================================
static void AStar_ListsToPath ( void )
{
	int count = 0;
	int cur = goalNode;
	qboolean Spath_added = false;//GHz

	while ( cur != originNode ) 
	{
		cur = astarnodes[cur].parent;
		count++;
	}
	cur = goalNode;
	
	while ( count >= 0 ) 
	{
		Apath[count] = cur;
		Apath_numNodes++;
		//GHz
		// save to list to speed up future searches
		if (Spath_numNodes < MAX_SPATH)
		{
			Spath[Spath_numNodes].path[count] = cur;
			Spath[Spath_numNodes].numNodes++;
			Spath_added = true;
		}
		//GHz
		count--;
		cur = astarnodes[cur].parent;
	}
	//GHz
	if (Spath_added)
		Spath_numNodes++;
	//GHz
}

// copies saved path (Spath.path) data to Apath
// Spath_index is the index in Spath with the data you want copied to Apath
// path_index_start is the starting index in Spath.path with the data you want copied
// path_index_end is the ending index in Spath.path
// path_index_start and path_index_end tell the function which subset of the path you want copied
static void AStar_UseSavedPath(int Spath_index, int path_index_start, int path_index_end)
{
	int i, cur = 0, len = abs(path_index_end - path_index_start) + 1;

	if (path_index_start > path_index_end)
	{
		// starting index is higher than ending index, so copy backwards from this index
		cur = path_index_start;
		for (i = 0;i < len;i++)
		{
			Apath[i] = Spath[Spath_index].path[cur];
			Apath_numNodes++;
			cur--;
		}
	}
	else
	{
		cur = path_index_start;
		for (i = 0;i < len;i++)
		{
			Apath[i] = Spath[Spath_index].path[cur];
			Apath_numNodes++;
			cur++;
		}
	}
	//gi.dprintf("AStar_UseSavedPath\n");
}

// searches saved path array for a path that contains start and end nodes
// if found, it copies the indexes to Apath and returns true
static qboolean AStar_FindSavedPath(int start_node, int end_node)
{
	int i, j;

	for (i = 0;i < Spath_numNodes;i++)
	{
		int start_index = 0, end_index = 0;
		qboolean found_start = false, found_end = false;
		for (j = 0;j < Spath[i].numNodes;j++)
		{
			if (Spath[i].path[j] == start_node)
			{
				start_index = j;
				found_start = true;
			}
			else if (Spath[i].path[j] == end_node)
			{
				end_index = j;
				found_end = true;
			}
			if (found_start && found_end 
				&& end_index > start_index) //GHz: reverse order is disabled for now because jump links would be impossible to do backwards!
			{
				//if (start_index != 0)
				//	gi.dprintf("saved path found in subset of data!\n");
				//gi.dprintf("saved path found! index: %d, start: %d end: %d %d->%d\n", i, start_index, end_index, start_node, end_node);
					AStar_UseSavedPath(i, start_index, end_index);
				return true;
			}
		}
	}
	return false;
}

static int	AStar_FillLists ( void )
{
	//put current node inside closed list
	AStar_PutInClosed( currentNode );
	
	//put adjacent nodes inside open list
	AStar_PutAdjacentsInOpen( currentNode );
	
	//find best adjacent and make it our current
	currentNode = AStar_FindInOpen_BestF();

	return (currentNode != -1);	//if -1 path is bloqued
}

int	AStar_ResolvePath ( int n1, int n2, int movetypes )
{
	//int i=0;//GHz
	ValidLinksMask = movetypes;

	if ( !ValidLinksMask )
		ValidLinksMask = DEFAULT_MOVETYPES_MASK;

	AStar_InitLists();

	if (AStar_FindSavedPath(n1, n2))//GHz
		return 1;

	originNode = n1;
	goalNode = n2;
	currentNode = originNode;

	while ( !AStar_nodeIsInOpen(goalNode) )
	{
		//i++;//GHz
		if( !AStar_FillLists() )
			return 0;	//failed
	}

	AStar_ListsToPath();

	//GHz: debugging A* perf issues
	//gi.dprintf("RESULT:\n Origin:%i\n Goal:%i\n numNodes:%i\n FirstInPath:%i\n LastInPath:%i\n", originNode, goalNode, Apath_numNodes, Apath[0], Apath[Apath_numNodes-1]);
	//gi.dprintf("AStar_FillLists() ran %d times\n", i);
	//gi.dprintf("Spath[%d]: start:%d end:%d length:%d\n", 
	//	Spath_numNodes-1, Spath[Spath_numNodes-1].path[0], Spath[Spath_numNodes-1].path[Spath[Spath_numNodes - 1].numNodes-1], Spath[Spath_numNodes-1].numNodes);

	return 1;
}

int AStar_GetPath( int origin, int goal, int movetypes, struct astarpath_s *path )
{
	int i;

	if( !AStar_ResolvePath ( origin, goal, movetypes ) )
		return 0;

	path->numNodes = Apath_numNodes;
	path->originNode = origin;
	path->goalNode = goal;
	for(i=0; i<path->numNodes; i++)
		path->nodes[i] = Apath[i];

	return 1;
}

