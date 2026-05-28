#include "entities/grid.h"

#include "g_local.h"

#define MAX_GRID_SIZE	10000

struct mapgrid_s {
    int numnodes;
    vec3_t pathnode[MAX_GRID_SIZE];

    // adjacency list
    int *adjacent_nodes[MAX_GRID_SIZE];
    int adjacent_node_count[MAX_GRID_SIZE];

    int *adjacent_nodes_fly[MAX_GRID_SIZE];
    int adjacent_node_count_fly[MAX_GRID_SIZE];
} mapgrid = {};

struct gridkdtree_s* gridtree = nullptr;

struct pfctx_s {
    struct gstack_s* stack;
    struct nodearena_s* arena;
    node_t *OPEN; // Start of OPEN List
    node_t *CLOSED; // Start of CLOSED List

    // results
    int *waypoints; // Integer array of nodenum's along the path
    int numpts; // Number of nodes in the path..
}* pfctx = nullptr;

void pfctx_free(struct pfctx_s** ptr) {
    if (!*ptr) return;

    nodearena_free(&(*ptr)->arena);
    gstack_free(&(*ptr)->stack);
    if ((*ptr)->waypoints) {
        free((*ptr)->waypoints);
    }

    free(*ptr);
    *ptr = nullptr;
}

struct pfctx_s* pfctx_create(size_t nodecapacity) {
    struct pfctx_s* ctx = malloc(sizeof(struct pfctx_s));
    if (!ctx) {
        return nullptr;
    }
    ctx->stack = gstack_create(nodecapacity);

    if (!ctx->stack)
        goto error;

    ctx->arena = nodearena_create(nodecapacity);

    if (!ctx->arena)
        goto error;

    ctx->OPEN = nullptr;
    ctx->CLOSED = nullptr;
    ctx->waypoints = malloc(nodecapacity * sizeof(int));
    if (!ctx->waypoints) {
        goto error;
    }
    ctx->numpts = 0;
    return ctx;

    error:
    pfctx_free(&ctx);
    return nullptr;
}

void pfctx_reset(struct pfctx_s* ctx) {
    gstack_reset(ctx->stack);
    nodearena_reset(ctx->arena);
    ctx->numpts = 0;
    ctx->OPEN = nullptr;
    ctx->CLOSED = nullptr;
}

#define maxx 512 // 32 units/node x=[0..255]
#define maxy 512
#define maxz 512 // 16 units/node z=[0..511]

#define xevery 16 // 32=8192/maxx
#define yevery 16
#define zevery 16

#define gridz(z) (int)min(max(((z)+4096)*0.06250,0),511)
#define g2v0(x)  (float)min(max((x)*xevery-4096+(xevery*0.5),-4096),4096)
#define g2v1(y)  (float)min(max((y)*yevery-4096+(yevery*0.5),-4096),4096)
#define g2v2(z)  (float)min(max((z)*zevery-4096+(zevery*0.5),-4096),4096)

//================== pathfinding stuff ================
// Check the desired OPEN/CLOSED LIST for NodeNum..
node_t *CheckLIST(node_t *LIST, const int NodeNum) {
    node_t *tNode = LIST->NextNode; // Start of OPEN or CLOSED

    while (tNode)
        if (tNode->nodenum == NodeNum) {
            // My test!!
            return (tNode);
        } // Found it!
        else
            tNode = tNode->NextNode;

    return nullptr; // NodeNum NOT on LIST.
}

void PrintNodes(node_t *Node, const qboolean reverse) {
    const node_t *tNode = Node;

    while (tNode) {
        int nodeNumber = tNode->nodenum;
        if (nodeNumber < 0 || nodeNumber > mapgrid.numnodes)
            nodeNumber = 9999;
        gi.dprintf("%d->", nodeNumber);
        if (reverse)
            tNode = tNode->PrevNode;
        else
            tNode = tNode->NextNode;
    }
    gi.dprintf("(null)\n");
}

// Propagate Old node's values to Nodes on Stack
void PropagateDown(struct gstack_s* stack, node_t *Old) {
    int g, c;

    for (c = 0; c < NUMCHILDS; c++) // parse through Old node children
        if (Old->Child[c])
            if (Old->dist + 1 < Old->Child[c]->dist) {
                Old->Child[c]->dist = g = Old->dist + 1; //FIXME:why is 'g' not initialized? GHZ: added 'g='
                Old->Child[c]->totaldistestimation = g + Old->distestimation;
                Old->Child[c]->PrevNode = Old;
                gstack_push(stack, Old->Child[c]);
            } // Push onto Stack

    while (gstack_top(stack)) {
        // is the stack in use?
        node_t *POPNode = gstack_pop(stack); // grab node from stack
        for (c = 0; c < NUMCHILDS; c++) {
            // parse through all existing POPNOde children
            if (!POPNode->Child[c]) break; // No more valid Child nodes!
            if (POPNode->dist + 1 < POPNode->Child[c]->dist) {
                // update g and f values
                POPNode->Child[c]->dist = g = POPNode->dist + 1;
                POPNode->Child[c]->totaldistestimation = g + POPNode->distestimation;
                POPNode->Child[c]->PrevNode = POPNode;
                gstack_push(stack, POPNode->Child[c]);
            }
        }
    } // Push onto Stack
}

#define vDiff(b,a) sqrt((a[0]*a[0]-b[0]*b[0])+(a[1]*a[1]-b[1]*b[1])+(a[2]*a[2]-b[2]*b[2]))

