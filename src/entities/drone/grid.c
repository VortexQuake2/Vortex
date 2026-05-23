#include "entities/grid.h"

#include "g_local.h"

// NOTE: This size of the mapgrid.pathnode array may have to change if a larger array is needed for your individual maps..

#define MAX_GRID_SIZE	10000

struct mapgrid_s {
    int numnodes;
    vec3_t pathnode[MAX_GRID_SIZE];
    int *cached_gridlist[MAX_GRID_SIZE];
    int cached_gridlist_count[MAX_GRID_SIZE];
    int gridlist[MAX_GRID_SIZE];
} mapgrid = {};

struct gridkdtree_s* gridtree = nullptr;

struct pfctx_s {
    struct gstack_s* stack;
    struct nodearena_s* arena;
    node_t *OPEN; // Start of OPEN List
    node_t *CLOSED; // Start of CLOSED List
}* pfctx = nullptr;

struct pfctx_s* pfctx_create(size_t nodecapacity) {
    struct pfctx_s* ctx = malloc(sizeof(struct pfctx_s));
    if (!ctx) {
        return nullptr;
    }
    ctx->stack = gstack_create(nodecapacity);
    ctx->arena = nodearena_create(nodecapacity);
    ctx->OPEN = nullptr;
    ctx->CLOSED = nullptr;
    return ctx;
}

void pfctx_free(struct pfctx_s** ptr) {
    if (!*ptr) return;

    nodearena_free(&(*ptr)->arena);
    gstack_free(&(*ptr)->stack);
    free(*ptr);
    *ptr = nullptr;
}

