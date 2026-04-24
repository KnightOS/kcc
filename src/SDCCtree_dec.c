/* Tree decomposition — C port of SDCCtree_dec.hpp.
 *
 * See the original header for references. The algorithms are Thorup's
 * heuristic elimination-ordering tree decomposition (D and E), plus the
 * three nicify passes that produce a nice tree decomposition. */

#include "SDCCtree_dec.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---------- tree_dec lifecycle ---------- */

void tree_dec_init(tree_dec_t *t) {
    cg_init(&t->g, CG_BIDIRECTIONAL, 0);
    t->bag = NULL;
    t->weight = NULL;
    t->cap = 0;
}

void tree_dec_free(tree_dec_t *t) {
    for (size_t v = 0; v < t->g.nvertices; v++) uiset_free(&t->bag[v]);
    free(t->bag);
    free(t->weight);
    cg_free(&t->g);
    t->bag = NULL;
    t->weight = NULL;
    t->cap = 0;
}

static void tree_dec_grow(tree_dec_t *t) {
    if (t->g.nvertices < t->cap) return;
    size_t nc = t->cap ? t->cap * 2 : 8;
    t->bag = (uiset_t *)realloc(t->bag, nc * sizeof(uiset_t));
    t->weight = (unsigned *)realloc(t->weight, nc * sizeof(unsigned));
    t->cap = nc;
}

unsigned int tree_dec_add_vertex(tree_dec_t *t) {
    tree_dec_grow(t);
    unsigned int v = cg_add_vertex(&t->g);
    uiset_init(&t->bag[v]);
    t->weight[v] = 0;
    return v;
}

/* ---------- multimap<unsigned int, unsigned int> helper ---------- */

typedef struct {
    unsigned int k, v;
} mm_pair_t;

typedef struct {
    mm_pair_t *items;
    size_t n, cap;
    int sorted;
} uu_mmap_t;

static void mm_init(uu_mmap_t *m) {
    m->items = NULL; m->n = 0; m->cap = 0; m->sorted = 1;
}

static void mm_free(uu_mmap_t *m) {
    free(m->items); m->items = NULL; m->n = 0; m->cap = 0;
}

static void mm_insert(uu_mmap_t *m, unsigned int k, unsigned int v) {
    if (m->n == m->cap) {
        m->cap = m->cap ? m->cap * 2 : 8;
        m->items = (mm_pair_t *)realloc(m->items, m->cap * sizeof(mm_pair_t));
    }
    m->items[m->n].k = k; m->items[m->n].v = v; m->n++;
    m->sorted = 0;
}

static int mm_cmp(const void *a, const void *b) {
    const mm_pair_t *x = (const mm_pair_t *)a;
    const mm_pair_t *y = (const mm_pair_t *)b;
    if (x->k != y->k) return x->k < y->k ? -1 : 1;
    if (x->v != y->v) return x->v < y->v ? -1 : 1;
    return 0;
}

static void mm_sort(uu_mmap_t *m) {
    if (m->sorted) return;
    qsort(m->items, m->n, sizeof(mm_pair_t), mm_cmp);
    m->sorted = 1;
}

/* Returns the start index of elements with key k; stores end in *end_out. */
static size_t mm_equal_range(uu_mmap_t *m, unsigned int k, size_t *end_out) {
    mm_sort(m);
    size_t lo = 0, hi = m->n;
    while (lo < hi) {
        size_t mid = lo + ((hi - lo) >> 1);
        if (m->items[mid].k < k) lo = mid + 1; else hi = mid;
    }
    size_t start = lo;
    hi = m->n;
    while (lo < hi) {
        size_t mid = lo + ((hi - lo) >> 1);
        if (m->items[mid].k <= k) lo = mid + 1; else hi = mid;
    }
    *end_out = lo;
    return start;
}

/* ---------- ordered list<unsigned int> helper (thorup ordering) ----------
 * Used as elimination ordering and then iterated in reverse. Array is
 * sufficient. */
typedef struct {
    unsigned int *items;
    size_t n, cap;
} ui_list_t;

static void ui_list_init(ui_list_t *l) { l->items = NULL; l->n = 0; l->cap = 0; }
static void ui_list_free(ui_list_t *l) { free(l->items); l->items = NULL; l->n = 0; l->cap = 0; }
static void ui_list_clear(ui_list_t *l) { l->n = 0; }
static void ui_list_push(ui_list_t *l, unsigned int v) {
    if (l->n == l->cap) {
        l->cap = l->cap ? l->cap * 2 : 8;
        l->items = (unsigned int *)realloc(l->items, l->cap * sizeof(unsigned int));
    }
    l->items[l->n++] = v;
}

