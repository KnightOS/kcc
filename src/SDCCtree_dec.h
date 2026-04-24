/* Tree decomposition — C port of SDCCtree_dec.hpp.
 *
 * A tree decomposition of a graph G is a tree T whose vertices (bags) are
 * subsets of V(G) satisfying the tree-decomposition properties. We use the
 * Thorup heuristic to build T from an elimination ordering, then "nicify"
 * it to produce a nice tree decomposition suitable for dynamic-programming
 * register allocation and related optimizations.
 *
 * See the original header for the algorithms' references (Thorup 1998).
 */
#ifndef KCC_SDCCTREE_DEC_H
#define KCC_SDCCTREE_DEC_H

#include <stddef.h>

#include "util/cgraph.h"
#include "util/uiset.h"

typedef struct tree_dec {
    cgraph_t   g;        /* bidirectional tree, no edge weights */
    uiset_t   *bag;      /* bag[v] — set of input-graph vertex indices */
    unsigned  *weight;   /* weight[v] — filled by nicify_diffs_more */
    size_t     cap;      /* capacity of bag[]/weight[] */
} tree_dec_t;

void         tree_dec_init(tree_dec_t *t);
void         tree_dec_free(tree_dec_t *t);
unsigned int tree_dec_add_vertex(tree_dec_t *t);

/* Build a tree decomposition of cfg using Thorup's heuristic. */
void         tree_dec_thorup(tree_dec_t *out, const cgraph_t *cfg);

/* Transform T into a nice tree decomposition. T must already be rooted-like:
 * our build produces an in-tree with exactly one source per component. */
void         tree_dec_nicify(tree_dec_t *t);

/* Find the (single) root of the tree T (walks up in_edges until there are
 * none). */
unsigned int tree_dec_find_root(const tree_dec_t *t);

#endif