void pfctx_reset(struct pfctx_s* ctx) {
    gstack_reset(ctx->stack);
    nodearena_reset(ctx->arena);
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

//=====================================================
//================== pathfinding stuff ================
//=====================================================;

int NodeCount = 0; //GHz: for debugging

//=====================================================
// Check the desired OPEN/CLOSED LIST for NodeNum..
//=====================================================
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

//===============================================
// Propagate Old node's values to Nodes on Stack
//===============================================
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

//===========================================
// Successor Nodes all pushed onto OPEN list
//===========================================
void GetSuccessorNodes(struct pfctx_s* ctx, node_t *StartNode, const int NodeNumS, const int NodeNumD) {
    int g, c;
    float h;

    // NOTE: NodeNumS is the index of a node that was found by the node searching routine
    //================================
    // Has NodeNumS been Searched yet?
    //================================
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

    //==================================
    // Has NodeNumS been searched yet?
    //==================================
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

    //=======================================
    // It is NOT on the OPEN or CLOSED List!!
    //=======================================
    // Make Successor a Child of StartNode
    //=======================================
    //Successor=(node_t *)malloc(sizeof(node_t));
    node_t *Successor = nodearena_alloc(ctx->arena);
    Successor->nodenum = NodeNumS;
    Successor->dist = g = StartNode->dist + 1;

    //GHz - track node memory use so we can free this later
    //	NodeList[NodeCount++] = Successor;
    NodeCount++;

    // NOTE: the heuristic estimate of the remaining path from this node
    // to the destination node is given by the difference between the 2
    // vectors.  You can come up with your own estimate..

    Successor->distestimation = h = distance(mapgrid.pathnode[NodeNumS], mapgrid.pathnode[NodeNumD]);
    //fabs(vDiff(node[NodeNumS].origin,node[NodeNumD].origin));//GHz - changed to fabs()
    Successor->totaldistestimation = g + h;
    Successor->PrevNode = StartNode; // reverse link to StartNode
    Successor->NextNode = nullptr;
    // make all child links of new Successor node nullptr
    for (c = 0; c < NUMCHILDS; c++)
        Successor->Child[c] = nullptr;

    for (c = 0; c < NUMCHILDS; c++)
        if (StartNode->Child[c] == nullptr) break; // Find first empty Child[] of StartNode
    StartNode->Child[((c < NUMCHILDS) ? c : (NUMCHILDS - 1))] = Successor; // make Successor a child of StartNode

    //=================================
    // Insert Successor into OPEN List
    //=================================
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
int NearestNodeNumber(vec3_t start, const float range, const qboolean vis) {
    int i = 0, bestNodeNum = -1;
    float best = 9999;
    vec3_t v;

    if (!mapgrid.numnodes)
        return -1;

    // get the nodenum for the closest node
    // for (; i < mapgrid.numnodes; i++) {
    //     VectorCopy(mapgrid.pathnode[i], v);
    //     // ignore nodes we can't see
    //     if (vis && !G_IsClearPath(nullptr, MASK_SOLID, v, start))
    //         continue;
    //     const float dist = distance(v, start);
    //     if (range && dist > range)
    //         continue;
    //     if (dist < best) {
    //         best = dist;
    //         bestNodeNum = i;
    //     }
    // }

    return gridkdtree_query(gridtree, start);
}

qboolean NearestNodeLocation(vec3_t start, vec3_t node_loc, const float range, const qboolean vis) {
    int i = 0, bestNodeNum = -1;
    float best = 9999;
    vec3_t v;

    if (!mapgrid.numnodes)
        return false;

    // get the nodenum for the closest node
    // for (; i < mapgrid.numnodes; i++) {
    //     VectorCopy(mapgrid.pathnode[i], v);
    //     const float dist = distance(v, start);
    //     if (range && dist > range)
    //         continue;
    //
    //     // ignore nodes we can't see
    //     if (vis && !G_IsClearPath(nullptr, MASK_SOLID, v, start))
    //         continue;
    //     if (dist < best) {
    //         best = dist;
    //         bestNodeNum = i;
    //     }
    // }
    //
    // if (bestNodeNum == -1)
    //     return false;


    bestNodeNum = gridkdtree_query(gridtree, start);
    if (bestNodeNum == SIZE_MAX)
        return false;

    VectorCopy(mapgrid.pathnode[bestNodeNum], node_loc);
    return true;
}

//=========================================
// What is nodenum of this origin, if any?
//=========================================
int GetNodeNum(vec3_t origin) {
    vec3_t vtmp;
    //float best=99999, bestd=99999, d;//GHz

    //gi.dprintf("GetNodeNum() is checking %d nodes\n", mapgrid.numnodes);

    // Search all of my node[i]'s
    for (int i = 0; i < mapgrid.numnodes; i++) {
        VectorSubtract(mapgrid.pathnode[i], origin, vtmp);
        const float dist = VectorLengthSqr(vtmp);

        /*if (dist < best)//GHz: added next 4 lines for debugging only
            best = dist;
        d=distance(mapgrid.pathnode[i], origin);
        if (d < bestd)
            bestd=d;
        */
        if (dist < 1.0F) //FIXME: wtf does this mean?
        {
            // we are close enough to our goal
            //gi.dprintf("best result=%f, need=%f, actual=%f\n", best, 1.0F, bestd);
            return i;
        }
    }

    // gi.dprintf("best result=%f, need=%f, actual=%f\n", best, 1.0F, bestd);
    return -1;
}

//=========================================
// Pull FIRST node from OPEN, put on CLOSED
//=========================================
node_t *NextBestNode(struct pfctx_s *ctx) {
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

int GetVerticalNodeNum(vec3_t start, const float x, const float y, const float max_z_range, const int nodes) {
    vec3_t v;

    // copy to temp vector
    VectorCopy(start, v);

    // modify 2-d coordinates of temp vector
    v[0] += x;
    v[1] += y;

    // search for nodes
    for (int i = 0; i < nodes; i++) {
        // put temp node and mapgrid.pathnode on the same vertical plane
        //v[2] = mapgrid.pathnode[i][2];
        // is there a match on the x and y coordinates?
        //if (distance(v, mapgrid.pathnode[i]) > 1.0F)
        //	continue;
        if (Get2dDistance(v, mapgrid.pathnode[i]) > 1)
            continue;
        // is it within our specified z range?
        if (abs((int) mapgrid.pathnode[i][2] - (int) start[2]) > max_z_range)
            continue;
        return i;
    }

    return -1;
}

int CheckVertical(vec3_t start, const float x, const float y, const float z, const int max_steps_up, const int max_steps_down) {
    int n;
    vec3_t v;

    VectorCopy(start, v);

    const int max_steps = max_steps_up + max_steps_down;

    v[0] += x; // left-right
    v[1] += y; // forward-backward

    // step up from this location until we find a node
    for (int i = 0; i <= max_steps; i++) {
        if ((n = GetNodeNum(v)) != -1)
            return n;
        if (i <= max_steps_up)
            v[2] += z;
        else
            v[2] -= z;
    }

    return -1;
}



// sets all gridlist values to -1
void ClearGridList(void) {
    for (int i = 0; i < mapgrid.numnodes; i++) //FIXME: should we initialize up to MAX_GRID_SIZE?
        mapgrid.gridlist[i] = -1;
}

qboolean CheckPath1(vec3_t start, vec3_t end) {
    vec3_t from;
    const edict_t *ignore = nullptr;
    trace_t tr;

    VectorCopy(start, from);

    for (int i = 0; i < 10000; i++) {
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

qboolean isValidChildNode(vec3_t start, vec3_t v, const int max_distance, const int max_z_delta) {
    // node should be within +/- 32 units of start on the Z axis
    if (fabs(v[2] - start[2]) > max_z_delta)
        return false;
    // distance check, next node could be anywhere between 128 - 255 units away
    if (Get2dDistance(start, v) >= max_distance)
        return false;
    // basic visibility check
    if (!gi.inPVS(start, v))
        return false;
    // advanced visibility check, make sure the path is wide enough to walk to
    if (!CheckPath(start, v))
        return false;
    return true;
}

// fills gridlist with visible nodes within +/- 32 of start on the Z axis
// returns the number of found nodes
int FillGridList(const int searchType, const int NodeNumStart) {
    int i, j, maxZdelta;
    vec3_t start;

    if (mapgrid.cached_gridlist[NodeNumStart] != nullptr) // az: don't recompute this
    {
        //gi.dprintf("FillGridList() exited: don't recompute\n");

        return mapgrid.cached_gridlist_count[NodeNumStart];
    }
    //else
    //	gi.dprintf("FillGridList()");

    // clear previous gridlist values
    ClearGridList();

    VectorCopy(mapgrid.pathnode[NodeNumStart], start);

    if (searchType == SEARCHTYPE_FLY)
        maxZdelta = 8192;
    else
        maxZdelta = 32;

    // fill gridlist with node indices
    for (i = 0, j = 0; i < mapgrid.numnodes; i++) {
        // we don't want the start node pointing to itself!
        if (i == NodeNumStart)
            continue;
        if (!isValidChildNode(start, mapgrid.pathnode[i], 256, maxZdelta))
            continue;

        // copy mapgrid.pathnode index to gridlist array and increment the gridlist index/nodes found
        mapgrid.gridlist[j++] = i;
    }

    mapgrid.cached_gridlist[NodeNumStart] = vrx_malloc(sizeof(int) * j, TAG_LEVEL);
    for (int k = 0; k < j; k++) // az: copy to cache
    {
        mapgrid.cached_gridlist[NodeNumStart][k] = mapgrid.gridlist[k];
    }

    mapgrid.cached_gridlist_count[NodeNumStart] = j;
    return j;
}

//TODO: limit search pattern on X and Y axis?
int SortGridList(const int searchType, const int NodeNumStart) {
    int childs = NUMCHILDS;
    vec3_t start;

    VectorCopy(mapgrid.pathnode[NodeNumStart], start);

    const int list_size = FillGridList(searchType, NodeNumStart);

    // nothing to sort!
    if (list_size < 2) {
        //gi.dprintf("nothing to sort\n");
        return list_size;
    }

    // we only need to sort nodes equal to the number of child nodes we want
    //if (list_size > NUMCHILDS)
    //	list_size = NUMCHILDS;

    if (list_size < childs) {
        //gi.dprintf("list < childs\n");
        childs = list_size;
    }

    // fill gridlist with node indices closest to start
    //for (i=0; i<list_size; i++)
    //
    for (int i = 0; i < childs; i++) {
        int GLindex = i;
        int index = mapgrid.cached_gridlist[NodeNumStart][i]; //i;
        float best = Get2dDistance(mapgrid.pathnode[mapgrid.cached_gridlist[NodeNumStart][i]], start);

        for (int j = i + 1; j < list_size; j++) {
            const float dist = Get2dDistance(mapgrid.pathnode[mapgrid.cached_gridlist[NodeNumStart][j]], start);
            if (dist < best) {
                index = mapgrid.cached_gridlist[NodeNumStart][j]; //j; // stored value
                GLindex = j; // list position of best value
                best = dist;
            }
        }

        // swap node index of current position with node index of closest node
        // this procedure sorts the list and keeps us from finding the same value twice
        const int temp = mapgrid.cached_gridlist[NodeNumStart][i];
        mapgrid.cached_gridlist[NodeNumStart][i] = index;
        mapgrid.cached_gridlist[NodeNumStart][GLindex] = temp;
        //gridlist[index] = temp;
    }

    //return CullGridList(start, childs);
    return childs; // return maximum number of valid child nodes found
}

int NextNode(const int i, vec3_t start) {
    switch (i) {
        case 0: return GetVerticalNodeNum(start, 32, 0, 32, mapgrid.numnodes); // right
        case 1: return GetVerticalNodeNum(start, -32, 0, 32, mapgrid.numnodes); // left
        case 2: return GetVerticalNodeNum(start, 0, 32, 32, mapgrid.numnodes); // forward
        case 3: return GetVerticalNodeNum(start, 0, -32, 32, mapgrid.numnodes); // backward
    }
    return -1;
}

//==========================================================

void ComputeSuccessors(struct pfctx_s* ctx, const int searchType, node_t *PresentNode, const int NodeNumD) {
    int NextNodeNum;

    // create a sorted list of the closest node indices
    const int maxChilds = SortGridList(searchType, PresentNode->nodenum);

    for (int i = 0; i < maxChilds; i++) {
        // GHz FIX - added if clause to use the cached gridlist if available
        // this is necessary because SortGridList() is only sorting the cached list, not gridList
        //GHz START
        if (mapgrid.cached_gridlist[PresentNode->nodenum] == nullptr)
            NextNodeNum = mapgrid.gridlist[i];
        else
            NextNodeNum = mapgrid.cached_gridlist[PresentNode->nodenum][i];
        //GHz END
        //if (NextNodeNum == PresentNode->nodenum)
        //	gi.dprintf("%d = %d\n", PresentNode->nodenum, NextNodeNum);
        if (NextNodeNum == -1)
            continue;
        GetSuccessorNodes(ctx, PresentNode, NextNodeNum, NodeNumD);
    }
}

int *Waypoint = nullptr; // Integer array of nodenum's along the path
int numpts = 0; // Number of nodes in the path..

int CopyWaypoints(int *wp, const int max) {
    int j = max;

    if (j > numpts)
        j = numpts;

    for (int i = 0; i < j; i++)
        wp[i] = Waypoint[i];

    return j;
}

void GetNodePosition(const int nodenum, vec3_t pos) {
    VectorCopy(mapgrid.pathnode[nodenum], pos);
}

// returns the waypoint index closest to start along the path leading to our final destination (or -1 if list is empty)
int NearestWaypointNum(vec3_t start, int *wp) {
    int bestNodeNum = -1;
    float best = 0;

    //if (!numpts)
    //	return -1;

    // get the nodenum for the closest node
    //for (i = 0; i < numpts; i++)
    for (int i = 0; i < MAX_GRID_SIZE; i++) {
        // we've reached the end of the list
        if (wp[i] == 0)
            break;

        const float dist = distanceSqr(mapgrid.pathnode[wp[i]], start);

        if (!best || dist < best) {
            best = dist;
            bestNodeNum = i;
        }
        //gi.dprintf("%d: node %d dist %f best %d %d %f\n", i, wp[i], dist, wp[bestNodeNum], bestNodeNum, best);//DEBUG: REMOVE THIS!
    }

    return bestNodeNum;
}


void RemoveDuplicates(node_t *BestList, node_t *OtherList) {
    node_t *tNodePrev = nullptr;

    const node_t *bestNode = BestList;

    while (bestNode) {
        node_t *tNode = OtherList;

        // search OtherList for a node that also exists in BestList
        while (tNode) {
            // is this nodenum on the BestList?
            if (tNode == bestNode)
                break;

            // save the previous node
            tNodePrev = tNode;
            // move to the next node in OtherList
            tNode = tNode->NextNode;
        }

        // we found a duplicate, so pop it out of OtherList
        if (tNode)
            tNodePrev->NextNode = tNode->NextNode;

        // move to the next node in BestList
        bestNode = bestNode->PrevNode;
    }
}

//=========================================
int FindPath(const int searchType, vec3_t start, vec3_t destination) {
    node_t *BestNode;
    int g;
    float h;
    vec3_t tstart, tdest;
    // sorry msvc i'm lazyyyyyyyyyyyy
    auto ctx = pfctx;
    pfctx_reset(ctx);

    VectorCopy(start, tstart);
    VectorCopy(destination, tdest);

    // Get NodeNum of start vector
    const int startnodenum = GetNodeNum(tstart);
    if (startnodenum == -1) {
        //gi.dprintf("bad nodenum at start\n");
        return 0; // ERROR
    }

    // Get NodeNum of destination vector
    const int endnodenum = GetNodeNum(tdest);
    if (endnodenum == -1) {
        //gi.dprintf("bad nondenum at end\n");
        return 0; // ERROR
    }

    //gi.dprintf("starting node = %d, end = %d\n", NodeNumS, NodeNumD);

    // Allocate OPEN/CLOSED list pointers..
    ctx->OPEN = nodearena_alloc(ctx->arena);
    ctx->OPEN->NextNode = nullptr;

    ctx->CLOSED = nodearena_alloc(ctx->arena);
    ctx->CLOSED->NextNode = nullptr;

    //================================================
    // This is our very first NODE!  Our start vector
    //================================================
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
    //================================================

    // next node in open list points to our starting node
    ctx->OPEN->NextNode = BestNode = StartNode; // First node on OPEN list..

    //GHz - need to free these nodes too!
    NodeCount += 2;

    for (;;) {
        BestNode = NextBestNode(ctx); // Get next node from OPEN list
        if (!BestNode) {
            //gi.dprintf("ran out of nodes to search\n");
            return 0; //GHz
        }

        if (BestNode->nodenum == endnodenum) break; // we there yet?
        //gi.dprintf("searching node %d\n", BestNode->nodenum);
        ComputeSuccessors(ctx, searchType, BestNode, endnodenum);
    } // Search from here..

    //================================================

    RemoveDuplicates(BestNode, ctx->CLOSED); //FIXME: move this up before the start==end crash check

    //gi.dprintf("%d: processed %d nodes\n", level.framenum,NodeCount);
    if (BestNode == StartNode) {
        return 0;
    }

    //debugging information - shows Best, OPEN, and CLOSED node lists
    //gi.dprintf("Start = %d End = %d\n", NodeNumS, NodeNumD);
    //gi.dprintf("Printing tNode (in reverse):\n");
    //PrintNodes(BestNode, true);
    //gi.dprintf("Printing OPEN list:\n");
    //PrintNodes(OPEN, false);
    //gi.dprintf("Printing CLOSED list:\n");
    //PrintNodes(CLOSED, false);

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
    numpts = i;

    Waypoint = (int *) vrx_malloc(numpts * sizeof(int), TAG_LEVEL);

    // Now, we have to assign the nodenum's along
    // this path in reverse order because that is
    // the way the A* algorithm finishes its search.
    // The last best node it visited was the END!
    // So, we copy them over in reverse.. No biggy..

    while (BestNode) {
        Waypoint[--i] = BestNode->nodenum; //GHz: how/when is this freed?
        BestNode = BestNode->PrevNode;
    }

    // NOTE: At this point, if our numpts returned is not
    // zero, then a path has been found!  To follow this
    // path we simply follow node[Waypoint[i]].origin
    // because Waypoint array is filled with indexes into
    // our node[i] array of valid vectors in the map..
    // We did it!!  Now free the stack and exit..
    NodeCount = 0;

    //TODO: performance... cpu usage is still very high
    //TODO: grid editor, save grid to disk
    //TODO: need some way of handling manually edited grid
    // because NextNode() only searches within a specific 32x32 pattern


    //gi.dprintf("%d: found %d\n",level.framenum, numpts);

    return (numpts);
}

//======================================================
void G_Spawn_Splash(const int type, const int count, const int color, vec3_t start, vec3_t movdir, vec3_t origin) {
    gi.WriteByte(svc_temp_entity);
    gi.WriteByte(type);
    gi.WriteByte(count);
    gi.WritePosition(start);
    gi.WriteDir(movdir);
    gi.WriteByte(color);
    gi.multicast(origin, MULTICAST_PVS);
}

//==================================================

//=====================================================
// NOTE: you may already have this function someplace
//=====================================================
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
        G_Spawn_Splash(TE_LASER_SPARKS, 4, 0xdcdddedf, mapgrid.pathnode[j], vec3_origin, mapgrid.pathnode[j]);
    }
}

void DrawPath1(void) {

    if (numpts < 0)
        return;

    for (int i = 0; i < numpts; i++) {
        int wp = Waypoint[i];
#ifndef VRX_REPRO
        G_Spawn_Splash(TE_LASER_SPARKS, 4, 0xdcdddedf, mapgrid.pathnode[wp], vec3_origin, mapgrid.pathnode[wp]);
#else
        if (i == 0)
            continue;

        int wpPrev = Waypoint[i-1];
        gire.Draw_Arrow(mapgrid.pathnode[wpPrev], mapgrid.pathnode[wp], 1, &rgba_blue, &rgba_blue, FRAMETIME, true);
#endif
        //G_Spawn_Trails(TE_BFG_LASER, mapgrid.pathnode[j], mapgrid.pathnode[j+1]);
    }
}


//=====================================================
//================== pathfinding stuff ================
//=====================================================


// draws the path to the spot the player is aiming at
void DrawPathToAimSpot(edict_t *ent) {
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

    if (FindPath(SEARCHTYPE_WALK, start, end)) {
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

//========================================================
// Put this at very top of ClientThink() so you can see
// the nodes created by CreateGrid as you walk around!
// Also, prototype this function above ClientThink()...
//========================================================

// NOTE: max search distance for child links should be set to
// gap specified in nearbyGridNode + x/yevery + min1/max1 (e.g. 128+32+3=163)
void DrawChildLinks(edict_t *ent) {
    int count;
    float maxDist;
    vec3_t start, end;
    //	trace_t	tr;

    if (ent->client->showGridDebug == GD_AIMSPOT) {
        DrawPathToAimSpot(ent);
        return;
    }

    // const int parentNode = NearestNodeNumber(ent->s.origin, 255, true);
    const int parentNode = gridkdtree_query(gridtree, ent->s.origin);
    VectorCopy(mapgrid.pathnode[parentNode], start);

    for (int i = count = maxDist = 0; i < mapgrid.numnodes; i++) {
        // don't link parent node to itself!
        if (parentNode && i == parentNode)
            continue;
        if (!isValidChildNode(start, mapgrid.pathnode[i], 256, 32))
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
                          NearestNodeNumber(ent->s.origin, 255, true), count, maxDist);
#ifndef VRX_REPRO
        safe_centerprintf(ent, "%s", s);
#else
        gire.configstring(CONFIG_STORY, s);
#endif
    }
}

void InvalidateGridCache() {
    for (int i = 0; i < mapgrid.numnodes; i++) {
        if (mapgrid.cached_gridlist[i]) {
            vrx_free(mapgrid.cached_gridlist[i]);
            mapgrid.cached_gridlist[i] = nullptr;
            mapgrid.cached_gridlist_count[i] = 0;
        }
    }
}

int GetGridNodes() {
    return mapgrid.numnodes;
}

qboolean GetRandomGridPosition(vec3_t pos) {
    if (mapgrid.numnodes < 1)
        return false;

    const int index = GetRandom(0, mapgrid.numnodes - 1);

    VectorCopy(mapgrid.pathnode[index], pos);
    return true;
}

qboolean GetGridPosition(vec3_t pos, int index) {
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
        if (GetGridPosition(v, i))
            gi.dprintf("%d: %.1f %.1f %.1f\n", i, v[0], v[1], v[2]);
    }
}

void DrawNearbyGrid(edict_t *ent) {
    vec3_t v, forward;

    AngleVectors(ent->s.angles, forward,nullptr,nullptr);

    for (int i = 0; i < mapgrid.numnodes; i++) {
        VectorSubtract(mapgrid.pathnode[i], ent->s.origin, v);
        if (VectorLength(v) >= 256) continue; // limit view distance to eliminate overflows
        if (DotProduct(v, forward) > 0.3) {
            // infront?
            VectorCopy(mapgrid.pathnode[i], v);
            v[2] -= 4; // node height
            // NearestNodeLocation(ent->s.origin, start);
            // gi.dprintf("%f\n", fabs(mapgrid.pathnode[i][2]-start[2]));
#ifndef VRX_REPRO
            G_Spawn_Trails(TE_BFG_LASER, mapgrid.pathnode[i], v);
#else
            gire.Draw_Point(v, 2, &rgba_green, FRAMETIME, true);
#endif
        }
    }
}

void DeleteNode(const int nodenum) {
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

    if ((nearestNode = NearestNodeNumber(ent->s.origin, 255, true)) != -1)
        DeleteNode(nearestNode);

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

//========================================================
int AdjustDownward(const edict_t *ignore, vec3_t start) {
    vec3_t endpt;
    VectorSet(endpt, start[0], start[1], -8192);
    trace_t tr = gi.trace(start, tv(-1, -1, 0), tv(1, 1, 0), endpt, ignore, CONTENTS_SOLID);
    tr.endpos[2] += 32;
    return (int) (tr.endpos[2] - start[2]); // return delta, if needed later..
}


//========================================================

int GetNumChildren(const int parent_nodenum, const int nodes) {
    int j;

    for (int i = j = 0; i < nodes; i++) {
        if (i == parent_nodenum)
            continue;
        if (!isValidChildNode(mapgrid.pathnode[parent_nodenum], mapgrid.pathnode[i], 256, 32))
            continue;
        j++;
    }

    return j;
}

// NOTE: max search distance for child links should be set to
// gap specified in nearbyGridNode + x/yevery + min1/max1 (e.g. 128+32+3=163)
qboolean NearbyGridNode(vec3_t start, const int nodes) {
    for (int i = 0; i < nodes; i++) {
        if (!isValidChildNode(start, mapgrid.pathnode[i], 129, 18))
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
        if (GetNumChildren(i, mapgrid.numnodes) < 1)
            DeleteNode(i);
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
    for (int i = 0; i < MAX_GRID_SIZE; i++) // az don't recompute grid all the damn time
    {
        mapgrid.cached_gridlist[i] = nullptr;
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

    //=====================================================
    //================== pathfinding stuff ================
    //=====================================================

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
        else if (ent->client->showGridDebug == GD_CHILD)
            safe_centerprintf(ent, "Show child links\n");
        else if (ent->client->showGridDebug == GD_AIMSPOT)
            safe_centerprintf(ent, "Show path to aim spot\n");
        //else
        //	safe_centerprintf(ent, "Create child links\n");
    }

#ifdef VRX_REPRO
    gire.configstring(CONFIG_STORY, "");
#endif
}