/* ---------- Thorup D ---------- */
/* See SDCCtree_dec.hpp for the algorithm. */
static void thorup_D(ui_list_t *l,
                     uu_mmap_t *MJ,
                     uu_mmap_t *MS,
                     unsigned int n) {
    /* m: unsigned int -> unsigned int, sorted by key */
    typedef struct { unsigned int k, v; } kv_t;
    kv_t *m = (kv_t *)malloc(n * sizeof(kv_t));
    char *has = (char *)calloc(n, 1);
    unsigned int *slot = (unsigned int *)malloc(n * sizeof(unsigned int));
    size_t m_n = 0;
    unsigned int i = 0;

    ui_list_clear(l);

    for (unsigned int j = n; j > 0;) {
        j--;
        if (!has[j]) {
            has[j] = 1;
            slot[j] = i;
            m[m_n].k = j; m[m_n].v = i; m_n++;
            i++;
        }

        size_t e;
        size_t a = mm_equal_range(MS, j, &e);
        for (size_t k = a; k < e; k++) {
            unsigned int kv = MS->items[k].v;
            if (!has[kv]) {
                has[kv] = 1;
                slot[kv] = i;
                m[m_n].k = kv; m[m_n].v = i; m_n++;
                i++;
            }
        }

        a = mm_equal_range(MJ, j, &e);
        for (size_t k = a; k < e; k++) {
            unsigned int kv = MJ->items[k].v;
            if (!has[kv]) {
                has[kv] = 1;
                slot[kv] = i;
                m[m_n].k = kv; m[m_n].v = i; m_n++;
                i++;
            }
        }
    }

    /* build v[i] = key by inverting m: we have slot[k] = i */
    unsigned int *inv = (unsigned int *)malloc(n * sizeof(unsigned int));
    for (size_t mi = 0; mi < m_n; mi++) inv[m[mi].v] = m[mi].k;

    for (unsigned int k = 0; k < n; k++) ui_list_push(l, inv[k]);

    free(m); free(has); free(slot); free(inv);
}

/* ---------- Thorup E ---------- */
/* Build M from graph I. We iterate i = 0..n-1; j = max(i, max neighbour
 * index). Use a stack of (first, second) pairs. */
typedef struct { int first; unsigned int second; } e_pair_t;

static void thorup_E(uu_mmap_t *M, const cgraph_t *I) {
    size_t n = cg_num_vertices(I);
    e_pair_t *stk = (e_pair_t *)malloc((n + 2) * sizeof(e_pair_t));
    size_t sp = 0;

    /* M.clear() is implied by caller; we just append. */
    stk[sp].first = -1;
    stk[sp].second = (unsigned int)n;
    sp++;

    for (unsigned int i = 0; i < n; i++) {
        unsigned int j = i;
        for (size_t k = 0; k < I->out[i].n; k++) {
            unsigned int nb = I->out[i].dst[k];
            if (nb > j) j = nb;
        }
        if (j == i) continue;

        while (stk[sp - 1].second <= i) {
            /* M.insert(pair(second, first)) */
            mm_insert(M, stk[sp - 1].second, (unsigned int)stk[sp - 1].first);
            sp--;
        }

        unsigned int i2 = i;
        while (j >= stk[sp - 1].second && stk[sp - 1].second > i2) {
            i2 = (unsigned int)stk[sp - 1].first;
            sp--;
        }

        stk[sp].first = (int)i2;
        stk[sp].second = j;
        sp++;
    }

    /* Flush stack, except the initial sentinel. */
    while (sp > 1) {
        mm_insert(M, stk[sp - 1].second, (unsigned int)stk[sp - 1].first);
        sp--;
    }

    free(stk);
}

/* ---------- thorup_elimination_ordering ---------- */

