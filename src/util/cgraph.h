/* Minimal C graph library to replace the boost::adjacency_list /
 * boost::adjacency_matrix usage in SDCCtree_dec, SDCCnaddr, SDCClospre,
 * and SDCCralloc.
 *
 * Design:
 *  - Vertex descriptors are sequential unsigned ints (0..nvertices-1), matching
 *    boost::vecS semantics.
 *  - Topology only. Per-vertex bundled properties are kept in caller-owned
 *    arrays indexed by vertex id.
 *  - Three modes:
 *      CG_BIDIRECTIONAL  — directed graph with incoming-edge tracking.
 *      CG_DIRECTED       — directed graph without incoming-edge tracking.
 *      CG_UNDIRECTED     — undirected graph (add_edge mirrors to both sides).
 *  - Optional per-edge float weight.
 *  - No edge descriptors. Callers walk adjacency lists directly via CG_FOREACH
 *    macros or returned (dst, weight) pairs.
 */
#ifndef KCC_CGRAPH_H
#define KCC_CGRAPH_H

#include <stddef.h>

typedef enum {
    CG_DIRECTED = 0,
    CG_BIDIRECTIONAL = 1,
    CG_UNDIRECTED = 2
} cg_mode_t;

typedef struct {
    unsigned int *dst;
    float        *w;        /* NULL if graph has no weights */
    size_t        n;
    size_t        cap;
} cg_adj_t;

typedef struct cgraph {
    cg_mode_t    mode;
    int          has_weights;
    cg_adj_t    *out;       /* out[v] */
    cg_adj_t    *in;        /* in[v], only used when mode == CG_BIDIRECTIONAL */
    size_t       nvertices;
    size_t       vcap;
    unsigned long nedges;   /* counts directed edges; undirected each counted once */
} cgraph_t;

void          cg_init(cgraph_t *g, cg_mode_t mode, int has_weights);
void          cg_free(cgraph_t *g);
unsigned int  cg_add_vertex(cgraph_t *g);
/* add_edge: weight ignored if !has_weights. Always succeeds. */
void          cg_add_edge(cgraph_t *g, unsigned int u, unsigned int v, float w);
/* remove_edge: no-op if not present. For undirected, removes from both sides. */
void          cg_remove_edge(cgraph_t *g, unsigned int u, unsigned int v);
int           cg_has_edge(const cgraph_t *g, unsigned int u, unsigned int v);
size_t        cg_num_vertices(const cgraph_t *g);
unsigned long cg_num_edges(const cgraph_t *g);
size_t        cg_out_degree(const cgraph_t *g, unsigned int v);
size_t        cg_in_degree(const cgraph_t *g, unsigned int v);
/* Returns weight of edge (u,v). Caller is responsible that the edge exists
 * and has_weights is set. */
float         cg_edge_weight(const cgraph_t *g, unsigned int u, unsigned int v);
/* Find the edge and update its weight. No-op if not present. */
void          cg_set_edge_weight(cgraph_t *g, unsigned int u, unsigned int v, float w);

/* Copy topology (no weights carried). dst is cleared first. */
void          cg_copy_topology(cgraph_t *dst, const cgraph_t *src, cg_mode_t dst_mode);
/* Copy topology preserving weights (if src has them and dst_has_weights). */
void          cg_copy_full(cgraph_t *dst, const cgraph_t *src, cg_mode_t dst_mode, int dst_has_weights);

/* connected_components: fills comp[v] with 0-based component id. Returns
 * number of components. Treats graph as undirected. comp must be size nvertices. */
size_t        cg_connected_components(const cgraph_t *g, unsigned int *comp);

/* ---- iteration helpers ----
 *
 * Usage:
 *   CG_FOREACH_OUT(g, v, i, nbr, wt) {
 *       // use nbr (unsigned int) and wt (float, 0 if no weights)
 *   }
 * i is a user-provided size_t variable; serves as loop index.
 */

#define CG_FOREACH_OUT(g, v, i, nbr, wt)                                       \
    for ((i) = 0;                                                              \
         (i) < (g)->out[(v)].n &&                                              \
             ((nbr) = (g)->out[(v)].dst[(i)],                                  \
              (wt) = (g)->out[(v)].w ? (g)->out[(v)].w[(i)] : 0.0f, 1);        \
         (i)++)

#define CG_FOREACH_IN(g, v, i, src, wt)                                        \
    for ((i) = 0;                                                              \
         (i) < (g)->in[(v)].n &&                                               \
             ((src) = (g)->in[(v)].dst[(i)],                                   \
              (wt) = (g)->in[(v)].w ? (g)->in[(v)].w[(i)] : 0.0f, 1);          \
         (i)++)

#endif