// Successor Nodes all pushed onto OPEN list
void vrx_pf_push_successors(struct pfctx_s* ctx, node_t *StartNode, const int NodeNumS, const int NodeNumD) {
    int g, c;
    float h;

    // NOTE: NodeNumS is the index of a node that was found by the node searching routine
    // Has NodeNumS been Searched yet?
    // see if this node is already on the OPEN list
    node_t *Old = CheckLIST(ctx->OPEN, NodeNumS);
    if (Old) {
        // node was found on the OPEN list
        // this means the node was found before (as a child of another node)
        // but not yet searched (as a parent node)
        for (c = 0; c < NUMCHILDS; c++) {
            // break on the first available child slot of StartNode
            if (!StartNode->Child[c])
                break;
        }

        // if we found an empty child slot, use it, otherwise use the last one
        StartNode->Child[((c < NUMCHILDS) ? c : (NUMCHILDS - 1))] = Old;

        // have we gone farther with this node than StartNode?
        if (StartNode->dist + 1 < Old->dist) {
            Old->dist = g = StartNode->dist + 1; // make node one step beyond StartNode
            Old->totaldistestimation = g + Old->distestimation; // update total cost
            Old->PrevNode = StartNode; // reverse link to StartNode
        }
        return;
    }

    // Has NodeNumS been searched yet?
    Old = CheckLIST(ctx->CLOSED, NodeNumS);
    if (Old != nullptr) {
        // node has been searched before
        for (c = 0; c < NUMCHILDS; c++)
            if (StartNode->Child[c] == nullptr) break;
        StartNode->Child[((c < NUMCHILDS) ? c : (NUMCHILDS - 1))] = Old;
        if (StartNode->dist + 1 < Old->dist) {
            Old->dist = g = StartNode->dist + 1;
            Old->totaldistestimation = g + Old->distestimation;
            Old->PrevNode = StartNode;
            PropagateDown(ctx->stack, Old);
        }
        return;
    }

    // It is NOT on the OPEN or CLOSED List!!
    // Make Successor a Child of StartNode
    node_t *Successor = nodearena_alloc(ctx->arena);
    Successor->nodenum = NodeNumS;
    Successor->dist = g = StartNode->dist + 1;

    // NOTE: the heuristic estimate of the remaining path from this node
    // to the destination node is given by the difference between the 2
    // vectors.  You can come up with your own estimate..

    Successor->distestimation = h = distanceSqr(mapgrid.pathnode[NodeNumS], mapgrid.pathnode[NodeNumD]);
    Successor->totaldistestimation = g + h;
    Successor->PrevNode = StartNode; // reverse link to StartNode
    Successor->NextNode = nullptr;
    // make all child links of new Successor node nullptr
    for (c = 0; c < NUMCHILDS; c++)
        Successor->Child[c] = nullptr;

    for (c = 0; c < NUMCHILDS; c++)
        if (StartNode->Child[c] == nullptr) break; // Find first empty Child[] of StartNode
    StartNode->Child[((c < NUMCHILDS) ? c : (NUMCHILDS - 1))] = Successor; // make Successor a child of StartNode

    // Insert Successor into OPEN List
    node_t *listcurrent = ctx->OPEN;
    node_t *listsuccessor = ctx->OPEN->NextNode;
    // find node in OPEN list with f-cost greater than Successor node
    while (listsuccessor && (listsuccessor->totaldistestimation < Successor->totaldistestimation)) {
        listcurrent = listsuccessor;
        listsuccessor = listsuccessor->NextNode;
    }
    Successor->NextNode = listsuccessor;
    listcurrent->NextNode = Successor;
    //gi.dprintf("added node %d to the OPEN list\n", Successor->nodenum);
}

vec_t VectorLengthSqr(vec3_t v) {
    float length = 0.0f;

    for (int i = 0; i < 3; i++)
        length += v[i] * v[i];

    return length;
}

// returns node index of node closest to start
int vrx_pf_nearest_node_index(vec3_t start, const float range, const qboolean vis) {
    if (!mapgrid.numnodes)
        return -1;

    auto idx = gridkdtree_query(gridtree, start);
    if (idx == SIZE_MAX)
        return -1;

    return idx;
}

qboolean vrx_pf_nearest_node_location(vec3_t start, vec3_t node_loc, const float range, const qboolean vis) {
    if (!mapgrid.numnodes)
        return false;

    const auto bestNodeNum = gridkdtree_query(gridtree, start);
    if (bestNodeNum == SIZE_MAX)
        return false;

    VectorCopy(mapgrid.pathnode[bestNodeNum], node_loc);
    return true;
}

// Pull FIRST node from OPEN, put on CLOSED
node_t *vrx_pf_pop_next_open_node(struct pfctx_s *ctx) {
    node_t *Node = ctx->OPEN->NextNode; // Pull from FRONT of OPEN list

    if (!Node) {
        // gi.dprintf("OPEN list is empty!\n");
        return nullptr;
    }

    //GHz: wont this break if the OPEN list is empty?
    ctx->OPEN->NextNode = ctx->OPEN->NextNode->NextNode;

    // GHz: OK, the final node list (Best) that is used by FindPath()
    // cannot reference any nodes in other lists, thus you can't have
    // a node appear on the OPEN or CLOSED list and the Best (final) list
    // if (Node->nodenum == NodeNumD || Node->nodenum == NodeNumS)
    //	  return Node; // return the final node but don't put it on the CLOSED list!!

    Node->NextNode = ctx->CLOSED->NextNode;
    ctx->CLOSED->NextNode = Node; // Put at FRONT of CLOSED list

    //gi.dprintf("moved %d to the CLOSED list\n", Node->nodenum);
    return Node; // Return Next Best Node
}