static void thorup_elimination_ordering(ui_list_t *l, const cgraph_t *G) {
    /* J: directed copy of G with sequential edges (i, i+1) removed. */
    cgraph_t J, S;
    cg_init(&J, CG_DIRECTED, 0);
    cg_init(&S, CG_UNDIRECTED, 0);

    cg_copy_topology(&J, G, CG_DIRECTED);
    size_t nJ = cg_num_vertices(&J);
    if (nJ > 0) {
        for (unsigned int i = 0; i + 1 < nJ; i++)
            cg_remove_edge(&J, i, i + 1);
    }

    /* S: undirected copy of J. */
    cg_copy_topology(&S, &J, CG_UNDIRECTED);

    uu_mmap_t MJ, MS;
    mm_init(&MJ);
    mm_init(&MS);

    thorup_E(&MJ, &J);
    thorup_E(&MS, &S);

    thorup_D(l, &MJ, &MS, (unsigned int)nJ);

    mm_free(&MJ);
    mm_free(&MS);
    cg_free(&J);
    cg_free(&S);
}

/* ---------- find_bag ---------- */
/* Linear scan over tree vertices for the LAST (newest) bag that includes
 * all elements of X. Returns (size_t)-1 if not found. */
static size_t find_bag_index(const uiset_t *X, const tree_dec_t *T) {
    size_t found = (size_t)-1;
    for (size_t t = 0; t < T->g.nvertices; t++) {
        if (uiset_includes(&T->bag[t], X)) found = t;
    }
    if (found == (size_t)-1) {
        fprintf(stderr, "find_bag() failed.\n");
        fflush(stderr);
    }
    return found;
}

/* ---------- make_clique ---------- */
static void make_clique(const uiset_t *X, cgraph_t *G) {
    for (size_t i = 0; i < X->n; i++)
        for (size_t j = i + 1; j < X->n; j++)
            cg_add_edge(G, X->items[i], X->items[j], 0.0f);
}

/* ---------- add_vertices_to_tree_decomposition (iterative) ---------- *
 * The original is tail-recursive over the elimination ordering. We flatten it
 * to an iterative loop to avoid blowing the C stack on long elim orderings.
 */
static void add_vertices_to_tree_decomposition(tree_dec_t *T,
                                               const unsigned int *order,
                                               size_t n_order,
                                               cgraph_t *G,
                                               char *active) {
    /* Recursion unrolled: we first recurse on the tail with active[v]=false,
     * THEN attach the current bag. To flatten, we must simulate the same
     * order: emit the deepest recursion's base case (empty bag) first,
     * then walk back up attaching bags.
     *
     * Process order[n_order-1], order[n_order-2], ...: for each v we compute
     * its neighbours (in the current, reduced G), mark v inactive, then add
     * its clique edges. At the BASE of recursion we emit a vertex. Then, on
     * unwinding, we attach (neighbours ∪ {v}) as a new bag connected to the
     * bag found via find_bag(neighbours).
     *
     * Stack per step: stored (v, neighbours) from forward pass, replayed
     * backwards on unwind. */

    if (n_order == 0) {
        tree_dec_add_vertex(T);  /* base case: empty bag */
        return;
    }

    /* Forward pass: collect per-step neighbour sets and mark-inactive ops. */
    uiset_t *nbrs = (uiset_t *)malloc(n_order * sizeof(uiset_t));
    unsigned int *vs = (unsigned int *)malloc(n_order * sizeof(unsigned int));

    for (size_t step = 0; step < n_order; step++) {
        /* Original recursion: v = order[step], then recurse on step+1.
         * active[neighbour] guarded; compute neighbours of v. */
        unsigned int v = order[step];
        vs[step] = v;
        uiset_init(&nbrs[step]);
        for (size_t k = 0; k < G->out[v].n; k++) {
            unsigned int nb = G->out[v].dst[k];
            if (active[nb]) uiset_insert(&nbrs[step], nb);
        }
        active[v] = 0;
        make_clique(&nbrs[step], G);
    }

    /* Base case of deepest recursion. */
    tree_dec_add_vertex(T);

    /* Unwind: attach bags from innermost to outermost (reverse of step). */
    for (size_t si = n_order; si > 0; si--) {
        size_t step = si - 1;
        size_t t = find_bag_index(&nbrs[step], T);
        unsigned int s = tree_dec_add_vertex(T);
        if (t != (size_t)-1) cg_add_edge(&T->g, (unsigned int)t, s, 0.0f);
        /* Bag of s = neighbours ∪ {v}. */
        uiset_copy(&T->bag[s], &nbrs[step]);
        uiset_insert(&T->bag[s], vs[step]);
    }

    for (size_t step = 0; step < n_order; step++) uiset_free(&nbrs[step]);
    free(nbrs);
    free(vs);
}

