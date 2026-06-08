#include "entities/grid.h"

#include "g_local.h"

cvar_t *vrx_gridgen_density;

#define MAX_GRID_SIZE	10000
#define MASK_PATH (MASK_SOLID|CONTENTS_MONSTERCLIP)
#define MASK_OPAQUE_PATH (MASK_OPAQUE|CONTENTS_MONSTERCLIP)
#define PF_DENSITY ((int)vrx_gridgen_density->value)

enum nodeflag_t : uint32_t {
    NF_NONE,

    // node cannot be walked on
    NF_NOWALK = U32BIT(1),

    // node is lower end of plat
    NF_PLATLOWER = U32BIT(2),

    // node is upper end of plat
    NF_PLATUPPER = U32BIT(3),

    // node was created referencing entity data
    NF_ENTREF = U32BIT(4),

    // the user added this thing
    NF_USER = U32BIT(5),

    // node is plat
    NF_PLAT = NF_PLATLOWER | NF_PLATUPPER,

    // do not save this node to disk if...
    NF_NOSAVE = NF_PLAT | NF_ENTREF
};

enum linkflag_t : uint32_t {
    LF_NONE,
    LF_FLY = U32BIT(1),
    LF_WALK = U32BIT(2),
    LF_FALL = U32BIT(3),
    LF_PLATFORM = U32BIT(4)
};

struct mapgrid_link_s {
    size_t nodenum;
    enum linkflag_t linkflags;
};

struct mapgrid_s {
    float gap;
    int numnodes;
    vec3_t pathnode[MAX_GRID_SIZE];
    enum nodeflag_t nodeflags[MAX_GRID_SIZE];
    edict_t *nodeent[MAX_GRID_SIZE];

    // adjacency list
    struct mapgrid_link_s *adjacent_nodes[MAX_GRID_SIZE];
    size_t adjacent_node_count[MAX_GRID_SIZE];
} *mapgrid;

struct gridkdtree_s *gridtree = nullptr;

struct pfctx_s {
    struct gstack_s *stack;
    struct nodearena_s *arena;
    struct gheap_s *openheap;

    node_t **nodelist;

    // results
    int *waypoints; // Integer array of nodenum's along the path
    int numpts; // Number of nodes in the path..

    size_t capacity;
} *pfctx = nullptr;

void pfctx_free(struct pfctx_s **ptr) {
    if (!*ptr) return;

    nodearena_free(&(*ptr)->arena);
    gstack_free(&(*ptr)->stack);
    gheap_free(&(*ptr)->openheap);
    if ((*ptr)->waypoints) {
        free((*ptr)->waypoints);
    }

    if ((*ptr)->nodelist)
        free((*ptr)->nodelist);

    free(*ptr);
    *ptr = nullptr;
}

struct pfctx_s *pfctx_create(size_t nodecapacity) {
    struct pfctx_s *ctx = malloc(sizeof(struct pfctx_s));
    if (!ctx) {
        return nullptr;
    }
    ctx->stack = gstack_create(nodecapacity);

    if (!ctx->stack)
        goto error;

    ctx->arena = nodearena_create(nodecapacity);

    if (!ctx->arena)
        goto error;

    ctx->waypoints = malloc(nodecapacity * sizeof ctx->waypoints[0]);
    if (!ctx->waypoints) {
        goto error;
    }

    ctx->nodelist = malloc(nodecapacity * sizeof ctx->nodelist[0]);
    if (!ctx->nodelist) {
        goto error;
    }

    ctx->openheap = gheap_create(nodecapacity);

    if (!ctx->openheap)
        goto error;

    ctx->numpts = 0;
    ctx->capacity = nodecapacity;
    return ctx;

error:
    pfctx_free(&ctx);
    return nullptr;
}

