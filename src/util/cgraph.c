#include "cgraph.h"

#include <stdlib.h>
#include <string.h>

static void adj_init(cg_adj_t *a, int has_weights) {
    a->dst = NULL; a->w = has_weights ? (float *)1 : NULL; a->n = 0; a->cap = 0;
    /* w starts as sentinel (1) to indicate "has weights but not allocated";
     * we swap it to NULL below and (re)allocate on first reserve. */
    a->w = NULL;
    (void)has_weights;
}

static void adj_free(cg_adj_t *a) {
    free(a->dst); a->dst = NULL;
    free(a->w);   a->w = NULL;
    a->n = 0; a->cap = 0;
}

static void adj_reserve(cg_adj_t *a, size_t need, int has_weights) {
    if (need <= a->cap) return;
    size_t nc = a->cap ? a->cap * 2 : 4;
    while (nc < need) nc *= 2;
    a->dst = (unsigned int *)realloc(a->dst, nc * sizeof(unsigned int));
    if (has_weights)
        a->w = (float *)realloc(a->w, nc * sizeof(float));
    a->cap = nc;
}

static void adj_push(cg_adj_t *a, unsigned int dst, float w, int has_weights) {
    adj_reserve(a, a->n + 1, has_weights);
    a->dst[a->n] = dst;
    if (has_weights) a->w[a->n] = w;
    a->n++;
}

/* Remove the first edge whose dst == target. Returns 1 if removed. */
static int adj_remove(cg_adj_t *a, unsigned int target, int has_weights) {
    for (size_t i = 0; i < a->n; i++) {
        if (a->dst[i] == target) {
            memmove(&a->dst[i], &a->dst[i + 1], (a->n - i - 1) * sizeof(unsigned int));
            if (has_weights)
                memmove(&a->w[i], &a->w[i + 1], (a->n - i - 1) * sizeof(float));
            a->n--;
            return 1;
        }
    }
    return 0;
}

static int adj_find(const cg_adj_t *a, unsigned int target) {
    for (size_t i = 0; i < a->n; i++)
        if (a->dst[i] == target) return (int)i;
    return -1;
}

void cg_init(cgraph_t *g, cg_mode_t mode, int has_weights) {
    g->mode = mode;
    g->has_weights = has_weights ? 1 : 0;
    g->out = NULL;
    g->in = NULL;
    g->nvertices = 0;
    g->vcap = 0;
    g->nedges = 0;
}

void cg_free(cgraph_t *g) {
    for (size_t v = 0; v < g->nvertices; v++) {
        adj_free(&g->out[v]);
        if (g->mode == CG_BIDIRECTIONAL) adj_free(&g->in[v]);
    }
    free(g->out); g->out = NULL;
    free(g->in);  g->in = NULL;
    g->nvertices = 0;
    g->vcap = 0;
    g->nedges = 0;
}

static void cg_grow(cgraph_t *g) {
    if (g->nvertices < g->vcap) return;
    size_t nc = g->vcap ? g->vcap * 2 : 8;
    g->out = (cg_adj_t *)realloc(g->out, nc * sizeof(cg_adj_t));
    if (g->mode == CG_BIDIRECTIONAL)
        g->in = (cg_adj_t *)realloc(g->in, nc * sizeof(cg_adj_t));
    g->vcap = nc;
}

unsigned int cg_add_vertex(cgraph_t *g) {
    cg_grow(g);
    unsigned int v = (unsigned int)g->nvertices++;
    adj_init(&g->out[v], g->has_weights);
    if (g->mode == CG_BIDIRECTIONAL) adj_init(&g->in[v], g->has_weights);
    return v;
}

void cg_add_edge(cgraph_t *g, unsigned int u, unsigned int v, float w) {
    adj_push(&g->out[u], v, w, g->has_weights);
    if (g->mode == CG_BIDIRECTIONAL) {
        adj_push(&g->in[v], u, w, g->has_weights);
    } else if (g->mode == CG_UNDIRECTED && u != v) {
        adj_push(&g->out[v], u, w, g->has_weights);
    }
    g->nedges++;
}

void cg_remove_edge(cgraph_t *g, unsigned int u, unsigned int v) {
    int r = adj_remove(&g->out[u], v, g->has_weights);
    if (g->mode == CG_BIDIRECTIONAL) {
        adj_remove(&g->in[v], u, g->has_weights);
    } else if (g->mode == CG_UNDIRECTED && u != v) {
        adj_remove(&g->out[v], u, g->has_weights);
    }
    if (r) g->nedges--;
}