/* ---------- tree_decomposition_from_elimination_ordering ---------- */
static void tree_decomposition_from_elimination_ordering(tree_dec_t *T,
                                                         const ui_list_t *l,
                                                         const cgraph_t *G) {
    /* Build an undirected symmetrization of G. */
    cgraph_t G_sym;
    cg_init(&G_sym, CG_UNDIRECTED, 0);
    cg_copy_topology(&G_sym, G, CG_UNDIRECTED);

    size_t n = cg_num_vertices(G);
    char *active = (char *)calloc(n, 1);
    for (size_t i = 0; i < n; i++) active[i] = 1;

    /* The original iterates the list in REVERSE. */
    unsigned int *order = (unsigned int *)malloc(l->n * sizeof(unsigned int));
    for (size_t i = 0; i < l->n; i++) order[i] = l->items[l->n - 1 - i];

    add_vertices_to_tree_decomposition(T, order, l->n, &G_sym, active);

    free(order);
    free(active);
    cg_free(&G_sym);
}

/* ---------- thorup_tree_decomposition ---------- */

void tree_dec_thorup(tree_dec_t *out, const cgraph_t *cfg) {
    ui_list_t order;
    ui_list_init(&order);
    thorup_elimination_ordering(&order, cfg);
    tree_decomposition_from_elimination_ordering(out, &order, cfg);
    ui_list_free(&order);
}

/* ---------- find_root ---------- */
unsigned int tree_dec_find_root(const tree_dec_t *t) {
    unsigned int v = 0;
    /* Walk up via in-edges until we find a source. */
    while (t->g.in[v].n > 0) {
        v = t->g.in[v].dst[0];
    }
    return v;
}

/* ---------- nicify_joins ---------- *
 * Each join node (two children) must share the bag of its children. Each
 * single-child or leaf node is recursed into. If a node has >2 children,
 * we introduce a shim node between it and two of its children and recurse.
 *
 * The recursion is at most O(nvertices) deep in the worst case — matches
 * the original which is also recursive. */
static void nicify_joins(tree_dec_t *T, unsigned int t) {
    for (;;) {
        size_t od = cg_out_degree(&T->g, t);
        if (od == 0) return;
        if (od == 1) {
            unsigned int c = T->g.out[t].dst[0];
            t = c;
            continue;
        }
        if (od == 2) {
            unsigned int c0 = T->g.out[t].dst[0];
            unsigned int c1 = T->g.out[t].dst[1];
            nicify_joins(T, c0);
            if (!uiset_equal(&T->bag[t], &T->bag[c0])) {
                unsigned int d = tree_dec_add_vertex(T);
                uiset_copy(&T->bag[d], &T->bag[t]);
                /* Re-resolve c0 and c1 — add_vertex may have grown arrays
                 * but cg edges haven't moved. We stored them before. */
                cg_add_edge(&T->g, d, c0, 0.0f);
                cg_remove_edge(&T->g, t, c0);
                cg_add_edge(&T->g, t, d, 0.0f);
            }
            nicify_joins(T, c1);
            if (!uiset_equal(&T->bag[t], &T->bag[c1])) {
                unsigned int d = tree_dec_add_vertex(T);
                uiset_copy(&T->bag[d], &T->bag[t]);
                cg_add_edge(&T->g, d, c1, 0.0f);
                cg_remove_edge(&T->g, t, c1);
                cg_add_edge(&T->g, t, d, 0.0f);
            }
            return;
        }
        /* od >= 3: introduce a shim combining two children, then retry. */
        unsigned int c0 = T->g.out[t].dst[0];
        unsigned int c1 = T->g.out[t].dst[1];
        unsigned int d = tree_dec_add_vertex(T);
        uiset_copy(&T->bag[d], &T->bag[t]);
        cg_add_edge(&T->g, d, c0, 0.0f);
        cg_add_edge(&T->g, d, c1, 0.0f);
        cg_remove_edge(&T->g, t, c0);
        cg_remove_edge(&T->g, t, c1);
        cg_add_edge(&T->g, t, d, 0.0f);
        /* loop — same t */
    }
}

/* ---------- nicify_diffs ---------- *
 * Each non-leaf node with a single child must either contain the child's
 * bag or be contained in it. If not, insert a shim with the intersection. */
