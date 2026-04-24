/* SDCCnaddr.c — pure-C port of SDCCnaddr.cc + SDCCnaddr.hpp.
 *
 * Optimal placement of bank switching instructions for named address spaces.
 *
 * See:
 *   Philipp Klaus Krause, "Optimal Placement of Bank Selection Instructions in
 *   Polynomial Time", M-SCOPES '13, pp. 23-30.
 *
 * This file reimplements the algorithm using our in-tree C graph library
 * (util/cgraph.h), the sorted-array sets (util/uiset.h), and the C tree
 * decomposition (SDCCtree_dec.h). The public entry point is
 * switchAddressSpacesOptimally(), declared in SDCCopt.h.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "SDCCopt.h"

#include "SDCCtree_dec.h"
#include "util/cgraph.h"
#include "util/uiset.h"

/* Named address spaces. -1 means "undefined". Others index into the addrspaces
 * map built by annotate_cfg_naddr(). */
typedef short naddrspace_t;

/* ---------- CFG (control flow graph) ---------- */

typedef struct {
    iCode   *ic;
    usset_t  possible_naddrspaces; /* set<unsigned short> of sign-encoded naddrspace_t */
} cfg_naddr_node_t;

typedef struct {
    cgraph_t          g;      /* CG_BIDIRECTIONAL, has_weights=1 */
    cfg_naddr_node_t *node;
    size_t            cap;
} cfg_naddr_t;

static void cfg_init(cfg_naddr_t *c) {
    cg_init(&c->g, CG_BIDIRECTIONAL, 1);
    c->node = NULL;
    c->cap = 0;
}

static void cfg_free(cfg_naddr_t *c) {
    size_t v;
    for (v = 0; v < c->g.nvertices; v++)
        usset_free(&c->node[v].possible_naddrspaces);
    free(c->node);
    c->node = NULL;
    c->cap = 0;
    cg_free(&c->g);
}

static unsigned int cfg_add_vertex(cfg_naddr_t *c) {
    if (c->g.nvertices >= c->cap) {
        size_t nc = c->cap ? c->cap * 2 : 16;
        c->node = (cfg_naddr_node_t *)realloc(c->node, nc * sizeof(*c->node));
        c->cap = nc;
    }
    unsigned int v = cg_add_vertex(&c->g);
    c->node[v].ic = NULL;
    usset_init(&c->node[v].possible_naddrspaces);
    return v;
}

/* ---------- Key -> vertex-index map (linear for simplicity; CFG is small) ---------- */

typedef struct {
    int         *keys;
    unsigned int *idxs;
    size_t       n, cap;
} key_map_t;

static void km_init(key_map_t *m) {
    m->keys = NULL; m->idxs = NULL; m->n = 0; m->cap = 0;
}

static void km_free(key_map_t *m) {
    free(m->keys); free(m->idxs);
    m->keys = NULL; m->idxs = NULL; m->n = 0; m->cap = 0;
}

static void km_put(key_map_t *m, int key, unsigned int idx) {
    if (m->n == m->cap) {
        size_t nc = m->cap ? m->cap * 2 : 32;
        m->keys = (int *)realloc(m->keys, nc * sizeof(int));
        m->idxs = (unsigned int *)realloc(m->idxs, nc * sizeof(unsigned int));
        m->cap = nc;
    }
    m->keys[m->n] = key;
    m->idxs[m->n] = idx;
    m->n++;
}

static unsigned int km_get(const key_map_t *m, int key) {
    size_t i;
    for (i = 0; i < m->n; i++)
        if (m->keys[i] == key) return m->idxs[i];
    return (unsigned int)-1; /* shouldn't happen on well-formed CFGs */
}

/* ---------- addrspaces map: naddrspace_t -> const symbol * ---------- */

typedef struct {
    naddrspace_t      *keys;
    const symbol    **vals;
    size_t             n, cap;
} ns_map_t;

static void ns_init(ns_map_t *m) {
    m->keys = NULL; m->vals = NULL; m->n = 0; m->cap = 0;
}

static void ns_free(ns_map_t *m) {
    free(m->keys); free(m->vals);
    m->keys = NULL; m->vals = NULL; m->n = 0; m->cap = 0;
}

