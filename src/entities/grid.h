#pragma once
#include <stdint.h>
#include <string.h>


typedef struct node_s node_t;

// numchilds should be set based on the maximum number of expected child nodes in search pattern
#define NUMCHILDS 12

struct node_s {
    int dist; // g-cost how far we've already gone from start to here
    float distestimation; // h-cost heuristic estimate of how far is left
    float totaldistestimation; // f-cost is total cost (estimated) from start to finish
    int nodenum; // index number of this node
    node_t *Child[NUMCHILDS];
    node_t *PrevNode;
    node_t *NextNode;
};

// min heap for open list
struct gheap_entry_s {
    int32_t cost;
    void* data;
    bool occupied;
};

struct gheap_s {
    size_t count;
    size_t capacity;
    struct gheap_entry_s entries[];
};

// bitmap for closed list
struct gbitmap_s {
    size_t capacity;
    uint8_t data[];
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