static void nicify_diffs(tree_dec_t *T, unsigned int t) {
    size_t od = cg_out_degree(&T->g, t);
    if (od == 0) {
        if (T->bag[t].n > 0) {
            unsigned int d = tree_dec_add_vertex(T);
            cg_add_edge(&T->g, t, d, 0.0f);
            /* bag[d] is empty already */
        }
        return;
    }
    if (od == 2) {
        unsigned int c0 = T->g.out[t].dst[0];
        unsigned int c1 = T->g.out[t].dst[1];
        nicify_diffs(T, c0);
        nicify_diffs(T, c1);
        return;
    }
    if (od != 1) {
        fprintf(stderr, "nicify_diffs error.\n");
        return;
    }

    unsigned int c0 = T->g.out[t].dst[0];
    nicify_diffs(T, c0);

    if (uiset_includes(&T->bag[t], &T->bag[c0]) ||
        uiset_includes(&T->bag[c0], &T->bag[t]))
        return;

    unsigned int d = tree_dec_add_vertex(T);
    /* Edges first — we already have c0 captured. */
    cg_add_edge(&T->g, d, c0, 0.0f);
    cg_remove_edge(&T->g, t, c0);
    uiset_intersection(&T->bag[t], &T->bag[c0], &T->bag[d]);
    cg_add_edge(&T->g, t, d, 0.0f);
}

/* ---------- nicify_diffs_more ---------- *
 * Ensure consecutive bags differ by at most one element. Also assigns
 * weight[v] for each node. */
static void nicify_diffs_more(tree_dec_t *T, unsigned int t) {
    for (;;) {
        size_t od = cg_out_degree(&T->g, t);
        if (od == 0) {
            if (T->bag[t].n > 1) {
                unsigned int d = tree_dec_add_vertex(T);
                uiset_copy(&T->bag[d], &T->bag[t]);
                /* erase smallest (equivalent to erasing begin()). */
                memmove(&T->bag[d].items[0], &T->bag[d].items[1],
                        (T->bag[d].n - 1) * sizeof(unsigned int));
                T->bag[d].n--;
                T->weight[d] = 0;
                cg_add_edge(&T->g, t, d, 0.0f);
                /* retry with same t */
                continue;
            }
            T->weight[t] = 0;
            return;
        }
        if (od == 2) {
            unsigned int c0 = T->g.out[t].dst[0];
            unsigned int c1 = T->g.out[t].dst[1];
            nicify_diffs_more(T, c0);
            nicify_diffs_more(T, c1);
            unsigned wmin = T->weight[c0] < T->weight[c1] ? T->weight[c0] : T->weight[c1];
            T->weight[t] = wmin + 1;
            return;
        }
        if (od != 1) {
            fprintf(stderr, "nicify_diffs_more error.\n");
            return;
        }

        unsigned int c0 = T->g.out[t].dst[0];
        size_t ts = T->bag[t].n;
        size_t c0s = T->bag[c0].n;

        if (ts <= c0s + 1 && ts + 1 >= c0s) {
            nicify_diffs_more(T, c0);
            T->weight[t] = T->weight[c0];
            return;
        }

        unsigned int d = tree_dec_add_vertex(T);
        cg_add_edge(&T->g, d, c0, 0.0f);
        cg_remove_edge(&T->g, t, c0);

        /* Copy larger bag into d, then erase the first element not in the
         * smaller bag. */
        const uiset_t *bigger  = ts > c0s ? &T->bag[t] : &T->bag[c0];
        const uiset_t *smaller = ts < c0s ? &T->bag[t] : &T->bag[c0];
        uiset_copy(&T->bag[d], bigger);

        size_t i;
        for (i = 0; i < T->bag[d].n; i++) {
            if (!uiset_contains(smaller, T->bag[d].items[i])) break;
        }
        /* erase element at index i */
        if (i < T->bag[d].n) {
            memmove(&T->bag[d].items[i], &T->bag[d].items[i + 1],
                    (T->bag[d].n - i - 1) * sizeof(unsigned int));
            T->bag[d].n--;
        }
        cg_add_edge(&T->g, t, d, 0.0f);
        /* retry with same t */
    }
}

/* ---------- nicify ---------- */
void tree_dec_nicify(tree_dec_t *T) {
    if (T->g.nvertices == 0) return;
    unsigned int t = tree_dec_find_root(T);

    if (T->bag[t].n > 0) {
        unsigned int d = t;
        t = tree_dec_add_vertex(T);
        cg_add_edge(&T->g, t, d, 0.0f);
    }

    nicify_joins(T, t);
    nicify_diffs(T, t);
    nicify_diffs_more(T, t);
}