static void ns_put(ns_map_t *m, naddrspace_t k, const symbol *v) {
    size_t i;
    for (i = 0; i < m->n; i++) {
        if (m->keys[i] == k) { m->vals[i] = v; return; }
    }
    if (m->n == m->cap) {
        size_t nc = m->cap ? m->cap * 2 : 8;
        m->keys = (naddrspace_t *)realloc(m->keys, nc * sizeof(naddrspace_t));
        m->vals = (const symbol **)realloc(m->vals, nc * sizeof(const symbol *));
        m->cap = nc;
    }
    m->keys[m->n] = k;
    m->vals[m->n] = v;
    m->n++;
}

static const symbol *ns_get(const ns_map_t *m, naddrspace_t k) {
    size_t i;
    for (i = 0; i < m->n; i++)
        if (m->keys[i] == k) return m->vals[i];
    return NULL;
}

/* ---------- symbol -> naddrspace_t map for annotate_cfg_naddr ---------- */

typedef struct {
    const symbol **syms;
    naddrspace_t  *nas;
    size_t         n, cap;
} sym_idx_map_t;

static void sim_init(sym_idx_map_t *m) {
    m->syms = NULL; m->nas = NULL; m->n = 0; m->cap = 0;
}

static void sim_free(sym_idx_map_t *m) {
    free(m->syms); free(m->nas);
    m->syms = NULL; m->nas = NULL; m->n = 0; m->cap = 0;
}

/* Returns existing index or inserts with new value and returns it. */
static int sim_find(const sym_idx_map_t *m, const symbol *sym, naddrspace_t *out) {
    size_t i;
    for (i = 0; i < m->n; i++) {
        if (m->syms[i] == sym) { *out = m->nas[i]; return 1; }
    }
    return 0;
}

static void sim_put(sym_idx_map_t *m, const symbol *sym, naddrspace_t na) {
    if (m->n == m->cap) {
        size_t nc = m->cap ? m->cap * 2 : 8;
        m->syms = (const symbol **)realloc(m->syms, nc * sizeof(const symbol *));
        m->nas  = (naddrspace_t *)realloc(m->nas, nc * sizeof(naddrspace_t));
        m->cap = nc;
    }
    m->syms[m->n] = sym;
    m->nas[m->n] = na;
    m->n++;
}

/* ---------- naddrspace <-> unsigned short encoding ----------
 *
 * The C++ version stored naddrspace_t (signed short) values in a
 * std::set<unsigned short>. We preserve that by bit-reinterpreting. On
 * two's-complement systems (which the rest of SDCC already assumes) the
 * cast is a no-op: (unsigned short)(short)-1 == 0xFFFF, and back again. */
static inline unsigned short ns_enc(naddrspace_t na) {
    return (unsigned short)na;
}
static inline naddrspace_t ns_dec(unsigned short v) {
    return (naddrspace_t)v;
}

/* ---------- assignment_naddr (linked list node) ---------- */

typedef struct assignment_naddr {
    float                      s;
    usset_t                    local;    /* set of unsigned short (vertex indices) */
    naddrspace_t              *global;   /* length global_n; -2 init, -1 none */
    size_t                     global_n;
    struct assignment_naddr   *prev;
    struct assignment_naddr   *next;
} assignment_naddr_t;

typedef struct {
    assignment_naddr_t *head;
    assignment_naddr_t *tail;
    size_t              n;
} assignment_list_naddr_t;

static void al_init(assignment_list_naddr_t *l) {
    l->head = l->tail = NULL;
    l->n = 0;
}

static assignment_naddr_t *a_clone(const assignment_naddr_t *src) {
    assignment_naddr_t *a = (assignment_naddr_t *)malloc(sizeof(*a));
    a->s = src->s;
    usset_init(&a->local);
    usset_copy(&a->local, &src->local);
    a->global_n = src->global_n;
    a->global = (naddrspace_t *)malloc(a->global_n * sizeof(naddrspace_t));
    memcpy(a->global, src->global, a->global_n * sizeof(naddrspace_t));
    a->prev = a->next = NULL;
    return a;
}

static void a_free(assignment_naddr_t *a) {
    if (!a) return;
    usset_free(&a->local);
    free(a->global);
    free(a);
}

static void al_clear(assignment_list_naddr_t *l) {
    assignment_naddr_t *p = l->head;
    while (p) {
        assignment_naddr_t *nx = p->next;
        a_free(p);
        p = nx;
    }
    l->head = l->tail = NULL;
    l->n = 0;
}

