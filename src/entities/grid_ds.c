#include "q_shared.h"
#include "grid.h"
#include "g_local.h"

struct gstack_s* gstack_create(size_t capacity) {
    struct gstack_s* ret = malloc(sizeof(struct gstack_s) + capacity * sizeof (void*));
    if (!ret) {
        return nullptr;
    }
    ret->capacity = capacity;
    ret->count = 0;
    return ret;
}

void* gstack_top(struct gstack_s* stack) {
    if (stack->count == 0) {
        return nullptr;
    }
    return stack->data[stack->count - 1];
}

void* gstack_pop(struct gstack_s* stack) {
    if (stack->count == 0) {
        return nullptr;
    }
    stack->count--;
    return stack->data[stack->count];
}


void gstack_push(struct gstack_s* stack, void* element) {
    if (stack->count >= stack->capacity) {
        gi.error("gstack_push: stack capacity exceeded");
        return;
    }
    stack->data[stack->count] = element;
    stack->count++;
}

void gstack_free(struct gstack_s** stack) {
    if (!*stack)
        return;

    free(*stack);
    *stack = nullptr;
}

void gstack_reset(struct gstack_s* stack) {
    stack->count = 0;
}

void nodearena_free(struct nodearena_s **arena) {
    if (!*arena)
        return;

    free(*arena);
    *arena = nullptr;
}

struct nodearena_s* nodearena_create(size_t capacity) {
    struct nodearena_s* arena = malloc(sizeof (node_t) * capacity);

    if (!arena)
        return nullptr;

    arena->capacity = capacity;
    arena->count = 0;

    return arena;
}

node_t * nodearena_alloc(struct nodearena_s *arena) {
    if (arena->capacity <= arena->count + 1) {
        gi.error("nodearena_alloc: arena capacity exceeded");
        return nullptr;
    }

    return &arena->nodes[arena->count++];
}

void nodearena_reset(struct nodearena_s* arena) {
    arena->count = 0;
}