qboolean CheckPath1(vec3_t start, vec3_t end) {
    vec3_t from;
    const edict_t *ignore = nullptr;
    trace_t tr;

    VectorCopy(start, from);

    for (int i = 0; i < 32; i++) {
        tr = gi.trace(from, tv(-16, -16, 0), tv(16, 16, 0), end, ignore, MASK_SOLID);
        // ignore doors
        if (tr.ent && tr.ent->inuse && tr.ent->mtype == FUNC_DOOR) {
            VectorCopy(tr.endpos, from);
            ignore = tr.ent;
        }
        // stop on anything else that is solid
        else
            break;
    }

    if (tr.fraction != 1.0 || tr.startsolid || tr.allsolid || tr.contents & MASK_SOLID)
        return false;
    return true;
}

qboolean CheckPath(vec3_t start, vec3_t end) {
    return CheckPath1(start, end);
}

qboolean CheckPathFly(vec3_t start, vec3_t end) {
    const edict_t *ignore = nullptr;

    // check in an L shape, first going up, then horizontally
    // check vertically, first
    vec3_t top;
    VectorSet(top, start[0], start[1], end[2]);
    trace_t tr = gi.trace(start, tv(-16, -16, 0), tv(16, 16, 0), top, ignore, MASK_SOLID);
    if (tr.fraction != 1.0 || tr.startsolid || tr.allsolid || tr.contents & MASK_SOLID)
        return false;

    // now the second part of the L, on the xy plane
    tr = gi.trace(top, tv(-16, -16, 0), tv(16, 16, 0), end, ignore, MASK_SOLID);
    if (tr.fraction != 1.0 || tr.startsolid || tr.allsolid || tr.contents & MASK_SOLID)
        return false;


    return true;
}

qboolean vrx_pf_is_valid_child_node(vec3_t start, vec3_t v, const int max_2d_distance, const int max_z_delta, enum searchtype_t searchtype) {
    // node should be within +/- 32 units of start on the Z axis
    if (fabs(v[2] - start[2]) > max_z_delta)
        return false;
    // distance check, next node could be anywhere between 128 - 255 units away
    if (Get2dDistance(start, v) >= max_2d_distance)
        return false;
    // basic visibility check
    if (!gi.inPVS(start, v))
        return false;
    // advanced visibility check, make sure the path is wide enough to walk to
    const bool validWalkPath = CheckPath(start, v);
    const bool validFlyPath = searchtype == SEARCHTYPE_FLY && CheckPathFly(start, v);

    return validWalkPath || validFlyPath;
}


int** vrx_pf_adjacency_list(enum searchtype_t searchType) {
    if (searchType == SEARCHTYPE_FLY)
        return mapgrid.adjacent_nodes_fly;
    else
        return mapgrid.adjacent_nodes;
}

int* vrx_pf_adjacency_count(enum searchtype_t searchType) {
    if (searchType == SEARCHTYPE_FLY)
        return mapgrid.adjacent_node_count_fly;
    else
        return mapgrid.adjacent_node_count;
}

// fills gridlist with visible nodes within +/- 32 of start on the Z axis
// returns the number of found nodes
int vrx_pf_compute_adjacent_nodes(const enum searchtype_t searchType, const int NodeNumStart) {
    int i, j, maxZdelta;
    vec3_t start;

    int** adjacency_list = vrx_pf_adjacency_list(searchType);
    int* adjacency_list_count = vrx_pf_adjacency_count(searchType);

    if (adjacency_list[NodeNumStart] != nullptr) // az: don't recompute this
        return adjacency_list_count[NodeNumStart];

    // clear previous gridlist values
    static int adjacencytmp[NUMCHILDS];
    memset(adjacencytmp, -1, sizeof(adjacencytmp));
    VectorCopy(mapgrid.pathnode[NodeNumStart], start);

    if (searchType == SEARCHTYPE_FLY)
        maxZdelta = 8192;
    else
        maxZdelta = 32;

    // fill gridlist with node indices
    for (i = 0, j = 0; i < mapgrid.numnodes && j < NUMCHILDS; i++) {
        // we don't want the start node pointing to itself!
        if (i == NodeNumStart)
            continue;
        if (!vrx_pf_is_valid_child_node(start, mapgrid.pathnode[i], 256, maxZdelta, searchType))
            continue;

        // copy mapgrid.pathnode index to gridlist array and increment the gridlist index/nodes found
        adjacencytmp[j++] = i;
    }

    // az: copy to cache
    adjacency_list[NodeNumStart] = vrx_malloc(sizeof(int) * j, TAG_LEVEL);
    memcpy(adjacency_list[NodeNumStart], adjacencytmp, sizeof(int) * j);
    adjacency_list_count[NodeNumStart] = j;

    return j;
}


//TODO: limit search pattern on X and Y axis?
int vrx_pf_sort_adjacent_nodes(const enum searchtype_t searchType, const int NodeNumStart) {
    vec3_t start;

    VectorCopy(mapgrid.pathnode[NodeNumStart], start);

    const int list_size = vrx_pf_compute_adjacent_nodes(searchType, NodeNumStart);

    int **adjacency_list = vrx_pf_adjacency_list(searchType);

    // nothing to sort!
    if (list_size < 2) {
        //gi.dprintf("nothing to sort\n");
        return list_size;
    }

    // fill gridlist with node indices closest to start
    for (int i = 0; i < list_size; i++) {
        int GLindex = i;
        int index = adjacency_list[NodeNumStart][i]; //i;
        float best = Get2dDistance(mapgrid.pathnode[adjacency_list[NodeNumStart][i]], start);

        for (int j = i + 1; j < list_size; j++) {
            const float dist = Get2dDistance(mapgrid.pathnode[adjacency_list[NodeNumStart][j]], start);
            if (dist < best) {
                index = adjacency_list[NodeNumStart][j]; //j; // stored value
                GLindex = j; // list position of best value
                best = dist;
            }
        }

        // swap node index of current position with node index of closest node
        // this procedure sorts the list and keeps us from finding the same value twice
        const int temp = adjacency_list[NodeNumStart][i];
        adjacency_list[NodeNumStart][i] = index;
        adjacency_list[NodeNumStart][GLindex] = temp;
    }

    return list_size; // return maximum number of valid child nodes found
}