static void al_push_back_take(assignment_list_naddr_t *l, assignment_naddr_t *a) {
    a->prev = l->tail;
    a->next = NULL;
    if (l->tail) l->tail->next = a; else l->head = a;
    l->tail = a;
    l->n++;
}

/* Detach node `a` from list `l` and free it. Returns the successor. */
static assignment_naddr_t *al_erase(assignment_list_naddr_t *l, assignment_naddr_t *a) {
    assignment_naddr_t *nx = a->next;
    if (a->prev) a->prev->next = a->next; else l->head = a->next;
    if (a->next) a->next->prev = a->prev; else l->tail = a->prev;
    l->n--;
    a_free(a);
    return nx;
}

static void al_swap(assignment_list_naddr_t *a, assignment_list_naddr_t *b) {
    assignment_list_naddr_t t = *a;
    *a = *b;
    *b = t;
}

/* ---------- assignment ordering ----------
 *
 * Original C++ operator< walks locals lexicographically (unsigned short order),
 * with ties broken by the corresponding global entries. */
static int assignment_compare(const assignment_naddr_t *x, const assignment_naddr_t *y) {
    size_t i, j, ni = x->local.n, nj = y->local.n;
    for (i = 0, j = 0;; i++, j++) {
        if (i == ni && j == nj) return 0;
        if (i == ni) return -1; /* x ran out first => x < y */
        if (j == nj) return 1;
        unsigned short xi = x->local.items[i];
        unsigned short yj = y->local.items[j];
        if (xi < yj) return -1;
        if (xi > yj) return 1;
        /* Equal local var. Compare globals at that index. */
        naddrspace_t gx = x->global[xi];
        naddrspace_t gy = y->global[yj];
        if (gx < gy) return -1;
        if (gx > gy) return 1;
    }
}

static int assignments_naddr_locally_same(const assignment_naddr_t *a,
                                          const assignment_naddr_t *b) {
    size_t i;
    if (!usset_equal(&a->local, &b->local)) return 0;
    for (i = 0; i < a->local.n; i++) {
        unsigned short v = a->local.items[i];
        if (a->global[v] != b->global[v]) return 0;
    }
    return 1;
}

/* ---------- Merge sort the doubly-linked assignment list ---------- */

static assignment_naddr_t *al_merge(assignment_naddr_t *a, assignment_naddr_t *b) {
    assignment_naddr_t head;
    assignment_naddr_t *tail = &head;
    head.next = NULL;
    while (a && b) {
        if (assignment_compare(a, b) <= 0) {
            tail->next = a; a->prev = tail; a = a->next;
        } else {
            tail->next = b; b->prev = tail; b = b->next;
        }
        tail = tail->next;
    }
    if (a) { tail->next = a; a->prev = tail; }
    else   { tail->next = b; if (b) b->prev = tail; }
    if (head.next) head.next->prev = NULL;
    return head.next;
}

static assignment_naddr_t *al_sort_nodes(assignment_naddr_t *head) {
    if (!head || !head->next) return head;
    /* split using slow/fast */
    assignment_naddr_t *slow = head;
    assignment_naddr_t *fast = head->next;
    while (fast && fast->next) {
        slow = slow->next;
        fast = fast->next->next;
    }
    assignment_naddr_t *mid = slow->next;
    slow->next = NULL;
    if (mid) mid->prev = NULL;
    assignment_naddr_t *l = al_sort_nodes(head);
    assignment_naddr_t *r = al_sort_nodes(mid);
    return al_merge(l, r);
}

static void al_sort(assignment_list_naddr_t *l) {
    l->head = al_sort_nodes(l->head);
    /* Fix tail. */
    assignment_naddr_t *p = l->head, *prev = NULL;
    while (p) { prev = p; p = p->next; }
    l->tail = prev;
}

/* ---------- tree_dec_naddr wrapper (parallel assignment lists) ---------- */

typedef struct {
    tree_dec_t               td;
    assignment_list_naddr_t *alist;
    size_t                   cap;
} tree_dec_naddr_t;

static void tdn_init(tree_dec_naddr_t *t) {
    tree_dec_init(&t->td);
    t->alist = NULL;
    t->cap = 0;
}

