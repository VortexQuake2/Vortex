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
    node_t *child[NUMCHILDS];
    node_t *prev;
    node_t *next;
    int dist; // g-cost how far we've already gone from start to here
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

void gridkdtree_free(struct gridkdtree_s** tree);
struct gridkdtree_s* gridkdtree_create(vec3_t* srcdata, size_t count);
size_t gridkdtree_query(struct gridkdtree_s* tree, vec3_t querypos);

struct gheap_s* gheap_create(size_t capacity);
void gheap_free(struct gheap_s** heap);
bool gheap_push(struct gheap_s* heap, int32_t cost, void* data);
void* gheap_pop(struct gheap_s* heap);
void gheap_reset(struct gheap_s* heap);