int cg_has_edge(const cgraph_t *g, unsigned int u, unsigned int v) {
    return adj_find(&g->out[u], v) >= 0;
}

size_t cg_num_vertices(const cgraph_t *g) { return g->nvertices; }
unsigned long cg_num_edges(const cgraph_t *g) { return g->nedges; }

size_t cg_out_degree(const cgraph_t *g, unsigned int v) {
    return g->out[v].n;
}

size_t cg_in_degree(const cgraph_t *g, unsigned int v) {
    if (g->mode == CG_BIDIRECTIONAL) return g->in[v].n;
    if (g->mode == CG_UNDIRECTED) return g->out[v].n;
    /* CG_DIRECTED: not tracked, scan. */
    size_t c = 0;
    for (size_t u = 0; u < g->nvertices; u++)
        if (adj_find(&g->out[u], v) >= 0) c++;
    return c;
}

float cg_edge_weight(const cgraph_t *g, unsigned int u, unsigned int v) {
    int i = adj_find(&g->out[u], v);
    if (i < 0 || !g->has_weights) return 0.0f;
    return g->out[u].w[i];
}

void cg_set_edge_weight(cgraph_t *g, unsigned int u, unsigned int v, float w) {
    if (!g->has_weights) return;
    int i = adj_find(&g->out[u], v);
    if (i >= 0) g->out[u].w[i] = w;
    if (g->mode == CG_BIDIRECTIONAL) {
        i = adj_find(&g->in[v], u);
        if (i >= 0) g->in[v].w[i] = w;
    } else if (g->mode == CG_UNDIRECTED && u != v) {
        i = adj_find(&g->out[v], u);
        if (i >= 0) g->out[v].w[i] = w;
    }
}

void cg_copy_full(cgraph_t *dst, const cgraph_t *src, cg_mode_t dst_mode, int dst_has_weights) {
    cg_free(dst);
    cg_init(dst, dst_mode, dst_has_weights);
    for (size_t v = 0; v < src->nvertices; v++) cg_add_vertex(dst);
    for (unsigned int u = 0; u < src->nvertices; u++) {
        for (size_t i = 0; i < src->out[u].n; i++) {
            unsigned int v = src->out[u].dst[i];
            float w = src->has_weights ? src->out[u].w[i] : 0.0f;
            if (src->mode == CG_UNDIRECTED && v < u) continue;
            cg_add_edge(dst, u, v, w);
        }
    }
}

void cg_copy_topology(cgraph_t *dst, const cgraph_t *src, cg_mode_t dst_mode) {
    cg_copy_full(dst, src, dst_mode, 0);
}

/* Union-Find based undirected connected components over out[] (for undirected
 * the adjacency lists are already symmetric; for directed-bidirectional we
 * explicitly union both directions — this matches boost's default which treats
 * the graph as undirected for this algorithm). */
size_t cg_connected_components(const cgraph_t *g, unsigned int *comp) {
    if (g->nvertices == 0) return 0;
    unsigned int *p = (unsigned int *)malloc(g->nvertices * sizeof(unsigned int));
    for (size_t i = 0; i < g->nvertices; i++) p[i] = (unsigned int)i;
    /* find with path compression */
    /* Using explicit while loops to avoid recursion issues on large graphs. */
    #define FIND(r, x) do {                                \
        (r) = (x);                                         \
        while (p[r] != (r)) (r) = p[r];                    \
        unsigned int _cur = (x);                           \
        while (p[_cur] != (r)) {                           \
            unsigned int _n = p[_cur]; p[_cur] = (r); _cur = _n; \
        }                                                  \
    } while (0)

    for (unsigned int u = 0; u < g->nvertices; u++) {
        for (size_t i = 0; i < g->out[u].n; i++) {
            unsigned int v = g->out[u].dst[i];
            unsigned int ru, rv;
            FIND(ru, u);
            FIND(rv, v);
            if (ru != rv) p[ru] = rv;
        }
    }
    #undef FIND
    /* renumber */
    size_t *map = (size_t *)malloc(g->nvertices * sizeof(size_t));
    for (size_t i = 0; i < g->nvertices; i++) map[i] = (size_t)-1;
    size_t next = 0;
    for (unsigned int v = 0; v < g->nvertices; v++) {
        unsigned int r = v;
        while (p[r] != r) r = p[r];
        if (map[r] == (size_t)-1) map[r] = next++;
        comp[v] = (unsigned int)map[r];
    }
    free(p);
    free(map);
    return next;
}