static void tdn_sync_arrays(tree_dec_naddr_t *t) {
    size_t need = t->td.g.nvertices;
    if (need <= t->cap) return;
    size_t nc = t->cap ? t->cap * 2 : 8;
    while (nc < need) nc *= 2;
    t->alist = (assignment_list_naddr_t *)realloc(t->alist, nc * sizeof(*t->alist));
    for (size_t i = t->cap; i < nc; i++) al_init(&t->alist[i]);
    t->cap = nc;
}

static void tdn_free(tree_dec_naddr_t *t) {
    size_t i;
    for (i = 0; i < t->td.g.nvertices; i++)
        al_clear(&t->alist[i]);
    free(t->alist);
    t->alist = NULL;
    t->cap = 0;
    tree_dec_free(&t->td);
}

/* ---------- annotate_cfg_naddr ---------- */

static void annotate_cfg_naddr(cfg_naddr_t *cfg, ns_map_t *addrspaces) {
    sym_idx_map_t sym_to_index;
    naddrspace_t na_max = -1;
    size_t nv = cfg->g.nvertices;
    char *predetermined = (char *)calloc(nv ? nv : 1, 1);
    unsigned int i;

    sim_init(&sym_to_index);

    for (i = 0; i < nv; i++) {
        const iCode *ic = cfg->node[i].ic;
        const symbol *addrspace;

        if (ic->op == CALL || ic->op == PCALL || ic->op == FUNCTION)
            predetermined[i] = 1;

        addrspace = getAddrspaceiCode(ic);
        if (addrspace) {
            naddrspace_t na;
            if (!sim_find(&sym_to_index, addrspace, &na)) {
                na = ++na_max;
                sim_put(&sym_to_index, addrspace, na);
            }
            ns_put(addrspaces, na, addrspace);
            usset_insert(&cfg->node[i].possible_naddrspaces, ns_enc(na));
            predetermined[i] = 1;
        } else {
            usset_insert(&cfg->node[i].possible_naddrspaces, ns_enc((naddrspace_t)-1));
        }
    }

    int change;
    do {
        change = 0;
        for (i = 0; i < nv; i++) {
            if (predetermined[i]) continue;

            size_t oldsize = cfg->node[i].possible_naddrspaces.n;

            size_t k;
            unsigned int nbr;
            float wt;
            CG_FOREACH_OUT(&cfg->g, i, k, nbr, wt) {
                (void)wt;
                usset_union_into(&cfg->node[i].possible_naddrspaces,
                                 &cfg->node[nbr].possible_naddrspaces);
            }
            CG_FOREACH_IN(&cfg->g, i, k, nbr, wt) {
                (void)wt;
                usset_union_into(&cfg->node[i].possible_naddrspaces,
                                 &cfg->node[nbr].possible_naddrspaces);
            }
            if (oldsize != cfg->node[i].possible_naddrspaces.n)
                change = 1;
        }
    } while (change);

    free(predetermined);
    sim_free(&sym_to_index);
}

/* ---------- create_cfg_naddr ---------- */

static void create_cfg_naddr(cfg_naddr_t *cfg, iCode *start_ic, ebbIndex *ebbi) {
    key_map_t key_to_index;
    iCode *ic;
    unsigned int i = 0;

    km_init(&key_to_index);

    for (ic = start_ic; ic; ic = ic->next, i++) {
        unsigned int v = cfg_add_vertex(cfg);
        cfg->node[v].ic = ic;
        km_put(&key_to_index, ic->key, v);
    }

    for (ic = start_ic; ic; ic = ic->next) {
        unsigned int u = km_get(&key_to_index, ic->key);

        if (ic->op != GOTO && ic->op != RETURN && ic->op != JUMPTABLE && ic->next)
            cg_add_edge(&cfg->g, u,
                        km_get(&key_to_index, ic->next->key), 3.0f);

        if (ic->op == GOTO) {
            cg_add_edge(&cfg->g, u,
                        km_get(&key_to_index,
                               eBBWithEntryLabel(ebbi, ic->label)->sch->key),
                        6.0f);
        } else if (ic->op == RETURN) {
            cg_add_edge(&cfg->g, u,
                        km_get(&key_to_index,
                               eBBWithEntryLabel(ebbi, returnLabel)->sch->key),
                        6.0f);
        } else if (ic->op == IFX) {
            symbol *target = IC_TRUE(ic) ? IC_TRUE(ic) : IC_FALSE(ic);
            cg_add_edge(&cfg->g, u,
                        km_get(&key_to_index,
                               eBBWithEntryLabel(ebbi, target)->sch->key),
                        6.0f);
        } else if (ic->op == JUMPTABLE) {
            symbol *lbl;
            for (lbl = (symbol *)setFirstItem(IC_JTLABELS(ic)); lbl;
                 lbl = (symbol *)setNextItem(IC_JTLABELS(ic))) {
                cg_add_edge(&cfg->g, u,
                            km_get(&key_to_index,
                                   eBBWithEntryLabel(ebbi, lbl)->sch->key),
                            6.0f);
            }
        }
    }

    km_free(&key_to_index);
}

