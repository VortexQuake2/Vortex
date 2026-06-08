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

uint16_t kdtree_build(struct kdtbuildctx_s *ctx, const size_t dim) {
    if (ctx->slicesize == 0)
        return UINT16_MAX;

    cmpstr.srcdata = ctx->tree->srcdata;
    cmpstr.dim = dim;
    qsort(ctx->sortedx, ctx->slicesize, sizeof(ctx->sortedx[0]), compare_v);

    // size 10 -> 5 (0-5, 6-10)
    // size 9 -> 4 (0-4, 5-9)
    const auto _node = gkdt_alloc_node(ctx->tree);
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

    return _node - ctx->tree->nodes;
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
        tree->nodes[i].nodenum = NODEID_MAX;
        tree->nodes[i].left = UINT16_MAX;
        tree->nodes[i].right = UINT16_MAX;
    }

    tree->srcdata = srcdata;
    tree->nodecount = 0;
    tree->capacity = capacity;

    kdtree_build(&(struct kdtbuildctx_s){
        tree, sortedx, count
    }, 0);

    free(sortedx);
    return tree;
}


void kdtree_query(
    const struct gridkdtree_s* tree,
    const vec3_t querypos,
    const struct kdtree_node_s* node,
    const size_t dim,
    size_t *best,
    double *bestdist
) {
    // determine if we're a leaf, if so, the best node is us.
    if (node == nullptr)
        return;

    if (node->nodenum == NODEID_MAX)
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
    const auto pnear = near != UINT16_MAX ? &tree->nodes[near] : nullptr;
    const auto pfar = far != UINT16_MAX ? &tree->nodes[far] : nullptr;

    kdtree_query(
        tree, querypos,
        pnear, ndim, best, bestdist);

    if (sdist * sdist < *bestdist) {
        // check other subtree
        kdtree_query(
            tree, querypos,
            pfar, ndim, best, bestdist);
    }
};

size_t gridkdtree_query(const struct gridkdtree_s* tree, const vec3_t querypos) {
    size_t best = SIZE_MAX;
    double bestdist = INFINITY;
    kdtree_query(tree, querypos, &tree->nodes[0], 0, &best, &bestdist);
    return best;
}

struct gheap_s* gheap_create(const size_t capacity) {
    struct gheap_s *ret = malloc(sizeof (struct gheap_s) + sizeof (struct gheap_entry_s) * capacity);
    if (ret == nullptr) {
        return nullptr;
    }

    for (size_t i = 0; i < capacity; i++) {
        ret->entries[i].cost = INT32_MAX;
        ret->entries[i].data = nullptr;
    }

    ret->capacity = capacity;
    ret->count = 0;
    return ret;
}

void gheap_free(struct gheap_s** heap) {
    if (!*heap)
        return;

    free(*heap);
    *heap = nullptr;
}

static void hswap(struct gheap_entry_s *a, struct gheap_entry_s *b) {
    const struct gheap_entry_s tmp = *a;
    *a = *b;
    *b = tmp;
}

bool gheap_push(struct gheap_s* heap, const int32_t cost, void* data) {
    assert (data != nullptr);
    if (heap->count >= heap->capacity)
        return false;

    heap->count++;
    size_t i = heap->count - 1;
    heap->entries[i].cost = cost;
    heap->entries[i].data = data;

    if (i == 0)
        return true;

    while (i != 0) {
        const size_t _parent = (i - 1) / 2;
        struct gheap_entry_s *parent = &heap->entries[_parent];
        if (heap->entries[i].cost < parent->cost) {
            hswap(&heap->entries[i], parent);
            i = _parent;
        } else {
            break;
        }
    }

    return true;
}

int32_t gheap_peek_cost(const struct gheap_s* heap, const size_t node) {
    if (node >= heap->count)
        return INT32_MAX;

    const auto entry = &heap->entries[node];
    return entry->cost;
}


struct gheap_entry_s gheap_pop_inner(struct gheap_s* heap) {
    if (heap->count == 0)
        return (struct gheap_entry_s){INT32_MAX, nullptr};

    const auto ret = heap->entries[0];
    heap->count--;
    heap->entries[0] = heap->entries[heap->count];

    size_t node = 0;
    for (;;) {
        const auto ln = node * 2 + 1;
        const auto rn = node * 2 + 2;
        if (node >= heap->count)
            break;

        const auto cur = &heap->entries[node];
        size_t smallest = node;
        if (ln < heap->count && gheap_peek_cost(heap, ln) < cur->cost) {
            smallest = ln;
        }

        if (rn < heap->count && gheap_peek_cost(heap, rn) < heap->entries[smallest].cost) {
            smallest = rn;
        }

        if (smallest == node)
            break;

        hswap (cur, &heap->entries[smallest]);
        node = smallest;
    }

    return ret;
}

void gheap_reset(struct gheap_s* heap) {
    heap->count = 0;
}

void* gheap_pop(struct gheap_s* heap) {
    const auto ret = gheap_pop_inner(heap).data;
    return ret;
}