void vrx_pf_compute_successors(struct pfctx_s* ctx, const enum searchtype_t searchType, node_t *PresentNode, const int NodeNumD) {
    // create a sorted list of the closest node indices
    const int maxChilds = vrx_pf_sort_adjacent_nodes(searchType, PresentNode->nodenum);

    for (int i = 0; i < maxChilds; i++) {
        // GHz FIX - added if clause to use the cached gridlist if available
        // this is necessary because SortGridList() is only sorting the cached list, not gridList
        //GHz START
        int** adjacency = vrx_pf_adjacency_list(searchType);
        const int nextNodeIndex = adjacency[PresentNode->nodenum][i];
        //GHz END

        if (nextNodeIndex == -1)
            continue;

        vrx_pf_push_successors(ctx, PresentNode, nextNodeIndex, NodeNumD);
    }
}

int vrx_copy_path_waypoints(int *wp, const int max) {
    int j = max;

    if (j > pfctx->numpts)
        j = pfctx->numpts;

    for (int i = 0; i < j; i++)
        wp[i] = pfctx->waypoints[i];

    return j;
}

void vrx_pf_get_node_position(const int nodenum, vec3_t pos) {
    VectorCopy(mapgrid.pathnode[nodenum], pos);
}

// returns the waypoint index closest to start along the path leading to our final destination (or -1 if list is empty)
int vrx_pf_nearest_waypoint_index_along_path(vec3_t start, int *wp, size_t wpcount) {
    int bestNodeNum = -1;
    float best = INFINITY;

    // get the nodenum for the closest node
    for (int i = 0; i < wpcount; i++) {
        const float dist = distanceSqr(mapgrid.pathnode[wp[i]], start);

        if (dist < best) {
            best = dist;
            bestNodeNum = i;
        }
        //gi.dprintf("%d: node %d dist %f best %d %d %f\n", i, wp[i], dist, wp[bestNodeNum], bestNodeNum, best);//DEBUG: REMOVE THIS!
    }

    return bestNodeNum;
}

int FindPath(const enum searchtype_t searchType, vec3_t start, vec3_t destination) {
    node_t *BestNode;
    int g;
    float h;
    vec3_t tstart, tdest;
    auto ctx = pfctx;
    pfctx_reset(ctx);

    VectorCopy(start, tstart);
    VectorCopy(destination, tdest);

    // Get NodeNum of start vector
    const int startnodenum = vrx_pf_nearest_node_index(tstart, INFINITY, false);
    if (startnodenum == -1) {
        //gi.dprintf("bad nodenum at start\n");
        return 0; // ERROR
    }

    // Get NodeNum of destination vector
    const int endnodenum = vrx_pf_nearest_node_index(tdest, INFINITY, false);
    if (endnodenum == -1) {
        //gi.dprintf("bad nondenum at end\n");
        return 0; // ERROR
    }

    // Allocate OPEN/CLOSED list pointers..
    ctx->OPEN = nodearena_alloc(ctx->arena);
    ctx->OPEN->NextNode = nullptr;

    ctx->CLOSED = nodearena_alloc(ctx->arena);
    ctx->CLOSED->NextNode = nullptr;

    // This is our very first NODE!  Our start vector
    node_t *StartNode = nodearena_alloc(ctx->arena);
    StartNode->nodenum = startnodenum; // starting position nodenum
    StartNode->dist = g = 0; // we haven't gone anywhere yet
    StartNode->distestimation = h = distance(start, destination);
    //fabs(vDiff(start,destination)); // calculate remaining distance (heuristic estimate) GHz - changed to fabs()
    StartNode->totaldistestimation = g + h; // total cost from start to finish
    for (int c = 0; c < NUMCHILDS; c++)
        StartNode->Child[c] = nullptr; // no children for search pattern yet
    StartNode->NextNode = nullptr;
    StartNode->PrevNode = nullptr;

    // next node in open list points to our starting node
    ctx->OPEN->NextNode = BestNode = StartNode; // First node on OPEN list..

    for (;;) {
        BestNode = vrx_pf_pop_next_open_node(ctx); // Get next node from OPEN list
        if (!BestNode) {
            //gi.dprintf("ran out of nodes to search\n");
            return 0; //GHz
        }

        if (BestNode->nodenum == endnodenum) break; // we there yet?
        vrx_pf_compute_successors(ctx, searchType, BestNode, endnodenum);
    } // Search from here..

    if (BestNode == StartNode) {
        return 0;
    }

    BestNode->NextNode = nullptr; // Must tie this off!

    // How many nodes we got?
    node_t *tNode = BestNode;
    int i = 0;
    while (tNode) {
        i++; // How many nodes?
        tNode = tNode->PrevNode;
    }

    if (i <= 2) {
        // Only nodes are Start and End??
        //gi.dprintf("only start and end nodes\n");
        return 0;
    }

    // Let's allocate our own stuff...
    pfctx->numpts = i;

    // Now, we have to assign the nodenum's along
    // this path in reverse order because that is
    // the way the A* algorithm finishes its search.
    // The last best node it visited was the END!
    // So, we copy them over in reverse.. No biggy..

    while (BestNode) {
        pfctx->waypoints[--i] = BestNode->nodenum; //GHz: how/when is this freed?
        BestNode = BestNode->PrevNode;
    }

    // NOTE: At this point, if our numpts returned is not
    // zero, then a path has been found!  To follow this
    // path we simply follow node[Waypoint[i]].origin
    // because Waypoint array is filled with indexes into
    // our node[i] array of valid vectors in the map..
    // We did it!!  Now free the stack and exit..
    return (pfctx->numpts);
}