/* ---------- dump_cfg_naddr (plain GraphViz) ---------- */

static void dump_cfg_naddr(const cfg_naddr_t *cfg) {
    if (!dstFileName) return;
    const char *suffix = ".dumpnaddrcfg";
    const char *fname = currFunc ? currFunc->rname : "__global";
    size_t len = strlen(dstFileName) + strlen(suffix) + strlen(fname) + 5;
    char *path = (char *)malloc(len);
    snprintf(path, len, "%s%s%s.dot", dstFileName, suffix, fname);
    FILE *f = fopen(path, "w");
    free(path);
    if (!f) return;

    fprintf(f, "digraph G {\n");
    size_t v;
    for (v = 0; v < cfg->g.nvertices; v++) {
        fprintf(f, "  %zu [label=\"%zu, %d: ", v, v,
                cfg->node[v].ic ? cfg->node[v].ic->key : -1);
        size_t i;
        for (i = 0; i < cfg->node[v].possible_naddrspaces.n; i++) {
            fprintf(f, "%d ",
                    (int)ns_dec(cfg->node[v].possible_naddrspaces.items[i]));
        }
        fprintf(f, "\"];\n");
    }
    for (v = 0; v < cfg->g.nvertices; v++) {
        size_t i;
        unsigned int nbr;
        float wt;
        CG_FOREACH_OUT(&cfg->g, v, i, nbr, wt) {
            fprintf(f, "  %zu -> %u [label=\"%g\"];\n", v, nbr, wt);
        }
    }
    fprintf(f, "}\n");
    fclose(f);
}

/* ---------- Tree DP: leaf/introduce/forget/join ---------- */

static void tree_dec_naddrswitch_leaf(tree_dec_naddr_t *T, unsigned int t,
                                      const cfg_naddr_t *G) {
    assignment_naddr_t *a = (assignment_naddr_t *)malloc(sizeof(*a));
    a->s = 0.0f;
    usset_init(&a->local);
    a->global_n = G->g.nvertices;
    a->global = (naddrspace_t *)malloc((a->global_n ? a->global_n : 1) *
                                       sizeof(naddrspace_t));
    for (size_t i = 0; i < a->global_n; i++) a->global[i] = -2;
    a->prev = a->next = NULL;
    al_push_back_take(&T->alist[t], a);
}

/* Find the (single) child via cg out_edges of the tree (our tree uses
 * bidirectional edges; children are out-neighbors from parent). */
static unsigned int tree_child(const tree_dec_t *td, unsigned int t, int which) {
    size_t i;
    unsigned int nbr;
    float wt;
    int seen = 0;
    CG_FOREACH_OUT(&td->g, t, i, nbr, wt) {
        (void)wt;
        if (seen == which) return nbr;
        seen++;
    }
    return (unsigned int)-1;
}

