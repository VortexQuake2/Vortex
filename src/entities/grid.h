#pragma once
#include <stdint.h>
#include <string.h>

#include "q_shared.h"


typedef struct node_s node_t;

// numchilds should be set based on the maximum number of expected child nodes in search pattern
#define NUMCHILDS 12

#define MAX_GRID_SIZE	10000

typedef uint16_t nodeid_t;
#define NODEID_MAX UINT16_MAX

static_assert(MAX_GRID_SIZE < NODEID_MAX, "max grid size overflows nodeid_t!");

// kinda lifted straight from jabot
enum listtype_t : uint8_t {
    LIST_NONE,
    LIST_OPEN,
    LIST_CLOSED
};

struct node_s {
    int dist; // g-cost how far we've already gone from start to here
    nodeid_t child[NUMCHILDS];
    nodeid_t prev;
    nodeid_t nodenum; // index number of this node
    enum listtype_t list;
};

// min heap for open list
struct gheap_entry_s {
    int32_t cost;
    void* data;
};

struct gheap_s {
    size_t count;
    size_t capacity;
    struct gheap_entry_s entries[];
};

// single-allocation node stack (for breadth-first search)
struct gstack_s {
    size_t capacity;
    size_t count;

    void* data[];
};

struct nodearena_s {
    size_t capacity;
    size_t count;
    node_t nodes[];
};

enum griddebug_state_t {
    GD_OFF,
    GD_NEARBY,
    GD_CHILD,
    GD_AIMSPOT,
    GD_MAX
};

struct kdtree_node_s {
    nodeid_t nodenum;
    // it's uint16_t, but it should match nodeid_t.
    uint16_t left;
    uint16_t right;
};

struct gridkdtree_s {
    size_t nodecount;
    size_t capacity;
    vec3_t* srcdata;
    struct kdtree_node_s nodes[];
};

struct gstack_s* gstack_create(size_t capacity);
void* gstack_top(struct gstack_s* stack);
void* gstack_pop(struct gstack_s* stack);
void gstack_push(struct gstack_s* stack, void* element);
void gstack_free(struct gstack_s** stack);
void gstack_reset(struct gstack_s* stack);

void nodearena_free(struct nodearena_s** arena);
struct nodearena_s* nodearena_create(size_t capacity);
node_t* nodearena_alloc(struct nodearena_s* arena);
void nodearena_reset(struct nodearena_s* arena);
node_t* nodearena_get(struct nodearena_s* arena, nodeid_t id);
nodeid_t nodearena_indexof(struct nodearena_s* arena, node_t* node);

void gridkdtree_free(struct gridkdtree_s** tree);
struct gridkdtree_s* gridkdtree_create(vec3_t* srcdata, size_t count);
size_t gridkdtree_query(const struct gridkdtree_s* tree, const vec3_t querypos);

struct gheap_s* gheap_create(size_t capacity);
void gheap_free(struct gheap_s** heap);
bool gheap_push(struct gheap_s* heap, int32_t cost, void* data);
void* gheap_pop(struct gheap_s* heap);
void gheap_reset(struct gheap_s* heap);

enum nodeflag_t : uint8_t {
    NF_NONE,

    // node cannot be walked on
    NF_NOWALK = U8BIT(1),

    // node is lower end of plat
    NF_PLATLOWER = U8BIT(2),

    // node is upper end of plat
    NF_PLATUPPER = U8BIT(3),

    // node was created referencing entity data
    NF_ENTREF = U8BIT(4),

    // the user added this thing
    NF_USER = U8BIT(5),

    // node is plat
    NF_PLAT = NF_PLATLOWER | NF_PLATUPPER,

    // do not save this node to disk if...
    NF_NOSAVE = NF_PLAT | NF_ENTREF
};

enum linkflag_t : uint8_t {
    LF_NONE,
    LF_FLY = U8BIT(1),
    LF_WALK = U8BIT(2),
    LF_FALL = U8BIT(3),
    LF_PLATFORM = U8BIT(4),
};

struct mapgrid_link_s {
    nodeid_t nodenum;
    enum linkflag_t linkflags;
};

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

#define validity_empty(v) (v.walk == false && v.fly == false && v.fall == false && v.platform == false)

enum searchtype_t {
    SEARCHTYPE_WALK = 1,	// find nodes on horizontal plane with limited Z delta
    SEARCHTYPE_FLY = 2     // find nodes regardless of Z delta between start end ending positions
   };

// pathfinding
bool vrx_pf_nearest_node_location(vec3_t start, vec3_t node_loc, float range, qboolean vis);
int vrx_pf_nearest_node_index(vec3_t start, float range, qboolean vis);
int vrx_pf_find_path(enum searchtype_t searchType, vec3_t start, vec3_t destination);
int vrx_copy_path_waypoints(nodeid_t *wp, int max);
int vrx_pf_nearest_waypoint_index_along_path(vec3_t start, const nodeid_t *wp, size_t wpcount);
void vrx_pf_get_node_position(int nodenum, vec3_t pos);
enum nodeflag_t vrx_pf_get_nodeflags(const nodeid_t node);

// subsystem
void InitPathfinding(); // every map load
void ShutdownPathfinding(); // shutdowngame time

// debug
void vrx_pf_draw_path(const edict_t *ent);

// randomness
qboolean vrx_pf_get_grid_position(vec3_t pos, int index);
qboolean vrx_pf_get_random_grid_position(vec3_t pos);
int vrx_pf_get_node_count();

// grid manipulation
void vrx_pf_remove_link(const nodeid_t child, const nodeid_t parent);
void vrx_pf_delete_node(const nodeid_t nodenum);
bool vrx_pf_add_link(const nodeid_t child, const nodeid_t potentialParent, const linkvalidity_t valid);
struct mapgrid_link_s* vrx_pf_is_linked(const nodeid_t child, const nodeid_t parent);
struct mapgrid_link_s* vrx_pf_get_links(const nodeid_t parent);
void vrx_pf_add_missing_reciprocals(const nodeid_t child);
uint8_t vrx_pf_get_link_count(const nodeid_t parent);