void G_Spawn_Splash(const int type, const int count, const int color, vec3_t start, vec3_t movdir, vec3_t origin) {
    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(type);
    gi.WriteByte(count);
    gi.WritePosition(start);
    gi.WriteDir(movdir);
    gi.WriteByte(color);
    gi.multicast(origin, MULTICAST_PVS);
}


// NOTE: you may already have this function someplace
void G_Spawn_Trails(const int type, vec3_t start, vec3_t endpos) {
    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(type);
    gi.WritePosition(start);
    gi.WritePosition(endpos);
    gi.multicast(start, MULTICAST_PVS);
}

void DrawPath(edict_t *ent) {
    if (!ent->showPathDebug)
        return;

    if (!ent->monsterinfo.numWaypoints)
        return;

    for (int i = 0; i < ent->monsterinfo.numWaypoints; i++) {
        const int j = ent->monsterinfo.waypoint[i];
#ifndef VRX_REPRO
        G_Spawn_Splash(TE_LASER_SPARKS, 4, 0xdcdddedf, mapgrid.pathnode[j], vec3_origin, mapgrid.pathnode[j]);
#else
        int wpPrev = ent->monsterinfo.waypoint[i-1];
        gire.Draw_Arrow(mapgrid.pathnode[wpPrev], mapgrid.pathnode[j], 1, &rgba_blue, &rgba_blue, FRAMETIME, true);
#endif
    }
}

void DrawPath1(void) {
    if (pfctx->numpts < 0)
        return;

    for (int i = 0; i < pfctx->numpts; i++) {
        int wp = pfctx->waypoints[i];
#ifndef VRX_REPRO
        G_Spawn_Splash(TE_LASER_SPARKS, 4, 0xdcdddedf, mapgrid.pathnode[wp], vec3_origin, mapgrid.pathnode[wp]);
#else
        if (i == 0)
            continue;

        int wpPrev = pfctx->waypoints[i-1];
        gire.Draw_Arrow(mapgrid.pathnode[wpPrev], mapgrid.pathnode[wp], 1, &rgba_blue, &rgba_blue, FRAMETIME, true);
#endif
    }
}


//================== pathfinding stuff ================


// draws the path to the spot the player is aiming at
void DrawPathToAimSpot(edict_t *ent, enum searchtype_t st) {
    vec3_t forward, right, start, offset, end;
    trace_t tr;

    if (!ent->myskills.administrator)
        return;

    // calculate starting position for trace
    AngleVectors(ent->client->v_angle, forward, right, nullptr);
    VectorSet(offset, 0, 7, ent->viewheight - 8);
    P_ProjectSource(ent->client, ent->s.origin, offset, forward, right, start);
    // calculate end position
    VectorMA(start, 8192, forward, end);
    // run trace
    tr = gi.trace(start, nullptr, nullptr, end, ent, MASK_SHOT);
    // find path to the spot we are aiming at
    // get node location nearest to us and our goal
    size_t snode = gridkdtree_query(gridtree, start);
    size_t enode = gridkdtree_query(gridtree, tr.endpos);
    if (snode == SIZE_MAX
        || enode == SIZE_MAX) {
        // can't find nearby nodes
        //gi.dprintf("couldn't find nearby nodes");
        gire.Draw_Circle(tr.endpos, 10, &rgba_red, 0.1, false);
        return;
    }
    VectorCopy(mapgrid.pathnode[snode], start);
    VectorCopy(mapgrid.pathnode[enode], end);

    if (FindPath(st, start, end)) {
        // draw it
        DrawPath1();
        //safe_centerprintf(ent, "success!");
    }
#ifdef VRX_REPRO
    gire.Draw_Circle(start, 10, &rgba_green, 0.1, false);
    gire.Draw_Circle(end, 10, &rgba_blue, 0.1, false);
    gire.Draw_Circle(tr.endpos, 10, &rgba_red, 0.1, false);
#endif
}

// Put this at very top of ClientThink() so you can see
// the nodes created by CreateGrid as you walk around!
// Also, prototype this function above ClientThink()...

// NOTE: max search distance for child links should be set to
// gap specified in nearbyGridNode + x/yevery + min1/max1 (e.g. 128+32+3=163)
void vrx_pf_draw_child_links(edict_t *ent) {
    int count;
    float maxDist;
    vec3_t start, end;
    //	trace_t	tr;

    if (ent->client->showGridDebug == GD_OFF)
        return;

    auto searchType = (ent->client->showGridDebug == GD_CHILD || ent->client->showGridDebug == GD_AIMSPOT) ?
        SEARCHTYPE_WALK : SEARCHTYPE_FLY;

    if (ent->client->showGridDebug == GD_AIMSPOT || ent->client->showGridDebug == GD_AIMSPOT_FLY) {
        DrawPathToAimSpot(ent, searchType);
        return;
    }

    if (ent->client->showGridDebug != GD_CHILD && ent->client->showGridDebug != GD_CHILD_FLY)
        return;

    const int parentNode = gridkdtree_query(gridtree, ent->s.origin);
    VectorCopy(mapgrid.pathnode[parentNode], start);

    for (int i = count = maxDist = 0; i < mapgrid.numnodes; i++) {
        // don't link parent node to itself!
        if (parentNode && i == parentNode)
            continue;

        int zdelta = 32;
        if (searchType == SEARCHTYPE_FLY)
            zdelta = 8192;

        if (!vrx_pf_is_valid_child_node(start, mapgrid.pathnode[i], 256, zdelta, searchType))
            continue;

        // copy node position
        VectorCopy(mapgrid.pathnode[i], end);

        // get distance between parent node and this node
        const float dist = Get2dDistance(start, end);

        // update maximum child link distance
        if (!maxDist || dist > maxDist)
            maxDist = dist;

        // spawn bfg laser trails between parent and child nodes
#ifndef VRX_REPRO
        G_Spawn_Trails(TE_BFG_LASER, start, end);
#else
        gire.Draw_Line(start, end, &rgba_red, FRAMETIME, true);
#endif

        // keep track of the number of child links found
        count++;
    }

    if (!(level.framenum % 20)) {
        const auto s = va("Node %d has %d child links @ %.0f.\n",
                          vrx_pf_nearest_node_index(ent->s.origin, 255, true), count, maxDist);
#ifndef VRX_REPRO
        safe_centerprintf(ent, "%s", s);
#else
        gire.configstring(CONFIG_STORY, s);
#endif
    }
}