static int tree_dec_naddrswitch_introduce(tree_dec_naddr_t *T, unsigned int t,
                                          const cfg_naddr_t *G) {
    unsigned int c = tree_child(&T->td, t, 0);
    assignment_list_naddr_t *alist2 = &T->alist[t];
    assignment_list_naddr_t *alist  = &T->alist[c];

    /* new_inst = bag[t] \ bag[c]. One element. */
    uiset_t new_inst;
    uiset_init(&new_inst);
    uiset_difference(&T->td.bag[t], &T->td.bag[c], &new_inst);
    unsigned short i_var = (unsigned short)new_inst.items[0];
    uiset_free(&new_inst);

    const usset_t *poss = &G->node[i_var].possible_naddrspaces;

    assignment_naddr_t *p = alist->head;
    while (p) {
        assignment_naddr_t *next = p->next;

        /* Detach p from alist first (we'll either repurpose it or free it). */
        if (p->prev) p->prev->next = p->next; else alist->head = p->next;
        if (p->next) p->next->prev = p->prev; else alist->tail = p->prev;
        alist->n--;
        p->prev = p->next = NULL;

        usset_insert(&p->local, i_var);

        /* For each possible naddrspace, push a copy of (modified) p into
         * alist2, with global[i_var] set. The last one reuses p itself. */
        size_t np = poss->n;
        if (np == 0) {
            a_free(p);
        } else {
            for (size_t k = 0; k < np - 1; k++) {
                assignment_naddr_t *copy = a_clone(p);
                copy->global[i_var] = ns_dec(poss->items[k]);
                al_push_back_take(alist2, copy);
            }
            p->global[i_var] = ns_dec(poss->items[np - 1]);
            al_push_back_take(alist2, p);
        }

        p = next;
    }

    /* alist (child) is now empty. */
    return (int)alist2->n <= options.max_allocs_per_node ? 0 : -1;
}

static void tree_dec_naddrswitch_forget(tree_dec_naddr_t *T, unsigned int t,
                                        const cfg_naddr_t *G) {
    unsigned int c = tree_child(&T->td, t, 0);
    assignment_list_naddr_t *alist = &T->alist[t];

    /* Move child assignments into t's list. */
    al_swap(alist, &T->alist[c]);

    /* old_inst = bag[c] \ bag[t]. One element. */
    uiset_t old_inst;
    uiset_init(&old_inst);
    uiset_difference(&T->td.bag[c], &T->td.bag[t], &old_inst);
    unsigned short i_var = (unsigned short)old_inst.items[0];
    uiset_free(&old_inst);

    /* For each assignment, drop i from local, and accumulate the cost of
     * switching across edges incident to i whose other endpoint is also local. */
    assignment_naddr_t *ai;
    for (ai = alist->head; ai; ai = ai->next) {
        usset_erase(&ai->local, i_var);

        size_t k;
        unsigned int nbr;
        float wt;
        /* out-edges: source=i_var, target=nbr */
        CG_FOREACH_OUT(&G->g, i_var, k, nbr, wt) {
            if (!usset_contains(&ai->local, (unsigned short)nbr) ||
                ai->global[nbr] == -1)
                continue;
            if (ai->global[i_var] == ai->global[nbr])
                continue;
            ai->s += wt;
        }
        /* in-edges: source=nbr, target=i_var */
        CG_FOREACH_IN(&G->g, i_var, k, nbr, wt) {
            if (!usset_contains(&ai->local, (unsigned short)nbr) ||
                ai->global[i_var] == -1)
                continue;
            if (ai->global[nbr] == ai->global[i_var])
                continue;
            ai->s += wt;
        }
    }

    al_sort(alist);

    /* Collapse locally-identical runs, keeping the lower-cost one. */
    assignment_naddr_t *cur = alist->head;
    while (cur) {
        assignment_naddr_t *aif = cur;
        assignment_naddr_t *nx = cur->next;
        while (nx && assignments_naddr_locally_same(aif, nx)) {
            if (aif->s > nx->s) {
                al_erase(alist, aif);
                aif = nx;
                nx = nx->next;
            } else {
                nx = al_erase(alist, nx);
            }
        }
        cur = aif->next;
    }
}

static void tree_dec_naddrswitch_join(tree_dec_naddr_t *T, unsigned int t,
                                      const cfg_naddr_t *G) {
    (void)G;
    unsigned int c2 = tree_child(&T->td, t, 0);
    unsigned int c3 = tree_child(&T->td, t, 1);

    assignment_list_naddr_t *alist1 = &T->alist[t];
    assignment_list_naddr_t *alist2 = &T->alist[c2];
    assignment_list_naddr_t *alist3 = &T->alist[c3];

    al_sort(alist2);
    al_sort(alist3);

    assignment_naddr_t *ai2 = alist2->head;
    assignment_naddr_t *ai3 = alist3->head;

    while (ai2 && ai3) {
        if (assignments_naddr_locally_same(ai2, ai3)) {
            ai2->s += ai3->s;
            for (size_t i = 0; i < ai2->global_n; i++) {
                if (ai2->global[i] == -2)
                    ai2->global[i] = ai3->global[i];
            }
            /* Take a copy into alist1 (since we still own ai2 in alist2
             * and will free it below). */
            assignment_naddr_t *copy = a_clone(ai2);
            al_push_back_take(alist1, copy);
            ai2 = ai2->next;
            ai3 = ai3->next;
        } else {
            int cmp = assignment_compare(ai2, ai3);
            if (cmp < 0) ai2 = ai2->next;
            else if (cmp > 0) ai3 = ai3->next;
            else {
                /* Same ordering but not locally same — advance both to match
                 * the C++ behavior of the loop body (which would just keep
                 * looping). In practice this branch should rarely trigger. */
                ai2 = ai2->next;
                ai3 = ai3->next;
            }
        }
    }

    al_clear(alist2);
    al_clear(alist3);
}

