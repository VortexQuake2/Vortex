#include "q_shared.h"
#include "grid.h"
#include "g_local.h"

struct gstack_s* gstack_create(const size_t capacity) {
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

struct nodearena_s* nodearena_create(const size_t capacity) {
    struct nodearena_s* arena = malloc(sizeof (node_t) * capacity + sizeof (struct nodearena_s));

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

void gridkdtree_free(struct gridkdtree_s** tree) {
    if (!*tree) return;
    free(*tree);
    *tree = nullptr;
}

thread_local struct cmpstr_s {
    uint8_t dim;
    vec3_t* srcdata;
} cmpstr;

struct kdtbuildctx_s {
    struct gridkdtree_s* tree;
    int* sortedx;
    size_t slicesize;
};

int compare_v(const void* _l, const void* _r) {
    const int l = *(const int*)_l;
    const int r = *(const int*)_r;
    const vec3_t* srcdata = cmpstr.srcdata;
    const uint8_t dim = cmpstr.dim;

    if (srcdata[l][dim] < srcdata[r][dim])
        return -1;
    if (srcdata[l][dim] > srcdata[r][dim])
        return 1;
    return 0;
}

struct kdtree_node_s* gkdt_alloc_node(struct gridkdtree_s* tree) {
    if (tree->nodecount < tree->capacity)
        return &tree->nodes[tree->nodecount++];
    return nullptr;
}

struct kdtree_node_s* kdtree_build(struct kdtbuildctx_s *ctx, const size_t dim) {
    if (ctx->slicesize == 0)
        return nullptr;

    cmpstr.srcdata = ctx->tree->srcdata;
    cmpstr.dim = dim;
    qsort(ctx->sortedx, ctx->slicesize, sizeof(ctx->sortedx[0]), compare_v);

    // size 10 -> 5 (0-5, 6-10)
    // size 9 -> 4 (0-4, 5-9)
    auto _node = gkdt_alloc_node(ctx->tree);
    const size_t mid = (ctx->slicesize - 1) / 2;
    _node->nodenum = ctx->sortedx[mid] ;

    const auto _dim = (dim + 1) % 3;
    _node->left = kdtree_build(&(struct kdtbuildctx_s) {
        ctx->tree, ctx->sortedx,
        mid
    }, _dim);
    _node->right = kdtree_build(&(struct kdtbuildctx_s) {
        ctx->tree,
        ctx->sortedx + mid + 1,
        ctx->slicesize - mid - 1
    }, _dim);

    return _node;
}

size_t nextPowerOfTwo(size_t n) {
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    return n;
}



struct gridkdtree_s* gridkdtree_create(vec3_t* srcdata, const size_t count) {
    int* sortedx = malloc(count * sizeof (int));

    const auto capacity = count;
    const auto size = sizeof(struct gridkdtree_s) +
        capacity * sizeof(struct kdtree_node_s);

    struct gridkdtree_s* tree = malloc(size);
    if (!tree || !sortedx) {
        if (tree) free(tree);
        if (sortedx) free(sortedx);
        return nullptr;
    }

    for (size_t i = 0; i < count; i++)
        sortedx[i] = i;

    for (size_t i = 0; i < capacity; i++) {
        tree->nodes[i].nodenum = SIZE_MAX;
        tree->nodes[i].left = nullptr;
        tree->nodes[i].right = nullptr;
    }

    tree->srcdata = srcdata;
    tree->nodecount = 0;
    tree->capacity = capacity;

    tree->root = kdtree_build(&(struct kdtbuildctx_s){
        tree, sortedx, count
    }, 0);

    free(sortedx);
    return tree;
}


void kdtree_query(
    struct gridkdtree_s* tree,
    vec3_t querypos,
    const struct kdtree_node_s* node,
    const size_t dim,
    size_t *best,
    double *bestdist
) {
    // determine if we're a leaf, if so, the best node is us.
    if (node == nullptr)
        return;

    if (node->nodenum == SIZE_MAX)
        return;

    const double dist = distanceSqr(querypos, tree->srcdata[node->nodenum]);

    if (dist < *bestdist) {
        *best = node->nodenum;
        *bestdist = dist;
    }

    // we're not a leaf! we should check the dimension.
    // hyperplane distance
    const double sdist = querypos[dim] - tree->srcdata[node->nodenum][dim];
    const auto near = sdist < 0 ? node->left : node->right;
    const auto far = sdist < 0 ? node->right : node->left;
    const size_t ndim = (dim + 1) % 3;

    kdtree_query(
        tree, querypos,
        near, ndim, best, bestdist);

    if (sdist * sdist < *bestdist) {
        // check other subtree
        kdtree_query(
            tree, querypos,
            far, ndim, best, bestdist);
    }
};

size_t gridkdtree_query(struct gridkdtree_s* tree, vec3_t querypos) {
    size_t best = SIZE_MAX;
    double bestdist = INFINITY;
    kdtree_query(tree, querypos, tree->root, 0, &best, &bestdist);
    return best;
}