void InvalidateGridCache() {
    for (int i = 0; i < mapgrid.numnodes; i++) {
        if (mapgrid.adjacent_nodes[i]) {
            vrx_free(mapgrid.adjacent_nodes[i]);
            mapgrid.adjacent_nodes[i] = nullptr;
            mapgrid.adjacent_node_count[i] = 0;
        }

        if (mapgrid.adjacent_nodes_fly[i]) {
            vrx_free(mapgrid.adjacent_nodes_fly[i]);
            mapgrid.adjacent_nodes_fly[i] = nullptr;
            mapgrid.adjacent_node_count_fly[i] = 0;
        }
    }
}

int GetGridNodes() {
    return mapgrid.numnodes;
}

qboolean vrx_pf_get_random_grid_position(vec3_t pos) {
    if (mapgrid.numnodes < 1)
        return false;

    const int index = GetRandom(0, mapgrid.numnodes - 1);

    VectorCopy(mapgrid.pathnode[index], pos);
    return true;
}

qboolean vrx_pf_get_grid_position(vec3_t pos, int index) {
    if (!index)
        index = 0;

    while (index < mapgrid.numnodes) {
        VectorCopy(mapgrid.pathnode[index], pos);
        return true;
    }
    // reached the end of the list
    return false;
}

void Cmd_GetGridPosition() {
    vec3_t v;

    for (int i = 0; i < 5; i++) {
        if (vrx_pf_get_grid_position(v, i))
            gi.dprintf("%d: %.1f %.1f %.1f\n", i, v[0], v[1], v[2]);
    }
}

void vrx_pf_draw_nearby_grid(edict_t *ent) {
    vec3_t v, forward;

    if (ent->client->showGridDebug == GD_OFF)
        return;

    AngleVectors(ent->s.angles, forward,nullptr,nullptr);

    for (int i = 0; i < mapgrid.numnodes; i++) {
        VectorSubtract(mapgrid.pathnode[i], ent->s.origin, v);
#ifndef VRX_REPRO
        if (VectorLength(v) >= 256) continue; // limit view distance to eliminate overflows
#endif
        if (DotProduct(v, forward) > 0.3) {
            // infront?
            VectorCopy(mapgrid.pathnode[i], v);
#ifndef VRX_REPRO
            v[2] -= 4; // node height
            G_Spawn_Trails(TE_BFG_LASER, mapgrid.pathnode[i], v);
#else
            gire.Draw_Point(v, 4, &rgba_green, FRAMETIME, true);
#endif
        }
    }
}

void vrx_pf_delete_node(const int nodenum) {
    // if this isn't the last node on the list, then copy the
    // vector stored at the end of the array to current position
    if (nodenum != mapgrid.numnodes - 1)
        VectorCopy(mapgrid.pathnode[mapgrid.numnodes-1], mapgrid.pathnode[nodenum]);

    // clear the value stored at the end of the list
    VectorClear(mapgrid.pathnode[mapgrid.numnodes-1]);

    // reduce the size of the list
    mapgrid.numnodes--;
}

void Cmd_DeleteNode_f(edict_t *ent) {
    int nearestNode;

    if (!ent->myskills.administrator)
        return;

    if ((nearestNode = vrx_pf_nearest_node_index(ent->s.origin, 255, true)) != -1)
        vrx_pf_delete_node(nearestNode);

    InvalidateGridCache();

    gridkdtree_free(&gridtree);
    gridtree = gridkdtree_create(mapgrid.pathnode, mapgrid.numnodes);

    safe_cprintf(ent, PRINT_HIGH, "**Closest node deleted (%d nodes total).**\n", mapgrid.numnodes);
}

void Cmd_AddNode_f(edict_t *ent) {
    vec3_t start;

    if (!ent->myskills.administrator)
        return;

    VectorCopy(ent->s.origin, start);
    start[2] -= 8192;
    const trace_t tr = gi.trace(ent->s.origin, nullptr, nullptr, start, nullptr, MASK_SOLID);
    VectorCopy(tr.endpos, start);
    start[2] += 32;
    VectorCopy(start, mapgrid.pathnode[mapgrid.numnodes]);
    mapgrid.numnodes++;

    gridkdtree_free(&gridtree);
    gridtree = gridkdtree_create(mapgrid.pathnode, mapgrid.numnodes);

    InvalidateGridCache();

    safe_cprintf(ent, PRINT_HIGH, "**Node added at current position (%d nodes total).\n**", mapgrid.numnodes);
}

void Cmd_DeleteAllNodes_f(edict_t *ent) {
    if (!ent->myskills.administrator)
        return;

    memset(&mapgrid.pathnode, 0, mapgrid.numnodes * sizeof(vec3_t));
    mapgrid.numnodes = 0;

    safe_cprintf(ent, PRINT_HIGH, "All nodes deleted.\n");
}