static int tree_dec_naddrswitch_nodes(tree_dec_naddr_t *T, unsigned int t,
                                      const cfg_naddr_t *G) {
    size_t deg = cg_out_degree(&T->td.g, t);
    unsigned int c0, c1;

    switch (deg) {
    case 0:
        tree_dec_naddrswitch_leaf(T, t, G);
        break;
    case 1:
        c0 = tree_child(&T->td, t, 0);
        if (tree_dec_naddrswitch_nodes(T, c0, G)) return -1;
        if (T->td.bag[c0].n < T->td.bag[t].n) {
            if (tree_dec_naddrswitch_introduce(T, t, G)) return -1;
        } else {
            tree_dec_naddrswitch_forget(T, t, G);
        }
        break;
    case 2:
        c0 = tree_child(&T->td, t, 0);
        c1 = tree_child(&T->td, t, 1);
        if (T->td.weight[c0] < T->td.weight[c1]) {
            unsigned int tmp = c0; c0 = c1; c1 = tmp;
        }
        if (tree_dec_naddrswitch_nodes(T, c0, G)) return -1;
        if (tree_dec_naddrswitch_nodes(T, c1, G)) return -1;
        tree_dec_naddrswitch_join(T, t, G);
        break;
    default:
        fprintf(stderr, "Not nice.\n");
        break;
    }
    return 0;
}

static void implement_naddr_assignment(const assignment_naddr_t *a,
                                       const cfg_naddr_t *G,
                                       const ns_map_t *addrspaces) {
    size_t src;
    for (src = 0; src < G->g.nvertices; src++) {
        size_t k;
        unsigned int tgt;
        float wt;
        CG_FOREACH_OUT(&G->g, src, k, tgt, wt) {
            (void)wt;
            naddrspace_t sourcespace = a->global[src];
            naddrspace_t targetspace = a->global[tgt];

            if (targetspace == -1 || sourcespace == targetspace)
                continue;

            if (G->node[src].ic->next != G->node[tgt].ic)
                fprintf(stderr, "Trying to switch address space at weird edge in CFG.");

            switchAddressSpaceAt(G->node[tgt].ic, ns_get(addrspaces, targetspace));
        }
    }
}

static int tree_dec_address_switch(tree_dec_naddr_t *T, const cfg_naddr_t *G,
                                   const ns_map_t *addrspaces) {
    tdn_sync_arrays(T);

    unsigned int root = tree_dec_find_root(&T->td);
    if (tree_dec_naddrswitch_nodes(T, root, G))
        return -1;

    assignment_naddr_t *winner = T->alist[root].head;
    if (!winner) return -1;

    implement_naddr_assignment(winner, G, addrspaces);
    return 0;
}

/* ---------- Public entry point ---------- */

int switchAddressSpacesOptimally(iCode *ic, ebbIndex *ebbi) {
    cfg_naddr_t      cfg;
    tree_dec_naddr_t td;
    ns_map_t         addrspaces;

    cfg_init(&cfg);
    tdn_init(&td);
    ns_init(&addrspaces);

    create_cfg_naddr(&cfg, ic, ebbi);
    annotate_cfg_naddr(&cfg, &addrspaces);

    if (options.dump_graphs)
        dump_cfg_naddr(&cfg);

    tree_dec_thorup(&td.td, &cfg.g);
    tree_dec_nicify(&td.td);
    tdn_sync_arrays(&td);

    int rc = tree_dec_address_switch(&td, &cfg, &addrspaces);

    tdn_free(&td);
    ns_free(&addrspaces);
    cfg_free(&cfg);

    return rc;
}