void pfctx_reset(struct pfctx_s *ctx) {
    gstack_reset(ctx->stack);
    nodearena_reset(ctx->arena);
    gheap_reset(ctx->openheap);
    ctx->numpts = 0;
    memset(ctx->nodelist, 0, ctx->capacity * sizeof(ctx->nodelist[0]));
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
void PrintNodes(const node_t *Node, const qboolean reverse) {
    const node_t *tNode = Node;

    while (tNode) {
        int nodeNumber = tNode->nodenum;
        if (nodeNumber < 0 || nodeNumber > mapgrid->numnodes)
            nodeNumber = 9999;
        gi.dprintf("%d->", nodeNumber);
        if (reverse)
            tNode = tNode->prev;
        else
            tNode = tNode->next;
    }
    gi.dprintf("(null)\n");
}

// Propagate Old node's values to Nodes on Stack
void PropagateDown(struct gstack_s *stack, node_t *Old) {
    int g, c;

    for (c = 0; c < NUMCHILDS; c++) // parse through Old node children
        if (Old->child[c])
            if (Old->dist + 1 < Old->child[c]->dist) {
                Old->child[c]->dist = g = Old->dist + 1; //FIXME:why is 'g' not initialized? GHZ: added 'g='
                Old->child[c]->totaldistestimation = g + Old->distestimation;
                Old->child[c]->prev = Old;
                gstack_push(stack, Old->child[c]);
            } // Push onto Stack

    while (gstack_top(stack)) {
        // is the stack in use?
        node_t *POPNode = gstack_pop(stack); // grab node from stack
        for (c = 0; c < NUMCHILDS; c++) {
            // parse through all existing POPNOde children
            if (!POPNode->child[c]) break; // No more valid Child nodes!
            if (POPNode->dist + 1 < POPNode->child[c]->dist) {
                // update g and f values
                POPNode->child[c]->dist = g = POPNode->dist + 1;
                POPNode->child[c]->totaldistestimation = g + POPNode->distestimation;
                POPNode->child[c]->prev = POPNode;
                gstack_push(stack, POPNode->child[c]);
            }
        }
    } // Push onto Stack
}

#define vDiff(b,a) sqrt((a[0]*a[0]-b[0]*b[0])+(a[1]*a[1]-b[1]*b[1])+(a[2]*a[2]-b[2]*b[2]))

// Successor Nodes all pushed onto OPEN list
void vrx_pf_push_successors(const struct pfctx_s *ctx, node_t *StartNode, const int NodeNumS, const int NodeNumD) {
    int g, c;
    float h;

    // NOTE: NodeNumS is the index of a node that was found by the node searching routine
    // Has NodeNumS been Searched yet?
    // see if this node is already on the OPEN list
    node_t *old = ctx->nodelist[NodeNumS];
    if (old && old->list != LIST_NONE) {
        // node was found on the OPEN list
        // this means the node was found before (as a child of another node)
        // but not yet searched (as a parent node)
        for (c = 0; c < NUMCHILDS; c++) {
            // break on the first available child slot of StartNode
            if (!StartNode->child[c])
                break;
        }

        // if we found an empty child slot, use it, otherwise use the last one
        StartNode->child[((c < NUMCHILDS) ? c : (NUMCHILDS - 1))] = old;

        // have we gone farther with this node than StartNode?
        if (StartNode->dist + 1 < old->dist) {
            old->dist = g = StartNode->dist + 1; // make node one step beyond StartNode
            old->totaldistestimation = g + old->distestimation; // update total cost
            old->prev = StartNode; // reverse link to StartNode

            if (old->list == LIST_CLOSED)
                PropagateDown(ctx->stack, old);
        }
        return;
    }

    // It is NOT on the OPEN or CLOSED List!!
    // Make Successor a Child of StartNode
    node_t *successor = nodearena_alloc(ctx->arena);
    successor->nodenum = NodeNumS;
    successor->dist = g = StartNode->dist + 1;

    // NOTE: the heuristic estimate of the remaining path from this node
    // to the destination node is given by the difference between the 2
    // vectors.  You can come up with your own estimate..

    successor->distestimation = h = distanceSqr(mapgrid->pathnode[NodeNumS], mapgrid->pathnode[NodeNumD]);
    successor->totaldistestimation = g + h;
    successor->prev = StartNode; // reverse link to StartNode
    successor->next = nullptr;
    // make all child links of new Successor node nullptr
    for (c = 0; c < NUMCHILDS; c++)
        successor->child[c] = nullptr;

    for (c = 0; c < NUMCHILDS; c++)
        if (StartNode->child[c] == nullptr) break; // Find first empty Child[] of StartNode
    StartNode->child[((c < NUMCHILDS) ? c : (NUMCHILDS - 1))] = successor; // make Successor a child of StartNode

    // Insert Successor into OPEN List
    gheap_push(ctx->openheap, successor->totaldistestimation, successor);
    successor->list = LIST_OPEN;
    ctx->nodelist[successor->nodenum] = successor;
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
    if (!mapgrid->numnodes)
        return -1;

    const auto idx = gridkdtree_query(gridtree, start);
    if (idx == SIZE_MAX)
        return -1;

    return idx;
}

qboolean vrx_pf_nearest_node_location(vec3_t start, vec3_t node_loc, const float range, const qboolean vis) {
    if (!mapgrid->numnodes)
        return false;

    const auto bestNodeNum = gridkdtree_query(gridtree, start);
    if (bestNodeNum == SIZE_MAX)
        return false;

    VectorCopy(mapgrid->pathnode[bestNodeNum], node_loc);
    return true;
}

// Pull FIRST node from OPEN, put on CLOSED
node_t *vrx_pf_pop_next_open_node(const struct pfctx_s *ctx) {
    node_t *node = gheap_pop(ctx->openheap);

    if (!node) {
        // gi.dprintf("OPEN list is empty!\n");
        return nullptr;
    }

    node->list = LIST_CLOSED; // Put at FRONT of CLOSED list
    return node; // Return Next Best Node
}

bool CheckPath1(vec3_t start, vec3_t end) {
    vec3_t from;
    const edict_t *ignore = nullptr;
    trace_t tr;

    VectorCopy(start, from);

    for (int i = 0; i < 32; i++) {
        tr = gi.trace(from, tv(-16, -16, 0), tv(16, 16, 0), end, ignore, MASK_PATH);
        // ignore doors
        if (tr.ent && tr.ent->inuse && tr.ent->mtype == FUNC_DOOR) {
            VectorCopy(tr.endpos, from);
            ignore = tr.ent;
        }
        // stop on anything else that is solid
        else
            break;
    }

    if (tr.fraction != 1.0 || tr.startsolid || tr.allsolid || tr.contents & MASK_PATH)
        return false;
    return true;
}

bool CheckPath(vec3_t start, vec3_t end) {
    return CheckPath1(start, end);
}

bool CheckPathFly(vec3_t start, vec3_t end) {
    const edict_t *ignore = nullptr;

    // quick check.
    if (start[2] >= end[2])
        return false;

    // check in an L shape, first going up, then horizontally
    // check vertically, first
    vec3_t top;
    VectorSet(top, start[0], start[1], end[2]);
    trace_t tr = gi.trace(start, tv(-16, -16, 0), tv(16, 16, 0), top, ignore, MASK_PATH);
    if (tr.fraction != 1.0 || tr.startsolid || tr.allsolid || tr.contents & MASK_PATH)
        return false;

    // now the second part of the L, on the xy plane
    tr = gi.trace(top, tv(-16, -16, 0), tv(16, 16, 0), end, ignore, MASK_PATH);
    if (tr.fraction != 1.0 || tr.startsolid || tr.allsolid || tr.contents & MASK_PATH)
        return false;


    return true;
}

bool CheckPathFall(vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs) {
    const edict_t *ignore = nullptr;

    // quick check.
    if (start[2] <= end[2])
        return false;

    // check in an L shape, first going horizontally, then down
    vec3_t top;
    VectorSet(top, end[0], end[1], start[2]);
    trace_t tr = gi.trace(start, mins, maxs, top, ignore, MASK_PATH);
    if (tr.fraction != 1.0 || tr.startsolid || tr.allsolid || tr.contents & MASK_PATH)
        return false;

    tr = gi.trace(top, mins, maxs, end, ignore, MASK_PATH);
    if (tr.fraction != 1.0 || tr.startsolid || tr.allsolid || tr.contents & MASK_PATH)
        return false;


    return true;
}


typedef struct linkvalidity_s {
    // valid for walk
    bool walk;

    // valid for fly
    bool fly;

    // valid for fall
    bool fall;

    // valid because it's a platform
    bool platform;
} linkvalidity_t;

// should only be reserved for spatial checks
linkvalidity_t vrx_pf_is_valid_child_position(const int max_2d_distance,
                                              vec3_t start, vec3_t v) {
    const bool validWalkZdist = fabs(v[2] - start[2]) <= 32;

    // distance check, next node could be anywhere between 128 - 255 units away
    if (Get2dDistance(start, v) >= max_2d_distance)
        return (linkvalidity_t){};

    // basic visibility check
    if (!gi.inPVS(start, v))
        return (linkvalidity_t){};

    // advanced visibility check, make sure the path is wide enough to walk to
    // now, it might seem that fly and fall are similar, but fall does the check backwards wrt fly
    const bool validWalkPath = CheckPath(start, v) && validWalkZdist;
    const bool validFlyPath = CheckPathFly(start, v);
    const bool validFallPath = CheckPathFall(start, v, tv(-16, -16, 0), tv(16, 16, 0)) && !validWalkZdist;

    return (linkvalidity_t){
        validWalkPath,
        validFlyPath,
        validFallPath
    };
}

// this one is reserved for actual children checks
linkvalidity_t vrx_pf_is_valid_child_node(
    size_t parent,
    size_t child) {
    vec3_t start, v;
    int max_2d_dist = mapgrid->gap + 8;

    if (parent == child)
        // no
        return (linkvalidity_t){};

    bool validPlat = false;
    if (mapgrid->nodeflags[parent] & NF_PLAT && mapgrid->nodeflags[child] & NF_PLAT) {
        if (mapgrid->nodeent[parent] == mapgrid->nodeent[child]) {
            validPlat = true;
        }
    }

    bool oneIsPlat = mapgrid->nodeflags[parent] & NF_PLAT || mapgrid->nodeflags[child] & NF_PLAT ;
    bool oneIsntPlat = !(mapgrid->nodeflags[parent] & NF_PLAT) || !(mapgrid->nodeflags[child] & NF_PLAT);
    if (oneIsPlat && oneIsntPlat) {
        // we need a mildly expanded range here that covers the entire plat
        edict_t* plat = nullptr;
        if (mapgrid->nodeent[parent])
            plat = mapgrid->nodeent[parent];
        if (mapgrid->nodeent[child])
            plat = mapgrid->nodeent[child];

        // draw a circle around the bbox
        vec3_t ur;
        vec3_t dl;
        VectorSet(ur, plat->maxs[0], plat->maxs[1], 0);
        VectorSet(dl, plat->mins[0], plat->mins[1], 0);

        vec3_t span;
        VectorSubtract(ur, dl, span);

        const float len = VectorLength(span) / 2 + mapgrid->gap / 2;
        max_2d_dist = (int)len;
    }

    VectorCopy(mapgrid->pathnode[parent], start);
    VectorCopy(mapgrid->pathnode[child], v);

    auto validity = vrx_pf_is_valid_child_position(max_2d_dist, start, v);
    validity.platform = validPlat;
    // platforms cannot have fly children
    validity.fly &= mapgrid->nodeflags[parent] & NF_PLATLOWER;

    return validity;
}

static struct mapgrid_link_s link_from_validity(size_t nodenum, struct linkvalidity_s validity) {
    struct mapgrid_link_s ret = {};
    ret.nodenum = nodenum;
    ret.linkflags |= validity.fly ? LF_FLY : LF_NONE;
    ret.linkflags |= validity.walk ? LF_WALK : LF_NONE;
    ret.linkflags |= validity.fall ? LF_FALL : LF_NONE;
    ret.linkflags |= validity.platform ? LF_PLATFORM : LF_NONE;

    return ret;
}

// fills gridlist with visible nodes within +/- 32 of start on the Z axis
// returns the number of found nodes
int vrx_pf_compute_adjacent_nodes(const int NodeNumStart, bool store) {
    // grid culling guarantees this case is valid
    // already computed, we found nothing
    if (mapgrid->adjacent_node_count[NodeNumStart] == SIZE_MAX)
        return 0;

    // already computed, we found something
    if (mapgrid->adjacent_node_count[NodeNumStart])
        return mapgrid->adjacent_node_count[NodeNumStart];

    // 0 == not computed at all
    int i, j;
    struct mapgrid_link_s adjacencytmp[32] = {0};

    // fill gridlist with node indices
    const auto maxadj = sizeof adjacencytmp / sizeof adjacencytmp[0];
    for (i = 0, j = 0; i < mapgrid->numnodes && j < maxadj; i++) {
        // we don't want the start node pointing to itself!
        if (i == NodeNumStart)
            continue;

        const auto validity = vrx_pf_is_valid_child_node(
            NodeNumStart,
            i
        );

        // az: we want a nonzero field == this memcmp check
        if (!memcmp(&validity, &(linkvalidity_t){}, sizeof validity))
            continue;

        // copy mapgrid->pathnode index to gridlist array and increment the gridlist index/nodes found
        adjacencytmp[j] = link_from_validity(i, validity);
        j++;
    }

    // az: copy to cache
    if (store && j) {
        mapgrid->adjacent_nodes[NodeNumStart] = malloc(sizeof(mapgrid->adjacent_nodes[0][0]) * j);
        memcpy(mapgrid->adjacent_nodes[NodeNumStart], adjacencytmp, sizeof(mapgrid->adjacent_nodes[0][0]) * j);
        mapgrid->adjacent_node_count[NodeNumStart] = j;
    } else {
        mapgrid->adjacent_node_count[NodeNumStart] = SIZE_MAX;
    }

    return j;
}


//TODO: limit search pattern on X and Y axis?
int vrx_pf_sort_adjacent_nodes(const int parent) {
    vec3_t start;

    VectorCopy(mapgrid->pathnode[parent], start);

    const int list_size = vrx_pf_compute_adjacent_nodes(parent, true);

    // nothing to sort!
    if (list_size < 2) {
        //gi.dprintf("nothing to sort\n");
        return list_size;
    }

    // fill gridlist with node indices closest to start
    for (int i = 0; i < list_size; i++) {
        const auto link = mapgrid->adjacent_nodes[parent][i];
        int bestChildIndex = i;
        auto bestChild = link;
        float best = Get2dDistance(mapgrid->pathnode[bestChild.nodenum], start);

        for (int childIdx = i + 1; childIdx < list_size; childIdx++) {
            const auto child = mapgrid->adjacent_nodes[parent][childIdx];
            const float dist = Get2dDistance(mapgrid->pathnode[child.nodenum], start);
            if (dist < best) {
                bestChild = child;
                bestChildIndex = childIdx; // list position of best value
                best = dist;
            }
        }

        // swap node index of current position with node index of closest node
        // this procedure sorts the list and keeps us from finding the same value twice
        const auto temp = link;
        mapgrid->adjacent_nodes[parent][i] = bestChild;
        mapgrid->adjacent_nodes[parent][bestChildIndex] = temp;
    }

    return list_size; // return maximum number of valid child nodes found
}


void vrx_pf_compute_successors(struct pfctx_s *ctx, const enum searchtype_t searchType, node_t *PresentNode,
                               const int NodeNumD) {
    // create a sorted list of the closest node indices
    const int maxChilds = vrx_pf_sort_adjacent_nodes(PresentNode->nodenum);

    for (int i = 0; i < maxChilds; i++) {
        // GHz FIX - added if clause to use the cached gridlist if available
        // this is necessary because SortGridList() is only sorting the cached list, not gridList
        //GHz START
        const auto nextLink = mapgrid->adjacent_nodes[PresentNode->nodenum][i];
        //GHz END

        if (searchType == SEARCHTYPE_WALK) {
            const bool canFall = nextLink.linkflags & LF_FALL;
            const bool canWalk = nextLink.linkflags & LF_WALK;
            const bool canPlatform = nextLink.linkflags & LF_PLATFORM &&
                mapgrid->nodeflags[PresentNode->nodenum] & NF_PLATLOWER;

            const bool canMove = canFall || canWalk || canPlatform;
            if (!canMove)
                continue;
        }

        if (searchType == SEARCHTYPE_FLY) {
            const bool canFly = nextLink.linkflags & LF_FLY;
            const bool canWalk = nextLink.linkflags & LF_WALK;
            const bool canFall = nextLink.linkflags & LF_FALL;

            const bool canMove = canFly || canWalk || canFall;
            if (!canMove)
                continue;
        }

        const int nextNodeIndex = nextLink.nodenum;
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
    VectorCopy(mapgrid->pathnode[nodenum], pos);
}

// returns the waypoint index closest to start along the path leading to our final destination (or -1 if list is empty)
int vrx_pf_nearest_waypoint_index_along_path(vec3_t start, const int *wp, size_t wpcount) {
    int bestNodeNum = -1;
    float best = INFINITY;

    // get the nodenum for the closest node
    for (int i = 0; i < wpcount; i++) {
        const float dist = distanceSqr(mapgrid->pathnode[wp[i]], start);

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

    // This is our very first NODE!  Our start vector
    node_t *start_node = nodearena_alloc(ctx->arena);
    start_node->nodenum = startnodenum; // starting position nodenum
    start_node->dist = g = 0; // we haven't gone anywhere yet
    start_node->distestimation = h = distance(start, destination);
    //fabs(vDiff(start,destination)); // calculate remaining distance (heuristic estimate) GHz - changed to fabs()
    start_node->totaldistestimation = g + h; // total cost from start to finish
    for (int c = 0; c < NUMCHILDS; c++)
        start_node->child[c] = nullptr; // no children for search pattern yet
    start_node->next = nullptr;
    start_node->prev = nullptr;
    start_node->list = LIST_OPEN;
    ctx->nodelist[startnodenum] = start_node;


    // next node in open list points to our starting node
    // First node on OPEN list..
    gheap_push(ctx->openheap, start_node->totaldistestimation, start_node);

    for (;;) {
        BestNode = vrx_pf_pop_next_open_node(ctx); // Get next node from OPEN list
        if (!BestNode) {
            //gi.dprintf("ran out of nodes to search\n");
            return 0; //GHz
        }

        if (BestNode->nodenum == endnodenum) break; // we there yet?
        vrx_pf_compute_successors(ctx, searchType, BestNode, endnodenum);
    } // Search from here..

    if (BestNode == start_node) {
        return 0;
    }

    BestNode->next = nullptr; // Must tie this off!

    // How many nodes we got?
    const node_t *tNode = BestNode;
    int i = 0;
    while (tNode) {
        i++; // How many nodes?
        tNode = tNode->prev;
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
        BestNode = BestNode->prev;
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

void DrawPath(const edict_t *ent) {
    if (!ent->showPathDebug)
        return;

    if (!ent->monsterinfo.numWaypoints)
        return;

    for (int i = 0; i < ent->monsterinfo.numWaypoints; i++) {
        const int j = ent->monsterinfo.waypoint[i];
#ifndef VRX_REPRO
        G_Spawn_Splash(TE_LASER_SPARKS, 4, 0xdcdddedf, mapgrid->pathnode[j], vec3_origin, mapgrid->pathnode[j]);
#else
        if (i == 0)
            continue;

        const int wpPrev = ent->monsterinfo.waypoint[i - 1];
        gire.Draw_Arrow(mapgrid->pathnode[wpPrev], mapgrid->pathnode[j], 1, &rgba_blue, &rgba_blue, FRAMETIME, true);
#endif
    }
}

void DrawPath1(void) {
    if (pfctx->numpts < 0)
        return;

    for (int i = 0; i < pfctx->numpts; i++) {
        const int wp = pfctx->waypoints[i];
#ifndef VRX_REPRO
        G_Spawn_Splash(TE_LASER_SPARKS, 4, 0xdcdddedf, mapgrid->pathnode[wp], vec3_origin, mapgrid->pathnode[wp]);
#else
        if (i == 0)
            continue;

        const int wpPrev = pfctx->waypoints[i - 1];
        gire.Draw_Arrow(mapgrid->pathnode[wpPrev], mapgrid->pathnode[wp], 1, &rgba_blue, &rgba_blue, FRAMETIME, true);
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
    const size_t snode = gridkdtree_query(gridtree, start);
    const size_t enode = gridkdtree_query(gridtree, tr.endpos);
    if (snode == SIZE_MAX
        || enode == SIZE_MAX) {
        // can't find nearby nodes
        //gi.dprintf("couldn't find nearby nodes");
        gire.Draw_Circle(tr.endpos, 10, &rgba_red, 0.1, false);
        return;
    }
    VectorCopy(mapgrid->pathnode[snode], start);
    VectorCopy(mapgrid->pathnode[enode], end);

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
    float maxDist;
    vec3_t start, end;
    //	trace_t	tr;

    if (ent->client->showGridDebug == GD_OFF)
        return;

    const auto searchType = (ent->client->showGridDebug == GD_CHILD || ent->client->showGridDebug == GD_AIMSPOT)
                                ? SEARCHTYPE_WALK
                                : SEARCHTYPE_FLY;

    if (ent->client->showGridDebug == GD_AIMSPOT) {
        DrawPathToAimSpot(ent, searchType);
        return;
    }

    if (ent->client->showGridDebug != GD_CHILD)
        return;

    const int parentNode = gridkdtree_query(gridtree, ent->s.origin);
    VectorCopy(mapgrid->pathnode[parentNode], start);

    const int children = vrx_pf_compute_adjacent_nodes(parentNode, true);
    for (int i = 0; i < children; i++) {
        const auto link = mapgrid->adjacent_nodes[parentNode][i];

        // copy node position
        VectorCopy(mapgrid->pathnode[link.nodenum], end);

        // get distance between parent node and this node
        const float dist = Get2dDistance(start, end);

        // update maximum child link distance
        if (!maxDist || dist > maxDist)
            maxDist = dist;

        // spawn bfg laser trails between parent and child nodes
#ifndef VRX_REPRO
        G_Spawn_Trails(TE_BFG_LASER, start, end);
#else
        if (link.linkflags & LF_FLY && !(link.linkflags & (LF_WALK | LF_PLATFORM)))
            gire.Draw_Line(start, end, &rgba_orange, FRAMETIME, true);
        else if (link.linkflags & LF_FALL && !(link.linkflags & (LF_PLATFORM | LF_WALK)))
            gire.Draw_Line(start, end, &rgba_red, FRAMETIME, true);
        else if (link.linkflags & LF_PLATFORM)
            gire.Draw_Line(start, end, &rgba_blue, FRAMETIME, true);
        else
            gire.Draw_Line(start, end, &rgba_green, FRAMETIME, true);
#endif
    }

    if (!(level.framenum % 20)) {
        const auto s = va("Node %d has %d child links @ %.0f.\n",
                          parentNode, children, maxDist);
#ifndef VRX_REPRO
        safe_centerprintf(ent, "%s", s);
#else
        gire.configstring(CONFIG_STORY, s);
#endif
    }
}

void vrx_free_adjacency_lists() {
    if (!mapgrid)
        return;
    for (int i = 0; i < mapgrid->numnodes; i++) {
        if (mapgrid->adjacent_nodes[i]) {
            free(mapgrid->adjacent_nodes[i]);
            mapgrid->adjacent_nodes[i] = nullptr;
            mapgrid->adjacent_node_count[i] = 0;
        }
    }
}

int GetGridNodes() {
    if (!mapgrid)
        return 0;
    return mapgrid->numnodes;
}

qboolean vrx_pf_get_random_grid_position(vec3_t pos) {
    if (mapgrid->numnodes < 1)
        return false;

    const int index = GetRandom(0, mapgrid->numnodes - 1);

    VectorCopy(mapgrid->pathnode[index], pos);
    return true;
}

qboolean vrx_pf_get_grid_position(vec3_t pos, int index) {
    if (!index)
        index = 0;

    while (index < mapgrid->numnodes) {
        VectorCopy(mapgrid->pathnode[index], pos);
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

    AngleVectors(ent->s.angles, forward, nullptr, nullptr);

    for (int i = 0; i < mapgrid->numnodes; i++) {
        VectorSubtract(mapgrid->pathnode[i], ent->s.origin, v);
#ifndef VRX_REPRO
        if (VectorLength(v) >= 256) continue; // limit view distance to eliminate overflows
#endif
        if (DotProduct(v, forward) > 0.3) {
            // infront?
            VectorCopy(mapgrid->pathnode[i], v);
#ifndef VRX_REPRO
            v[2] -= 4; // node height
            G_Spawn_Trails(TE_BFG_LASER, mapgrid->pathnode[i], v);
#else
            gire.Draw_Point(v, 4, &rgba_green, FRAMETIME, true);
            // gire.Draw_Bounds(
            //     tv(-16 + v[0], -16 + v[1], -32 + v[2]),
            //     tv(16 + v[0], 16 + v[1], 0 + v[2]),
            //     &rgba_white, FRAMETIME, true);
#endif
        }
    }
}

void gridtree_regenerate(void) {
    gridkdtree_free(&gridtree);
    gridtree = gridkdtree_create(mapgrid->pathnode, mapgrid->numnodes);
}


void vrx_pf_remove_link(int child, int parent) {
    // guard against the SIZE_MAX sentinel and against missing buffer
    if (mapgrid->adjacent_node_count[parent] == SIZE_MAX)
        return;
    if (!mapgrid->adjacent_nodes[parent])
        return;
    for (size_t i = 0; i < mapgrid->adjacent_node_count[parent]; i++) {
        if (mapgrid->adjacent_nodes[parent][i].nodenum == (size_t)child) {
            mapgrid->adjacent_nodes[parent][i] = mapgrid->adjacent_nodes[parent][mapgrid->adjacent_node_count[parent] - 1];

            mapgrid->adjacent_nodes[parent][mapgrid->adjacent_node_count[parent] - 1] = (struct mapgrid_link_s){};
            mapgrid->adjacent_node_count[parent]--;
            // continue scanning in case of duplicate entries
            if (i > 0) i--;
            else if (mapgrid->adjacent_node_count[parent] == 0) return;
        }
    }
}

// remove all references to this child node
void vrx_pf_remove_references(const int child) {
    for (int i = 0; i < mapgrid->numnodes; i++) {
        vrx_pf_remove_link(child, i);
    }
}

void vrx_pf_move_node_references(const int newindex, const int lastindex) {
    for (int p = 0; p < mapgrid->numnodes; p++) {
        if (mapgrid->adjacent_node_count[p] == SIZE_MAX) continue;
        if (!mapgrid->adjacent_nodes[p]) continue;
        for (size_t k = 0; k < mapgrid->adjacent_node_count[p]; k++) {
            if (mapgrid->adjacent_nodes[p][k].nodenum == (size_t)lastindex)
                mapgrid->adjacent_nodes[p][k].nodenum = (size_t)newindex;
        }
    }
}

void vrx_pf_delete_node(const int nodenum) {
    vrx_pf_remove_references(nodenum);

    const int last_idx = mapgrid->numnodes - 1;

    // if this isn't the last node on the list, then copy the
    // vector stored at the end of the array to current position
    if (nodenum != last_idx) {
        VectorCopy(mapgrid->pathnode[last_idx], mapgrid->pathnode[nodenum]);
        mapgrid->nodeflags[nodenum] = mapgrid->nodeflags[last_idx];
        mapgrid->nodeent[nodenum] = mapgrid->nodeent[last_idx];

        if (mapgrid->adjacent_nodes[nodenum])
            free(mapgrid->adjacent_nodes[nodenum]);

        mapgrid->adjacent_nodes[nodenum] = mapgrid->adjacent_nodes[last_idx];
        mapgrid->adjacent_node_count[nodenum] = mapgrid->adjacent_node_count[last_idx];

        // rewrite all references from old index (last_idx) to new index (nodenum)
        // across every node's adjacency list. without this, links silently retarget
        // to whatever node was just swapped in.
        vrx_pf_move_node_references(nodenum, last_idx);
    }

    // clear the value stored at the end of the list
    VectorClear(mapgrid->pathnode[mapgrid->numnodes-1]);
    mapgrid->nodeflags[mapgrid->numnodes - 1] = NF_NONE;
    mapgrid->nodeent[mapgrid->numnodes - 1] = nullptr;

    mapgrid->adjacent_nodes[mapgrid->numnodes - 1] = nullptr;
    mapgrid->adjacent_node_count[mapgrid->numnodes - 1] = 0;

    // reduce the size of the list
    mapgrid->numnodes--;

    gridtree_regenerate();
}

void Cmd_DeleteNode_f(edict_t *ent) {
    int nearestNode;

    if (!ent->myskills.administrator)
        return;

    if ((nearestNode = vrx_pf_nearest_node_index(ent->s.origin, 255, true)) != -1)
        vrx_pf_delete_node(nearestNode);

    safe_cprintf(ent, PRINT_HIGH, "**Closest node deleted (%d nodes total).**\n", mapgrid->numnodes);
}


qboolean CheckBottom(const vec3_t pos, const vec3_t boxmin, const vec3_t boxmax) {
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
    trace = gi.trace(start, vec3_origin, vec3_origin, stop, nullptr,MASK_PATH /*MASK_MONSTERSOLID*/);

    if (trace.fraction == 1.0)
        return false;
    mid = bottom = trace.endpos[2];

    // the corners must be within 16 of the midpoint
    for (x = 0; x <= 1; x++)
        for (y = 0; y <= 1; y++) {
            start[0] = stop[0] = x ? maxs[0] : mins[0];
            start[1] = stop[1] = y ? maxs[1] : mins[1];

            trace = gi.trace(start, vec3_origin, vec3_origin, stop, nullptr, MASK_PATH /*MASK_MONSTERSOLID*/);

            if (trace.fraction != 1.0 && trace.endpos[2] > bottom)
                bottom = trace.endpos[2];
            if (trace.fraction == 1.0 || mid - trace.endpos[2] > 18)
                return false;
        }

    // c_yes++;
    return true;
}

// NOTE: max search distance for child links should be set to
// gap specified in nearbyGridNode + x/yevery + min1/max1 (e.g. 128+32+3=163)
qboolean vrx_node_has_siblings(vec3_t start) {
    for (int i = 0; i < mapgrid->numnodes; i++) {
        const auto valid = vrx_pf_is_valid_child_position(
            mapgrid->gap,
            start,
            mapgrid->pathnode[i]
        );

        if (!memcmp(&valid, &(linkvalidity_t){}, sizeof valid))
            continue;

        return true;
    }
    return false;
}



void vrx_pf_add_missing_reciprocals(int child) {
    vrx_pf_compute_adjacent_nodes(child, true);
    if (mapgrid->adjacent_node_count[child] == SIZE_MAX)
        return;
    for (size_t i = 0; i < mapgrid->adjacent_node_count[child]; i++) {
        const auto potentialParent = mapgrid->adjacent_nodes[child][i];

        // normalize the SIZE_MAX sentinel on the parent so we can safely append
        if (mapgrid->adjacent_node_count[potentialParent.nodenum] == SIZE_MAX)
            mapgrid->adjacent_node_count[potentialParent.nodenum] = 0;

        // check if we are missing from their list (compare against `child`, not loop index `i`)
        bool found = false;
        for (size_t j = 0; j < mapgrid->adjacent_node_count[potentialParent.nodenum]; j++) {
            if (mapgrid->adjacent_nodes[potentialParent.nodenum][j].nodenum == (size_t)child) {
                found = true;
                break;
            }
        }

        if (found)
            continue;

        const auto valid = vrx_pf_is_valid_child_node(potentialParent.nodenum, child);
        if (!memcmp(&valid, &(linkvalidity_t){}, sizeof valid))
            continue;

        const auto newSize = sizeof mapgrid->adjacent_nodes[potentialParent.nodenum][0] *
            (mapgrid->adjacent_node_count[potentialParent.nodenum] + 1);
        struct mapgrid_link_s* ptr = realloc(mapgrid->adjacent_nodes[potentialParent.nodenum], newSize);

        if (ptr) {
            mapgrid->adjacent_nodes[potentialParent.nodenum] = ptr;
            ptr[mapgrid->adjacent_node_count[potentialParent.nodenum]] = link_from_validity(child, valid);
            mapgrid->adjacent_node_count[potentialParent.nodenum]++;
        }
    }
}

// to be used at the generation stage
static bool try_add_node(int *cnt, vec3_t v, int *z, enum nodeflag_t flags) {
    vec3_t endpt;

    static constexpr vec3_t min1 = {0, 0, 0}; // width 6x6
    static constexpr vec3_t max1 = {0, 0, 0};

    static constexpr vec3_t min2 = {-16, -16, 0}; // width 32x32 (was 24x24)
    static constexpr vec3_t max2 = {+16, +16, 0};

    // Skip world locations in solid/lava/slime/window/ladder
    if (gi.pointcontents(v) & MASK_OPAQUE_PATH && !(flags & NF_PLAT)) {
        // az: can happen if we're adding a plat in what is probably
        // a valid location, the plat is BSP.
        (*z)--;
        return true;
    }
    // At this point,v(x,y,z) is a point in mid-air
    // Trace small bbox down to see what is below

    // Stop at world locations in solid/lava/slime/window/ladder
    bool skipTraceDown = flags & NF_PLATUPPER;
    vec3_t trace2_end;
    if (!skipTraceDown) {
        VectorSet(endpt, v[0], v[1], -8192);
        // az: this is opaque rather than opaque_path because we don't want that monstersolids that fence paths
        // count as valid locations to stop at.
        trace_t tr1 = gi.trace(v, min1, max1, endpt, nullptr,MASK_OPAQUE);

        // Set for-loop index to our endpt's grid(z)
        *z = gridz(tr1.endpos[2]);

        // Skip if trace endpt hit func entity.
        bool probablyPlat = (tr1.ent->use || tr1.ent->think || tr1.ent->blocked);
        bool allowPlat = flags & (NF_PLAT | NF_USER);
        if (tr1.ent && probablyPlat && !allowPlat)
            return true;

        // Skip if trace endpt hit lava/slime/window/ladder.
        // az: _now_ we check for monsterclip contents.
        if (tr1.contents & (CONTENTS_LAVA | CONTENTS_SLIME | CONTENTS_WINDOW | CONTENTS_MONSTERCLIP)) return true;
        // Skip if trace endpt hit non-walkable slope
        if (tr1.plane.normal[2] < 0.7) return true;

        // Test vertical clearance above v(x,y,z)
        VectorCopy(tr1.endpos, endpt);
        VectorCopy(tr1.endpos, trace2_end);
        endpt[2] += 32; // endpt at approx crouch height
    } else {
        VectorCopy(v, endpt);
        VectorCopy(v, trace2_end);
    }

    const trace_t tr2 = gi.trace(endpt, min2, max2, trace2_end, nullptr,MASK_OPAQUE_PATH);
    //GHz - push down instead of up

    // Skip if linewidth inside solid - too close to adjoining surface?
    if (tr2.startsolid || tr2.allsolid)
        return true;

    // GHz: check final position to see if it intersects with a solid
    trace_t solidchk = gi.trace(tr2.endpos, min2, max2, tr2.endpos, nullptr,MASK_OPAQUE_PATH);
    if (solidchk.fraction != 1.0 || solidchk.startsolid || solidchk.allsolid)
        return true;

    bool ignoreBottomCheck = skipTraceDown;
    if (!ignoreBottomCheck && !CheckBottom(tr2.endpos, min2, max2))
        return true;

    if (!ignoreBottomCheck) {
        VectorCopy(tr2.endpos, endpt); //GHz
        endpt[2] += 32; //GHz
    }

    // don't do the sibling check if we're manually editing
    const bool ignoreSiblingCheck = flags & (NF_USER | NF_PLAT);
    if (!ignoreSiblingCheck && vrx_node_has_siblings(endpt))
        return true; //GHz

    // Houston,we have a valid node!
    // copy to mapgrid->pathnode[] array
    VectorCopy(endpt, mapgrid->pathnode[*cnt]);
    mapgrid->nodeflags[*cnt] = flags;

    // initialize the rest of the slot to avoid inheriting stale state
    // (e.g. from a previously deleted node that was swapped into this slot)
    mapgrid->nodeent[*cnt] = nullptr;
    if (mapgrid->adjacent_nodes[*cnt]) {
        free(mapgrid->adjacent_nodes[*cnt]);
        mapgrid->adjacent_nodes[*cnt] = nullptr;
    }
    mapgrid->adjacent_node_count[*cnt] = 0;

    (*cnt)++;
    return false;
}

static void force_add_node(vec3_t pos) {
    vec3_t start;
    VectorCopy(pos, start);  // start was uninitialized before this fix
    start[2] -= 8192;
    const trace_t tr = gi.trace(pos, nullptr, nullptr, start, nullptr, MASK_SOLID);
    VectorCopy(tr.endpos, start);
    start[2] += 32;

    const int slot = mapgrid->numnodes;
    VectorCopy(start, mapgrid->pathnode[slot]);

    // initialize the slot fully so we don't inherit stale state from prior deletes
    mapgrid->nodeflags[slot] = NF_USER;
    mapgrid->nodeent[slot] = nullptr;
    if (mapgrid->adjacent_nodes[slot]) {
        free(mapgrid->adjacent_nodes[slot]);
        mapgrid->adjacent_nodes[slot] = nullptr;
    }
    mapgrid->adjacent_node_count[slot] = 0;

    mapgrid->numnodes++;
}

// to be used at non-generation stage
static bool try_add_node_with_links(vec3_t v, enum nodeflag_t flags, bool deferLinkCalculation) {
    int z;
    if (!try_add_node(&mapgrid->numnodes, v, &z, flags)) {
        if (!deferLinkCalculation)
            vrx_pf_add_missing_reciprocals(mapgrid->numnodes - 1);
        return false;
    }

    return true;
}

void Cmd_AddNode_f(edict_t *ent) {
    vec3_t start;

    if (!ent->myskills.administrator)
        return;

    const bool force = gi.argc() > 1 && !strcmp(gi.argv(1), "force");

    if (force) {
        force_add_node(ent->s.origin);
        gridtree_regenerate();
        vrx_pf_add_missing_reciprocals(mapgrid->numnodes - 1);
        return;
    }

    VectorCopy(ent->s.origin, start);
    if (!try_add_node_with_links(start, NF_USER, false)) {
        gridtree_regenerate();
        safe_cprintf(ent, PRINT_HIGH, "**Node added at current position (%d nodes total).\n**", mapgrid->numnodes);
    } else
        safe_cprintf(ent, PRINT_HIGH, "**Node addition failed at current position.\n**");
}

void Cmd_DeleteAllNodes_f(edict_t *ent) {
    if (!ent->myskills.administrator)
        return;

    // free adjacency lists FIRST, while numnodes still reflects valid entries
    vrx_free_adjacency_lists();

    memset(&mapgrid->pathnode, 0, mapgrid->numnodes * sizeof(vec3_t));
    memset(&mapgrid->nodeflags, 0, mapgrid->numnodes * sizeof(mapgrid->nodeflags[0]));
    memset(&mapgrid->nodeent, 0, mapgrid->numnodes * sizeof(mapgrid->nodeent[0]));
    memset(&mapgrid->adjacent_node_count, 0, mapgrid->numnodes * sizeof(mapgrid->adjacent_node_count[0]));
    mapgrid->numnodes = 0;

    gridtree_regenerate();

    safe_cprintf(ent, PRINT_HIGH, "All nodes deleted.\n");
}

#define GRID_MAGIC 0x10204066
#define VERSION 1

void vrx_pf_save_grid_new(void) {
    char filename[255];
    FILE *fptr;

    CreateDirIfNotExists(va("%s/settings/grd", gamedir->string));

    Com_sprintf(filename, sizeof(filename), "%s/settings/grd/%s.grx", game_path->string, level.mapname);

    if ((fptr = fopen(filename, "wb")) == nullptr) {
        gi.dprintf("Unable to save grid file: %s\n", filename);
        return;
    }

    WriteInteger(fptr, GRID_MAGIC);
    WriteInteger(fptr, VERSION);

    int remap[mapgrid->numnodes];
    int next_idx = 0;
    for (int i = 0; i < mapgrid->numnodes; i++) {
        if (mapgrid->nodeflags[i] & NF_NOSAVE)
            remap[i] = -1;
        else
            remap[i] = next_idx++;
    }

    int num_nodes_to_save = next_idx;

    WriteInteger(fptr, num_nodes_to_save);
    WriteFloat(fptr, mapgrid->gap);

    for (size_t i = 0; i < mapgrid->numnodes; i++) {
        if (mapgrid->nodeflags[i] & NF_NOSAVE)
            continue;

        fwrite(&mapgrid->pathnode[i], sizeof(vec3_t), 1, fptr);
        fwrite(&mapgrid->nodeflags[i], sizeof(enum nodeflag_t), 1, fptr);

        // it's a valid count, so let's exclude the nodes that are NOSAVE.
        uint64_t cnt = 0;
        if (mapgrid->adjacent_node_count[i] == SIZE_MAX)
            cnt = UINT64_MAX;
        else {
            for (int j = 0; j < mapgrid->adjacent_node_count[i]; j++) {
                // remove from the count the links that are NOSAVE.
                if (!(mapgrid->nodeflags[mapgrid->adjacent_nodes[i][j].nodenum] & NF_NOSAVE))
                    cnt++;
            }
        }

        fwrite(&cnt, sizeof cnt, 1, fptr);

        if (cnt > 0 && cnt != UINT64_MAX) {
            // we have to traverse all of the adjacent nodes to exclude the NOSAVE links
            for (int j = 0; j < mapgrid->adjacent_node_count[i]; j++) {
                size_t target_idx = mapgrid->adjacent_nodes[i][j].nodenum;
                if (mapgrid->nodeflags[target_idx] & NF_NOSAVE)
                    continue;

                const auto remapped_nodenum = remap[target_idx];
                const auto linkflags = mapgrid->adjacent_nodes[i][j].linkflags;

                fwrite(&remapped_nodenum, sizeof(remapped_nodenum), 1, fptr);
                fwrite(&linkflags, sizeof linkflags, 1, fptr);
            }
        }
    }

    fclose(fptr);
    gi.dprintf("Grid successfully saved.\n");
}

void make_doodad(vec3_t pos) {
    edict_t* doodad = G_Spawn();
    VectorCopy(pos, doodad->s.origin);
    gi.setmodel(doodad, "models/objects/gibs/bone/tris.md2");
    gi.linkentity(doodad);
}


void vrx_grd_create_ent_nodes(bool isGenerating) {
    const auto navi_count = vrx_inv_get_navi_count();
    bool navi_visited[navi_count] = {};
    for (size_t i = 0; i < navi_count; i++) {
        if (navi_visited[i])
            continue;

        navi_visited[i] = true;
        const edict_t *navi = vrx_inv_get_navi(i);
        int z;

        vec3_t start;
        VectorCopy(navi->s.origin, start);
        try_add_node_with_links(start, NF_ENTREF, isGenerating);

        if (!navi->target)
            break;

        edict_t *from = nullptr;
        while ((from = G_Find(from, FOFS(targetname), navi->target))) {
            vec3_t end, dir;
            VectorCopy(from->s.origin, end);
            VectorSubtract(end, start, dir);

            const float dist = VectorNormalize(dir);

            // add nodes along the path
            for (int j = 0; j < dist / mapgrid->gap; j++) {
                vec3_t nodepos;
                VectorMA(start, mapgrid->gap * (j + 1), dir, nodepos);
                try_add_node_with_links(nodepos, NF_ENTREF, isGenerating);
            }
        }
    }

    edict_t* plats = nullptr;
    while ((plats = G_Find(plats, FOFS(classname), "func_plat"))) {
        vec3_t topcenter;
        vec3_t top, bottom;

        // Upper node
        VectorSet(topcenter,
            (plats->maxs[0] - plats->mins[0]) * 0.5 + plats->mins[0],
            (plats->maxs[1] - plats->mins[1]) * 0.5 + plats->mins[1],
            plats->maxs[2]
        );

        VectorSet( top, topcenter[0], topcenter[1], topcenter[2] + 32 );

        const float height = plats->pos1[2] - plats->pos2[2];
        VectorSet(bottom,
            topcenter[0],
            topcenter[1],
            topcenter[2] - height + 32
        );

        // Upper node
        if (!try_add_node_with_links(top, NF_PLATUPPER | NF_ENTREF, true)) {
            mapgrid->nodeent[mapgrid->numnodes - 1] = plats;
            // if we're generating this link generation process will be done in bulk later
            // otherwise we can do it now, it is fine.
            if (!isGenerating)
                vrx_pf_add_missing_reciprocals(mapgrid->numnodes - 1);
        } else {
            gi.dprintf("grid: Could not add upper node for plat %d\n", plats->s.number);
            // make_doodad(top);
            continue;
        }

        // Lower node
        if (!try_add_node_with_links(bottom, NF_PLATLOWER | NF_ENTREF, true)) {
            mapgrid->nodeent[mapgrid->numnodes - 1] = plats;
            if (!isGenerating)
                vrx_pf_add_missing_reciprocals(mapgrid->numnodes - 1);
        } else {
            make_doodad(bottom);
            gi.dprintf("grid: Could not add lower node for plat %d\n", plats->s.number);
        }
    }
}

// also calculates adjacency!
void vrx_pf_cull_unlinked_nodes(void) {
    for (int i = 0; i < mapgrid->numnodes; i++) {
        const int children = vrx_pf_compute_adjacent_nodes(i, true);

        // delete nodes that have no children
        if (children < 1) {
            vrx_pf_delete_node(i);

            // we gotta recheck this node since the last one will replace this one lol
            i--;
        }
    }
}

qboolean vrx_pf_load_grid_old(void) {
    char filename[255];
    FILE *fptr;

    Com_sprintf(filename, sizeof(filename), "%s/settings/grd/%s.grd", game_path->string, level.mapname);

    if ((fptr = fopen(filename, "rb")) == nullptr) {
        gi.dprintf("Grid file not found.\n");
        return false;
    }

    fread(&mapgrid->pathnode[0], sizeof(vec3_t), MAX_GRID_SIZE, fptr);
    mapgrid->numnodes = ReadInteger(fptr);
    fclose(fptr);

    if (mapgrid->numnodes < 1) {
        gi.dprintf("Grid file is invalid and must be rebuilt.\n");
        return false;
    }
    gi.dprintf("Grid successfully loaded (%d nodes).\n", mapgrid->numnodes);
    mapgrid->gap = 256; // fixed, old value

    gridtree_regenerate();
    vrx_grd_create_ent_nodes(false);

    vrx_pf_cull_unlinked_nodes();
    return true;
}

qboolean vrx_pf_load_grid_new(void) {
    char filename[255];
    FILE *fptr;

    Com_sprintf(filename, sizeof(filename), "%s/settings/grd/%s.grx", game_path->string, level.mapname);

    if ((fptr = fopen(filename, "rb")) == nullptr) {
        return false;
    }

    const int magic = ReadInteger(fptr);
    if (magic != GRID_MAGIC) {
        gi.dprintf("bad magic on grd file\n");
        return false;
    }

    const int version = ReadInteger(fptr);
    if (version != VERSION) {
        gi.dprintf("unsupported grd version %d != %d\n", version, VERSION);
        return false;
    }

    mapgrid->numnodes = ReadInteger(fptr);
    mapgrid->gap = ReadFloat(fptr);

    if (mapgrid->numnodes < 1) {
        gi.dprintf("Grid file is invalid and must be rebuilt.\n");
        return false;
    }

    for (int i = 0; i < mapgrid->numnodes; i++) {
        fread(&mapgrid->pathnode[i], sizeof(vec3_t), 1, fptr);
        fread(&mapgrid->nodeflags[i], sizeof(enum nodeflag_t), 1, fptr);
        uint64_t cnt;
        fread(&cnt, sizeof cnt, 1, fptr);

        if (cnt == UINT64_MAX || !cnt)
            // don't read in this, there's nothing at all.
            mapgrid->adjacent_node_count[i] = SIZE_MAX;
        else {
            mapgrid->adjacent_node_count[i] = cnt;
            mapgrid->adjacent_nodes[i] = malloc(sizeof(struct mapgrid_link_s) * cnt);

            for (uint64_t j = 0; j < cnt; j++) {
                int32_t nodenum;
                uint32_t flags;
                fread(&nodenum, sizeof nodenum, 1, fptr);
                fread(&flags, sizeof flags, 1, fptr);
                mapgrid->adjacent_nodes[i][j].nodenum = (size_t)nodenum;
                mapgrid->adjacent_nodes[i][j].linkflags = (enum linkflag_t)flags;
            }
        }
    }

    fclose(fptr);

    gi.dprintf("Grid successfully loaded (%d nodes).\n", mapgrid->numnodes);
    vrx_grd_create_ent_nodes(false);
    gridtree_regenerate();
    return true;
}

void Cmd_SaveNodes_f(edict_t *ent) {
    if (!ent->myskills.administrator)
        return;

    safe_cprintf(ent, PRINT_HIGH, "Saving nodes...\n", mapgrid->numnodes);
    vrx_pf_save_grid_new();
}

void Cmd_LoadNodes_f(edict_t *ent) {
    if (!ent->myskills.administrator)
        return;

    safe_cprintf(ent, PRINT_HIGH, "Loading nodes...\n", mapgrid->numnodes);

    vrx_free_adjacency_lists();
    memset(mapgrid, 0, sizeof *mapgrid);
    if (!vrx_pf_load_grid_new()) {
        if (vrx_pf_load_grid_old()) {
            vrx_pf_save_grid_new();
            gi.dprintf("Updated old grid to new format\n");
        } else {
            gi.dprintf("Grid file not found.\n");
        }
    }
}




void vrx_pf_create_grid(qboolean force) {
    vrx_free_adjacency_lists();
    memset(mapgrid, 0, sizeof *mapgrid);
    mapgrid->gap = PF_DENSITY;

    if (!force) {
        if (!vrx_pf_load_grid_new()) {
            if (vrx_pf_load_grid_old()) {
                vrx_pf_save_grid_new();
                gi.dprintf("Old grid migrated.");
                return;
            }
        } else {
            return;
        }
    }

    for (int x = 0; x < maxx; x++) {
        const float v0 = g2v0(x); // convert grid(x) to v[0]
        for (int y = 0; y < maxy; y++) {
            const float v1 = g2v1(y); // convert grid(y) to v[1]
            for (int z = maxz - 1; z >= 0; z--) {
                const float v2 = g2v2(z); // convert grid(z) to v[2]
                vec3_t v;
                VectorSet(v, v0, v1, v2);
                try_add_node(&mapgrid->numnodes, v, &z, NF_NONE);
            }
        }
    }


    vrx_grd_create_ent_nodes(true);

    vrx_pf_cull_unlinked_nodes();

    gi.dprintf("%d Nodes Created\n", mapgrid->numnodes);

    //================== pathfinding stuff ================

    if (!force)
        vrx_pf_save_grid_new();

    gridtree_regenerate();
}

// #define _kddbg

void InitPathfinding() {
    vrx_gridgen_density = gi.cvar("vrx_gridgen_density", "32", CVAR_ARCHIVE);
    if (!mapgrid)
        mapgrid = malloc(sizeof(struct mapgrid_s));
    else
        vrx_free_adjacency_lists();

    *mapgrid = (struct mapgrid_s){};
    vrx_pf_create_grid(false);


#ifdef _kddbg
    for (int i = 0; i < gridtree->capacity; i++)
        gi.dprintf("%-3d => %3d\n", i, gridtree->data[i]);
#endif

    // az: reconstructed every map load depending on the # of map nodes
    if (pfctx)
        pfctx_free(&pfctx);

    pfctx = pfctx_create(mapgrid->numnodes + 2);
}

void ShutdownPathfinding() {
    pfctx_free(&pfctx);
    gridkdtree_free(&gridtree);

    if (mapgrid) {
        vrx_free_adjacency_lists();
        free(mapgrid);
        mapgrid = nullptr;
    }
}

void Cmd_ComputeNodes_f(edict_t *ent) {
    if (!ent->myskills.administrator)
        return;

    vrx_free_adjacency_lists();
    *mapgrid = (struct mapgrid_s){};
    vrx_pf_create_grid(true);
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