void SaveGrid(void) {
    char filename[255];
    FILE *fptr;

    CreateDirIfNotExists(va("%s/settings/grd", gamedir->string));

    Com_sprintf(filename, sizeof(filename), "%s/settings/grd/%s.grd", game_path->string, level.mapname);

    if ((fptr = fopen(filename, "wb")) != nullptr) {
        fwrite(&mapgrid.pathnode[0], sizeof(vec3_t), MAX_GRID_SIZE, fptr);
        WriteInteger(fptr, mapgrid.numnodes);
        fclose(fptr);
        gi.dprintf("Grid successfully saved.\n");
    } else
        gi.dprintf("Unable to save grid file: %s\n", filename);
}

qboolean LoadGrid(void) {
    char filename[255];
    FILE *fptr;

    //memset(&mapgrid.pathnode[0], 0, sizeof(vec3_t));
    Com_sprintf(filename, sizeof(filename), "%s/settings/grd/%s.grd", game_path->string, level.mapname);

    if ((fptr = fopen(filename, "rb")) != nullptr) {
        fread(&mapgrid.pathnode[0], sizeof(vec3_t), MAX_GRID_SIZE, fptr);
        mapgrid.numnodes = ReadInteger(fptr);
        fclose(fptr);

        if (mapgrid.numnodes < 1) {
            gi.dprintf("Grid file is invalid and must be rebuilt.\n");
            return false;
        }
        gi.dprintf("Grid successfully loaded (%d nodes).\n", mapgrid.numnodes);
        return true;
    }

    gi.dprintf("Grid file not found.\n");
    return false;
}

void Cmd_SaveNodes_f(edict_t *ent) {
    if (!ent->myskills.administrator)
        return;

    SaveGrid();

    safe_cprintf(ent, PRINT_HIGH, "Saving nodes...\n", mapgrid.numnodes);
}

void Cmd_LoadNodes_f(edict_t *ent) {
    if (!ent->myskills.administrator)
        return;

    LoadGrid();

    safe_cprintf(ent, PRINT_HIGH, "Loading nodes...\n", mapgrid.numnodes);
}

int AdjustDownward(const edict_t *ignore, vec3_t start) {
    vec3_t endpt;
    VectorSet(endpt, start[0], start[1], -8192);
    trace_t tr = gi.trace(start, tv(-1, -1, 0), tv(1, 1, 0), endpt, ignore, CONTENTS_SOLID);
    tr.endpos[2] += 32;
    return (int) (tr.endpos[2] - start[2]); // return delta, if needed later..
}



int vrx_pf_get_all_children_count(const int parent_nodenum) {
    return vrx_pf_adjacency_count(SEARCHTYPE_FLY)[parent_nodenum];
}

// NOTE: max search distance for child links should be set to
// gap specified in nearbyGridNode + x/yevery + min1/max1 (e.g. 128+32+3=163)
qboolean NearbyGridNode(vec3_t start, const int nodes) {
    for (int i = 0; i < nodes; i++) {
        if (!vrx_pf_is_valid_child_node(start, mapgrid.pathnode[i], 129, 18, SEARCHTYPE_WALK))
            continue;
        return true;
    }
    return false;
}

qboolean CheckBottom(vec3_t pos, vec3_t boxmin, vec3_t boxmax) {
    vec3_t mins, maxs, start, stop;
    trace_t trace;
    int x, y;
    float mid, bottom;

    VectorAdd(pos, boxmin, mins);
    VectorAdd(pos, boxmax, maxs);

    // if all of the points under the corners are solid world, don't bother
    // with the tougher checks
    // the corners must be within 16 of the midpoint
    start[2] = mins[2] - 8;
    for (x = 0; x <= 1; x++)
        for (y = 0; y <= 1; y++) {
            start[0] = x ? maxs[0] : mins[0];
            start[1] = y ? maxs[1] : mins[1];
            if (gi.pointcontents(start) != CONTENTS_SOLID)
                goto realcheck;
        }

    return true; // we got out easy

realcheck:
    start[2] = mins[2];

    // the midpoint must be within 16 of the bottom
    start[0] = stop[0] = (mins[0] + maxs[0]) * 0.5;
    start[1] = stop[1] = (mins[1] + maxs[1]) * 0.5;
    stop[2] = start[2] - 2 * 18;
    trace = gi.trace(start, vec3_origin, vec3_origin, stop, nullptr,MASK_PLAYERSOLID /*MASK_MONSTERSOLID*/);

    if (trace.fraction == 1.0)
        return false;
    mid = bottom = trace.endpos[2];

    // the corners must be within 16 of the midpoint
    for (x = 0; x <= 1; x++)
        for (y = 0; y <= 1; y++) {
            start[0] = stop[0] = x ? maxs[0] : mins[0];
            start[1] = stop[1] = y ? maxs[1] : mins[1];

            trace = gi.trace(start, vec3_origin, vec3_origin, stop, nullptr, MASK_PLAYERSOLID /*MASK_MONSTERSOLID*/);

            if (trace.fraction != 1.0 && trace.endpos[2] > bottom)
                bottom = trace.endpos[2];
            if (trace.fraction == 1.0 || mid - trace.endpos[2] > 18)
                return false;
        }

    // c_yes++;
    return true;
}

void CullGrid(void) {
    for (int i = 0; i < mapgrid.numnodes; i++) {
        // delete nodes that have no children
        if (vrx_pf_get_all_children_count(i) < 1)
            vrx_pf_delete_node(i);
    }
}


void CreateGrid(qboolean force) {
    int x, y, z, cnt = 0;
    vec3_t v, endpt;
    trace_t tr1, tr2;
    float v0, v1, v2;

    vec3_t min1 = {0, 0, 0}; // width 6x6
    vec3_t max1 = {0, 0, 0};

    vec3_t min2 = {-16, -16, 0}; // width 32x32 (was 24x24)
    vec3_t max2 = {+16, +16, 0};

    mapgrid.numnodes = 0;
    for (int i = 0; i < MAX_GRID_SIZE; i++) {
        // az don't recompute grid all the damn time
        mapgrid.adjacent_nodes[i] = nullptr;
        mapgrid.adjacent_nodes_fly[i] = nullptr;
        mapgrid.adjacent_node_count[i] = 0;
        mapgrid.adjacent_node_count_fly[i] = 0;
    }


    if (!force && LoadGrid())
        return;

    for (x = 0; x < maxx; x++) {
        v0 = g2v0(x); // convert grid(x) to v[0]
        for (y = 0; y < maxy; y++) {
            v1 = g2v1(y); // convert grid(y) to v[1]
            for (z = maxz - 1; z >= 0; z--) {
                v2 = g2v2(z); // convert grid(z) to v[2]
                //--------------------------------------
                VectorSet(v, v0, v1, v2);
                // Skip world locations in solid/lava/slime/window/ladder
                if (gi.pointcontents(v) & MASK_OPAQUE) {
                    z--;
                    continue;
                }
                //-----------------------------------------------
                // At this point,v(x,y,z) is a point in mid-air
                //-----------------------------------------------
                // Trace small bbox down to see what is below
                VectorSet(endpt, v[0], v[1], -8192);
                // Stop at world locations in solid/lava/slime/window/ladder
                tr1 = gi.trace(v, min1, max1, endpt,nullptr,MASK_OPAQUE);
                // Set for-loop index to our endpt's grid(z)
                z = gridz(tr1.endpos[2]);
                // Skip if trace endpt hit func entity.
                if (tr1.ent && (tr1.ent->use || tr1.ent->think || tr1.ent->blocked)) continue;
                // Skip if trace endpt hit lava/slime/window/ladder.
                if (tr1.contents & (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WINDOW)) continue;
                // Skip if trace endpt hit non-walkable slope
                if (tr1.plane.normal[2] < 0.7) continue;
                //----------------------------------------
                // Test vertical clearance above v(x,y,z)
                //----------------------------------------
                VectorCopy(tr1.endpos, endpt);
                //tr1.endpos[2]+=2; // set start just above surface
                endpt[2] += 32; // endpt at approx crouch height
                tr2 = gi.trace(endpt, min2, max2, tr1.endpos,nullptr,MASK_OPAQUE); //GHz - push down instead of up
                // Skip if not reachable by crouched bbox - trace incomplete?
                // if (tr2.fraction != 1.0) continue;
                // Skip if linewidth inside solid - too close to adjoining surface?
                if (tr2.startsolid || tr2.allsolid) continue;

                // GHz: check final position to see if it intersects with a solid
                tr1 = gi.trace(tr2.endpos, min2, max2, tr2.endpos,nullptr,MASK_OPAQUE);
                if (tr1.fraction != 1.0 || tr1.startsolid || tr1.allsolid)
                    continue;
                if (!CheckBottom(tr2.endpos, min2, max2))
                    continue;

                VectorCopy(tr2.endpos, endpt); //GHz
                endpt[2] += 32; //GHz
                //if (tr2.allsolid) continue;
                //-------------------------------------
                // Now, adjust downward for uniformity
                //-------------------------------------
                // AdjustDownward(nullptr,endpt);
                // Houston,we have a valid node!
                if (NearbyGridNode(endpt, cnt))
                    continue; //GHz
                VectorCopy(endpt, mapgrid.pathnode[cnt]); // copy to mapgrid.pathnode[] array
                cnt++;
            }
        }
    }

    mapgrid.numnodes = cnt;
    CullGrid();

    gi.dprintf("%d Nodes Created\n", mapgrid.numnodes);

    //================== pathfinding stuff ================

    if (!force)
        SaveGrid();
}

// #define _kddbg

void InitPathfinding() {
    CreateGrid(false);

    gridkdtree_free(&gridtree);
    gridtree = gridkdtree_create(mapgrid.pathnode, mapgrid.numnodes);

#ifdef _kddbg
    for (int i = 0; i < gridtree->capacity; i++)
        gi.dprintf("%-3d => %3d\n", i, gridtree->data[i]);
#endif

    // az: reconstructed every map load depending on the # of map nodes
    if (pfctx)
        pfctx_free(&pfctx);

    pfctx = pfctx_create(mapgrid.numnodes + 2);
}

void ShutdownPathfinding() {
    pfctx_free(&pfctx);
    gridkdtree_free(&gridtree);

    mapgrid = (struct mapgrid_s){};
}

void Cmd_ComputeNodes_f(edict_t *ent) {
    if (!ent->myskills.administrator)
        return;

    CreateGrid(true);
    safe_cprintf(ent, PRINT_HIGH, "Computing nodes...\n");
}

void Cmd_ToggleShowGrid(edict_t *ent) {
    if (!ent->myskills.administrator)
        return;
    ent->client->showGridDebug++;
    if (ent->client->showGridDebug == GD_MAX) {
        safe_centerprintf(ent, "Show grid OFF\n");
        ent->client->showGridDebug = GD_OFF;
    } else {
        if (ent->client->showGridDebug == GD_NEARBY)
            safe_centerprintf(ent, "Show nearby nodes\n");
        else if (ent->client->showGridDebug == GD_CHILD || ent->client->showGridDebug == GD_CHILD_FLY)
            safe_centerprintf(ent, "Show child links (Fly: %d)\n", ent->client->showGridDebug == GD_CHILD_FLY);
        else if (ent->client->showGridDebug == GD_AIMSPOT)
            safe_centerprintf(ent, "Show path to aim spot (Fly: %d)\n", ent->client->showGridDebug == GD_AIMSPOT_FLY);
        //else
        //	safe_centerprintf(ent, "Create child links\n");
    }

#ifdef VRX_REPRO
    gire.configstring(CONFIG_STORY, "");
#endif
}
