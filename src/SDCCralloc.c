/* SDCCralloc.c — pure-C port of SDCCralloc.hpp.
 *
 * An optimal, polynomial-time register allocator (Krause 2013). This file
 * implements the generic dynamic-programming algorithm over a nice tree
 * decomposition. Port-specific cost/feasibility hooks are supplied by the
 * backend (see backend/ralloc2.c).
 */

#include "SDCCralloc.h"

#include <float.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "SDCCbtree.h"

/* Infinity sentinel used pervasively by the algorithm. */
static const float RA_INF = 1.0f / 0.0f;

static int ra_is_inf(float f) { return f == RA_INF || !(f < RA_INF); }

/* -------------------- i_assignment_t -------------------- */

void i_assignment_init(i_assignment_t *ia) {
    reg_t r;
    int i;
    for (r = 0; r < MAX_NUM_REGS; r++)
        for (i = 0; i < 2; i++)
            ia->registers[r][i] = -1;
}

void i_assignment_add_var(i_assignment_t *ia, short v, signed char r) {
    if (ia->registers[r][1] < v) {
        ia->registers[r][0] = ia->registers[r][1];
        ia->registers[r][1] = v;
    } else {
        ia->registers[r][0] = v;
    }
}

void i_assignment_remove_var(i_assignment_t *ia, short v) {
    reg_t r;
    for (r = 0; r < port->num_regs; r++) {
        if (ia->registers[r][1] == v) {
            ia->registers[r][1] = ia->registers[r][0];
            ia->registers[r][0] = -1;
        } else if (ia->registers[r][0] == v) {
            ia->registers[r][0] = -1;
        }
    }
}

/* -------------------- assignment_t -------------------- */

void assignment_init(assignment_t *a) {
    a->s = 0.0f;
    sss_init(&a->local);
    a->global = NULL;
    a->global_n = 0;
    a->i_costs = NULL;
    a->i_costs_n = 0;
    a->i_costs_cap = 0;
    i_assignment_init(&a->i_assignment);
    a->marked = 0;
}

void assignment_free(assignment_t *a) {
    sss_free(&a->local);
    free(a->global);
    a->global = NULL;
    a->global_n = 0;
    free(a->i_costs);
    a->i_costs = NULL;
    a->i_costs_n = a->i_costs_cap = 0;
}

void assignment_copy(assignment_t *dst, const assignment_t *src) {
    assignment_free(dst);
    dst->s = src->s;
    sss_init(&dst->local);
    sss_copy(&dst->local, &src->local);
    dst->global_n = src->global_n;
    if (src->global_n) {
        dst->global = (signed char *)malloc(src->global_n * sizeof(signed char));
        memcpy(dst->global, src->global, src->global_n * sizeof(signed char));
    } else {
        dst->global = NULL;
    }
    dst->i_costs_n = src->i_costs_n;
    dst->i_costs_cap = src->i_costs_n;
    if (src->i_costs_n) {
        dst->i_costs = (icost_entry_t *)malloc(src->i_costs_n * sizeof(icost_entry_t));
        memcpy(dst->i_costs, src->i_costs, src->i_costs_n * sizeof(icost_entry_t));
    } else {
        dst->i_costs = NULL;
    }
    dst->i_assignment = src->i_assignment;
    dst->marked = src->marked;
}

void assignment_move(assignment_t *dst, assignment_t *src) {
    assignment_free(dst);
    *dst = *src;
    /* Zero out src so it can be safely free'd. */
    sss_init(&src->local);
    src->global = NULL;
    src->global_n = 0;
    src->i_costs = NULL;
    src->i_costs_n = src->i_costs_cap = 0;
    i_assignment_init(&src->i_assignment);
    src->marked = 0;
    src->s = 0.0f;
}

/* Binary search for key in i_costs. Returns insertion index; *found=1 if
 * exact match at returned index. */
static size_t icost_lower(const assignment_t *a, int key, int *found) {
    size_t lo = 0, hi = a->i_costs_n;
    while (lo < hi) {
        size_t mid = lo + ((hi - lo) >> 1);
        if (a->i_costs[mid].key < key)
            lo = mid + 1;
        else
            hi = mid;
    }
    *found = (lo < a->i_costs_n && a->i_costs[lo].key == key);
    return lo;
}

void assignment_icost_set(assignment_t *a, int key, float val) {
    int found;
    size_t ix = icost_lower(a, key, &found);
    if (found) {
        a->i_costs[ix].val = val;
        return;
    }
    if (a->i_costs_n == a->i_costs_cap) {
        a->i_costs_cap = a->i_costs_cap ? a->i_costs_cap * 2 : 4;
        a->i_costs = (icost_entry_t *)realloc(a->i_costs,
                                              a->i_costs_cap * sizeof(icost_entry_t));
    }
    memmove(&a->i_costs[ix + 1], &a->i_costs[ix],
            (a->i_costs_n - ix) * sizeof(icost_entry_t));
    a->i_costs[ix].key = key;
    a->i_costs[ix].val = val;
    a->i_costs_n++;
}

int assignment_icost_get(const assignment_t *a, int key, float *out) {
    int found;
    size_t ix = icost_lower(a, key, &found);
    if (!found) {
        /* Default: if key was never set the original std::map would insert 0. */
        if (out) *out = 0.0f;
        return 0;
    }
    if (out) *out = a->i_costs[ix].val;
    return 1;
}

void assignment_icost_erase(assignment_t *a, int key) {
    int found;
    size_t ix = icost_lower(a, key, &found);
    if (!found) return;
    memmove(&a->i_costs[ix], &a->i_costs[ix + 1],
            (a->i_costs_n - ix - 1) * sizeof(icost_entry_t));
    a->i_costs_n--;
}

int assignment_compare(const assignment_t *x, const assignment_t *y) {
    size_t i = 0, j = 0;
    while (1) {
        if (i == x->local.n) return (j == y->local.n) ? 0 : -1;
        if (j == y->local.n) return 1;
        short xi = x->local.items[i];
        short yj = y->local.items[j];
        if (xi < yj) return -1;
        if (xi > yj) return 1;
        signed char gx = x->global[xi];
        signed char gy = y->global[yj];
        if (gx < gy) return -1;
        if (gx > gy) return 1;
        i++;
        j++;
    }
}

/* -------------------- cfg_ralloc_t -------------------- */

void cfg_ralloc_init(cfg_ralloc_t *c) {
    cg_init(&c->g, CG_BIDIRECTIONAL, 0);
    c->node = NULL;
    c->cap = 0;
}

void cfg_ralloc_free(cfg_ralloc_t *c) {
    size_t v;
    for (v = 0; v < c->g.nvertices; v++) {
        free(c->node[v].operands);
        sss_free(&c->node[v].alive);
        sss_free(&c->node[v].dying);
    }
    free(c->node);
    c->node = NULL;
    c->cap = 0;
    cg_free(&c->g);
}

static unsigned int cfg_add_vertex(cfg_ralloc_t *c) {
    if (c->g.nvertices >= c->cap) {
        size_t nc = c->cap ? c->cap * 2 : 16;
        c->node = (cfg_node_t *)realloc(c->node, nc * sizeof(cfg_node_t));
        c->cap = nc;
    }
    unsigned int v = cg_add_vertex(&c->g);
    c->node[v].ic = NULL;
    c->node[v].operands = NULL;
    c->node[v].operands_n = 0;
    c->node[v].operands_cap = 0;
    sss_init(&c->node[v].alive);
    sss_init(&c->node[v].dying);
    return v;
}

size_t cfg_operands_lower_bound(const cfg_node_t *n, int k) {
    size_t lo = 0, hi = n->operands_n;
    while (lo < hi) {
        size_t mid = lo + ((hi - lo) >> 1);
        if (n->operands[mid].key < k)
            lo = mid + 1;
        else
            hi = mid;
    }
    return lo;
}

size_t cfg_operands_equal_range(const cfg_node_t *n, int k, size_t *e_out) {
    size_t s = cfg_operands_lower_bound(n, k);
    size_t e = s;
    while (e < n->operands_n && n->operands[e].key == k) e++;
    *e_out = e;
    return s;
}

/* Insert into operand multimap, preserving sorted-by-key order. Duplicate (k,v)
 * pairs retained — mimics std::multimap::insert behaviour. */
static void cfg_operands_insert(cfg_node_t *n, int k, short v) {
    if (n->operands_n == n->operands_cap) {
        n->operands_cap = n->operands_cap ? n->operands_cap * 2 : 4;
        n->operands = (operand_entry_t *)realloc(
            n->operands, n->operands_cap * sizeof(operand_entry_t));
    }
    size_t ix = cfg_operands_lower_bound(n, k);
    /* Insert after existing entries with the same key (upper_bound). */
    while (ix < n->operands_n && n->operands[ix].key == k) ix++;
    memmove(&n->operands[ix + 1], &n->operands[ix],
            (n->operands_n - ix) * sizeof(operand_entry_t));
    n->operands[ix].key = k;
    n->operands[ix].var = v;
    n->operands_n++;
}

/* Returns 1 if operand multimap has any entry with key k. */
static int cfg_operands_has_key(const cfg_node_t *n, int k) {
    size_t e;
    size_t s = cfg_operands_equal_range(n, k, &e);
    return s < e;
}

/* -------------------- con_t -------------------- */

void con_init(con_t *c) {
    cg_init(&c->g, CG_UNDIRECTED, 0);
    c->node = NULL;
    c->cap = 0;
}

void con_free(con_t *c) {
    free(c->node);
    c->node = NULL;
    c->cap = 0;
    cg_free(&c->g);
}

static unsigned int con_add_vertex(con_t *c) {
    if (c->g.nvertices >= c->cap) {
        size_t nc = c->cap ? c->cap * 2 : 16;
        c->node = (con_node_t *)realloc(c->node, nc * sizeof(con_node_t));
        c->cap = nc;
    }
    unsigned int v = cg_add_vertex(&c->g);
    c->node[v].v = -1;
    c->node[v].byte = 0;
    c->node[v].size = 0;
    c->node[v].name = NULL;
    return v;
}

/* -------------------- tree_dec_ralloc_t -------------------- */

void tree_dec_ralloc_init(tree_dec_ralloc_t *T) {
    tree_dec_init(&T->td);
    T->node = NULL;
    T->cap = 0;
}

static void free_alist(tree_dec_ralloc_node_t *n) {
    assignment_node_t *p = n->alist_head;
    while (p) {
        assignment_node_t *nx = p->next;
        assignment_free(&p->a);
        free(p);
        p = nx;
    }
    n->alist_head = n->alist_tail = NULL;
    n->alist_n = 0;
}

void tree_dec_ralloc_free(tree_dec_ralloc_t *T) {
    size_t v;
    for (v = 0; v < T->td.g.nvertices; v++) {
        sss_free(&T->node[v].alive);
        free_alist(&T->node[v]);
    }
    free(T->node);
    T->node = NULL;
    T->cap = 0;
    tree_dec_free(&T->td);
}

/* -------------------- assignment list helpers -------------------- */

static assignment_node_t *alist_push_back_move(tree_dec_ralloc_node_t *n,
                                               assignment_t *a_to_move) {
    assignment_node_t *node = (assignment_node_t *)malloc(sizeof(assignment_node_t));
    assignment_init(&node->a);
    assignment_move(&node->a, a_to_move);
    node->prev = n->alist_tail;
    node->next = NULL;
    if (n->alist_tail) n->alist_tail->next = node; else n->alist_head = node;
    n->alist_tail = node;
    n->alist_n++;
    return node;
}

static assignment_node_t *alist_push_back_copy(tree_dec_ralloc_node_t *n,
                                               const assignment_t *a) {
    assignment_node_t *node = (assignment_node_t *)malloc(sizeof(assignment_node_t));
    assignment_init(&node->a);
    assignment_copy(&node->a, a);
    node->prev = n->alist_tail;
    node->next = NULL;
    if (n->alist_tail) n->alist_tail->next = node; else n->alist_head = node;
    n->alist_tail = node;
    n->alist_n++;
    return node;
}

static assignment_node_t *alist_erase(tree_dec_ralloc_node_t *n,
                                      assignment_node_t *node) {
    assignment_node_t *nx = node->next;
    if (node->prev) node->prev->next = node->next; else n->alist_head = node->next;
    if (node->next) node->next->prev = node->prev; else n->alist_tail = node->prev;
    assignment_free(&node->a);
    free(node);
    n->alist_n--;
    return nx;
}

static void alist_clear(tree_dec_ralloc_node_t *n) {
    free_alist(n);
}

static void alist_swap(tree_dec_ralloc_node_t *a, tree_dec_ralloc_node_t *b) {
    assignment_node_t *h = a->alist_head, *t = a->alist_tail;
    size_t nn = a->alist_n;
    a->alist_head = b->alist_head;
    a->alist_tail = b->alist_tail;
    a->alist_n = b->alist_n;
    b->alist_head = h;
    b->alist_tail = t;
    b->alist_n = nn;
}

/* Merge-sort the doubly-linked assignment list by assignment_compare. */
static assignment_node_t *alist_merge(assignment_node_t *a, assignment_node_t *b) {
    assignment_node_t head;
    assignment_node_t *tail = &head;
    head.next = NULL;
    while (a && b) {
        if (assignment_compare(&a->a, &b->a) <= 0) {
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

static assignment_node_t *alist_sort_nodes(assignment_node_t *head) {
    if (!head || !head->next) return head;
    assignment_node_t *slow = head, *fast = head->next;
    while (fast && fast->next) { slow = slow->next; fast = fast->next->next; }
    assignment_node_t *mid = slow->next;
    slow->next = NULL;
    if (mid) mid->prev = NULL;
    assignment_node_t *left = alist_sort_nodes(head);
    assignment_node_t *right = alist_sort_nodes(mid);
    return alist_merge(left, right);
}

static void alist_sort(tree_dec_ralloc_node_t *n) {
    n->alist_head = alist_sort_nodes(n->alist_head);
    assignment_node_t *p = n->alist_head, *last = NULL;
    while (p) { last = p; p = p->next; }
    n->alist_tail = last;
}

/* -------------------- add_operand_to_cfg_node helper -------------------- */

/* Key -> var map (OP_SYMBOL->key -> first var index). The original used a
 * std::map<std::pair<int, reg_t>, var_t>. We flatten to (key, byte) -> var. */
typedef struct {
    int   key;
    int   byte;
    short var;
} sym_map_entry_t;

typedef struct {
    sym_map_entry_t *items;
    size_t           n;
    size_t           cap;
} sym_map_t;

static void sym_map_init(sym_map_t *m) { m->items = NULL; m->n = m->cap = 0; }
static void sym_map_free(sym_map_t *m) { free(m->items); m->items = NULL; m->n = m->cap = 0; }

static int sym_map_find(const sym_map_t *m, int key, int byte, short *out) {
    size_t i;
    for (i = 0; i < m->n; i++)
        if (m->items[i].key == key && m->items[i].byte == byte) {
            *out = m->items[i].var; return 1;
        }
    return 0;
}

static void sym_map_put(sym_map_t *m, int key, int byte, short var) {
    if (m->n == m->cap) {
        m->cap = m->cap ? m->cap * 2 : 32;
        m->items = (sym_map_entry_t *)realloc(m->items, m->cap * sizeof(sym_map_entry_t));
    }
    m->items[m->n].key = key;
    m->items[m->n].byte = byte;
    m->items[m->n].var = var;
    m->n++;
}

/* Key -> cfg vertex index map. */
typedef struct {
    int          *keys;
    unsigned int *idxs;
    size_t        n, cap;
} key_idx_map_t;

static void kim_init(key_idx_map_t *m) { m->keys = NULL; m->idxs = NULL; m->n = m->cap = 0; }
static void kim_free(key_idx_map_t *m) { free(m->keys); free(m->idxs); m->keys = NULL; m->idxs = NULL; m->n = m->cap = 0; }

static void kim_put(key_idx_map_t *m, int key, unsigned int idx) {
    if (m->n == m->cap) {
        m->cap = m->cap ? m->cap * 2 : 32;
        m->keys = (int *)realloc(m->keys, m->cap * sizeof(int));
        m->idxs = (unsigned int *)realloc(m->idxs, m->cap * sizeof(unsigned int));
    }
    m->keys[m->n] = key;
    m->idxs[m->n] = idx;
    m->n++;
}

static int kim_get(const key_idx_map_t *m, int key, unsigned int *out) {
    size_t i;
    for (i = 0; i < m->n; i++)
        if (m->keys[i] == key) { *out = m->idxs[i]; return 1; }
    return 0;
}

static void add_operand_to_cfg_node(cfg_node_t *n, operand *o, const sym_map_t *sm) {
    if (!o || !IS_SYMOP(o)) return;
    int k0key = OP_SYMBOL_CONST(o)->key;
    short v0;
    if (!sym_map_find(sm, k0key, 0, &v0)) return;
    if (cfg_operands_has_key(n, k0key)) return;
    int nRegs = OP_SYMBOL_CONST(o)->nRegs;
    int k;
    for (k = 0; k < nRegs; k++) {
        short v;
        if (sym_map_find(sm, k0key, k, &v))
            cfg_operands_insert(n, k0key, v);
    }
}

/* -------------------- create_cfg -------------------- */

iCode *ralloc_create_cfg(cfg_ralloc_t *cfg, con_t *con, ebbIndex *ebbi) {
    eBBlock **ebbs = ebbi->bbOrder;
    iCode *start_ic, *ic;

    key_idx_map_t key_to_index;
    sym_map_t     sym_to_index;
    kim_init(&key_to_index);
    sym_map_init(&sym_to_index);

    start_ic = iCodeLabelOptimize(iCodeFromeBBlock(ebbs, ebbi->count));

    /* Pass 1: create cfg vertices, build conflict graph vertices from live ranges. */
    {
        int i;
        short j;
        wassertl(!cg_num_vertices(&cfg->g), "CFG non-empty before creation.");
        for (ic = start_ic, i = 0, j = 0; ic; ic = ic->next, i++) {
            cfg_add_vertex(cfg);
            kim_put(&key_to_index, ic->key, (unsigned int)i);

            if (ic->op == SEND && ic->builtinSEND) {
                operand *bi_parms[MAX_BUILTIN_ARGS];
                int nbi_parms;
                getBuiltinParms(ic, &nbi_parms, bi_parms);
            }

            ralloc_extra_ic_generated(ic);

            cfg->node[i].ic = ic;

            if (ic->generated) continue;

            int j2;
            for (j2 = 0; j2 <= operandKey; j2++) {
                if (bitVectBitValue(ic->rlive, j2)) {
                    symbol *sym = (symbol *)(hTabItemWithKey(liveRanges, j2));
                    if (!sym->for_newralloc) continue;

                    short dummy;
                    if (sym_map_find(&sym_to_index, j2, 0, &dummy)) continue;

                    int k;
                    for (k = 0; k < sym->nRegs; k++) {
                        con_add_vertex(con);
                        con->node[j].v = j2;
                        con->node[j].byte = k;
                        con->node[j].size = sym->nRegs;
                        con->node[j].name = sym->name;
                        sym_map_put(&sym_to_index, j2, k, j);
                        int l;
                        for (l = 0; l < k; l++)
                            cg_add_edge(&con->g, (unsigned int)(j - l - 1),
                                        (unsigned int)j, 0.0f);
                        j++;
                    }
                }
            }
        }
    }

    /* Pass 2: edges + operand maps + alive sets. */
    for (ic = start_ic; ic; ic = ic->next) {
        unsigned int my_idx;
        if (!kim_get(&key_to_index, ic->key, &my_idx)) continue;

        if (ic->op != GOTO && ic->op != RETURN && ic->op != JUMPTABLE && ic->next) {
            unsigned int nx;
            if (kim_get(&key_to_index, ic->next->key, &nx))
                cg_add_edge(&cfg->g, my_idx, nx, 0.0f);
        }

        if (ic->op == GOTO) {
            unsigned int tgt;
            if (kim_get(&key_to_index,
                        eBBWithEntryLabel(ebbi, ic->label)->sch->key, &tgt))
                cg_add_edge(&cfg->g, my_idx, tgt, 0.0f);
        } else if (ic->op == RETURN) {
            unsigned int tgt;
            if (kim_get(&key_to_index,
                        eBBWithEntryLabel(ebbi, returnLabel)->sch->key, &tgt))
                cg_add_edge(&cfg->g, my_idx, tgt, 0.0f);
        } else if (ic->op == IFX) {
            symbol *lbl = IC_TRUE(ic) ? IC_TRUE(ic) : IC_FALSE(ic);
            unsigned int tgt;
            if (kim_get(&key_to_index, eBBWithEntryLabel(ebbi, lbl)->sch->key, &tgt))
                cg_add_edge(&cfg->g, my_idx, tgt, 0.0f);
        } else if (ic->op == JUMPTABLE) {
            symbol *lbl;
            for (lbl = (symbol *)setFirstItem(IC_JTLABELS(ic)); lbl;
                 lbl = (symbol *)setNextItem(IC_JTLABELS(ic))) {
                unsigned int tgt;
                if (kim_get(&key_to_index,
                            eBBWithEntryLabel(ebbi, lbl)->sch->key, &tgt))
                    cg_add_edge(&cfg->g, my_idx, tgt, 0.0f);
            }
        }

        int i;
        for (i = 0; i <= operandKey; i++) {
            short dummy;
            if (!sym_map_find(&sym_to_index, i, 0, &dummy)) continue;
            if (bitVectBitValue(ic->rlive, i)) {
                symbol *isym = (symbol *)hTabItemWithKey(liveRanges, i);
                int k;
                for (k = 0; k < isym->nRegs; k++) {
                    short v;
                    if (sym_map_find(&sym_to_index, i, k, &v))
                        sss_insert(&cfg->node[my_idx].alive, v);
                }
                if (isym->block)
                    isym->block = btree_lowest_common_ancestor(isym->block, ic->block);
                else
                    isym->block = ic->block;
            }
        }

        if (ic->op == IFX)
            add_operand_to_cfg_node(&cfg->node[my_idx], IC_COND(ic), &sym_to_index);
        else if (ic->op == JUMPTABLE)
            add_operand_to_cfg_node(&cfg->node[my_idx], IC_JTCOND(ic), &sym_to_index);
        else {
            add_operand_to_cfg_node(&cfg->node[my_idx], IC_RESULT(ic), &sym_to_index);
            add_operand_to_cfg_node(&cfg->node[my_idx], IC_LEFT(ic), &sym_to_index);
            add_operand_to_cfg_node(&cfg->node[my_idx], IC_RIGHT(ic), &sym_to_index);
        }

        ralloc_add_operand_conflicts_in_node(&cfg->node[my_idx], con);
    }

    /* Non-connected live ranges workaround — unchanged from original logic.
     * We skip this pass if there are no con vertices. */
    {
        size_t ncon = cg_num_vertices(&con->g);
        size_t ncfg = cg_num_vertices(&cfg->g);
        for (short ii = (short)ncon - 1; ii >= 0; ii--) {
            cgraph_t cfg2;
            cg_init(&cfg2, CG_UNDIRECTED, 0);
            cg_copy_topology(&cfg2, &cfg->g, CG_UNDIRECTED);
            /* "Remove" vertices where alive does not contain ii by clearing their edges.
             * In our cgraph we can't cheaply remove vertices, so we just zero out edges
             * for nodes that don't contain ii. */
            unsigned int j;
            for (j = 0; j < ncfg; j++) {
                if (!sss_contains(&cfg->node[j].alive, ii)) {
                    /* Clear j's adjacency by removing each edge individually. */
                    /* Copy out[j].dst into a buffer first since remove mutates. */
                    size_t deg = cfg2.out[j].n;
                    unsigned int *buf = (unsigned int *)malloc(deg * sizeof(unsigned int));
                    size_t kk;
                    for (kk = 0; kk < deg; kk++) buf[kk] = cfg2.out[j].dst[kk];
                    for (kk = 0; kk < deg; kk++) cg_remove_edge(&cfg2, j, buf[kk]);
                    free(buf);
                }
            }
            unsigned int *comp = (unsigned int *)malloc(ncfg * sizeof(unsigned int));
            size_t ncomp = cg_connected_components(&cfg2, comp);
            if (ncomp > 1) {
                fprintf(stderr,
                        "Warning: Non-connected liverange found and extended to "
                        "connected component of the CFG: %s. Please contact sdcc "
                        "authors with source code to reproduce.\n",
                        con->node[ii].name ? con->node[ii].name : "?");
                /* Recompute over the full (unfiltered) cfg topology. */
                cgraph_t cfg3;
                cg_init(&cfg3, CG_UNDIRECTED, 0);
                cg_copy_topology(&cfg3, &cfg->g, CG_UNDIRECTED);
                unsigned int *comp2 = (unsigned int *)malloc(ncfg * sizeof(unsigned int));
                cg_connected_components(&cfg3, comp2);
                unsigned int jj;
                for (jj = 0; jj + 1 < ncfg; jj++) {
                    if (sss_contains(&cfg->node[jj].alive, ii)) {
                        unsigned int kk;
                        for (kk = 0; kk + 1 < ncfg; kk++) {
                            if (comp2[jj] == comp2[kk])
                                sss_insert(&cfg->node[kk].alive, ii);
                        }
                    }
                }
                free(comp2);
                cg_free(&cfg3);
            }
            free(comp);
            cg_free(&cfg2);
        }
    }

    /* Compute dying sets. dying = alive - {variables needed by successors in
     * meaningful ways; see original}. */
    {
        size_t ncfg = cg_num_vertices(&cfg->g);
        unsigned int i;
        for (i = 0; i < ncfg; i++) {
            sss_copy(&cfg->node[i].dying, &cfg->node[i].alive);
            /* Walk outgoing neighbours. */
            size_t k;
            unsigned int nbr;
            float wt;
            CG_FOREACH_OUT(&cfg->g, i, k, nbr, wt) {
                (void)wt;
                size_t vi;
                for (vi = 0; vi < cfg->node[nbr].alive.n; vi++) {
                    short vv = cfg->node[nbr].alive.items[vi];
                    const symbol *vsym =
                        (symbol *)hTabItemWithKey(liveRanges, con->node[vv].v);
                    const operand *left = IC_LEFT(cfg->node[nbr].ic);
                    const operand *right = IC_RIGHT(cfg->node[nbr].ic);
                    const operand *result = IC_RESULT(cfg->node[nbr].ic);
                    if (!POINTER_SET(cfg->node[nbr].ic) &&
                        (!left || !IS_SYMOP(left) ||
                         OP_SYMBOL_CONST(left)->key != vsym->key) &&
                        (!right || !IS_SYMOP(right) ||
                         OP_SYMBOL_CONST(right)->key != vsym->key) &&
                        result && IS_SYMOP(result) &&
                        OP_SYMBOL_CONST(result)->key == vsym->key)
                        continue;
                    sss_erase(&cfg->node[i].dying, vv);
                }
            }
        }
    }

    /* Add conflict graph edges: pairs of surviving variables at each CFG node. */
    {
        size_t ncfg = cg_num_vertices(&cfg->g);
        unsigned int i;
        for (i = 0; i < ncfg; i++) {
            const iCode *icn = cfg->node[i].ic;
            size_t vi;
            for (vi = 0; vi < cfg->node[i].alive.n; vi++) {
                short v = cfg->node[i].alive.items[vi];
                if (sss_contains(&cfg->node[i].dying, v)) continue;
                /* Skip variables that are the "result of this instruction" — no self-conflict. */
                int skip_this = 0;
                if (icn->op != IFX && icn->op != JUMPTABLE && IC_RESULT(icn) &&
                    IS_SYMOP(IC_RESULT(icn))) {
                    size_t oe;
                    size_t os = cfg_operands_equal_range(
                        &cfg->node[i], OP_SYMBOL_CONST(IC_RESULT(icn))->key, &oe);
                    size_t oi;
                    for (oi = os; oi < oe; oi++)
                        if (cfg->node[i].operands[oi].var == v) { skip_this = 1; break; }
                }
                if (skip_this) continue;

                size_t vi2;
                for (vi2 = 0; vi2 < cfg->node[i].alive.n; vi2++) {
                    short v2 = cfg->node[i].alive.items[vi2];
                    if (v == v2) continue;
                    if (sss_contains(&cfg->node[i].dying, v2)) continue;
                    if (!cg_has_edge(&con->g, (unsigned int)v, (unsigned int)v2))
                        cg_add_edge(&con->g, (unsigned int)v, (unsigned int)v2, 0.0f);
                }
            }
        }
    }

    kim_free(&key_to_index);
    sym_map_free(&sym_to_index);
    return start_ic;
}

/* -------------------- alive_tree_dec -------------------- */

void ralloc_alive_tree_dec(tree_dec_ralloc_t *T, const cfg_ralloc_t *G) {
    size_t nt = cg_num_vertices(&T->td.g);
    unsigned int i;
    for (i = 0; i < nt; i++) {
        sss_clear(&T->node[i].alive);
        size_t bi;
        for (bi = 0; bi < T->td.bag[i].n; bi++) {
            unsigned int v = T->td.bag[i].items[bi];
            size_t k;
            for (k = 0; k < G->node[v].alive.n; k++)
                sss_insert(&T->node[i].alive, G->node[v].alive.items[k]);
        }
    }
}

/* -------------------- assignment helpers for DP -------------------- */

static int assignment_conflict(const assignment_t *a, const con_t *I, short v,
                               reg_t r) {
    size_t i;
    for (i = 0; i < a->local.n; i++) {
        short w = a->local.items[i];
        if (a->global[w] != r) continue;
        if (cg_has_edge(&I->g, (unsigned int)w, (unsigned int)v)) return 1;
    }
    return 0;
}

static int assignments_locally_same(const assignment_t *a1, const assignment_t *a2) {
    if (!sss_equal(&a1->local, &a2->local)) return 0;
    size_t i;
    for (i = 0; i < a1->local.n; i++) {
        short v = a1->local.items[i];
        if (a1->global[v] != a2->global[v]) return 0;
    }
    return 1;
}

/* Compute set-intersection (a.local ∩ G[i].alive) into out (caller-init'd). */
static void sss_intersect_into(sssset_t *out, const sssset_t *a, const sssset_t *b) {
    sss_clear(out);
    size_t i = 0, j = 0;
    while (i < a->n && j < b->n) {
        if (a->items[i] < b->items[j]) i++;
        else if (a->items[i] > b->items[j]) j++;
        else { sss_insert(out, a->items[i]); i++; j++; }
    }
}

static void assignments_introduce_instruction(tree_dec_ralloc_node_t *n,
                                              unsigned short i,
                                              const cfg_ralloc_t *G) {
    assignment_node_t *an;
    sssset_t tmp;
    sss_init(&tmp);
    for (an = n->alist_head; an; an = an->next) {
        sss_intersect_into(&tmp, &an->a.local, &G->node[i].alive);
        i_assignment_t ia;
        i_assignment_init(&ia);
        size_t k;
        for (k = 0; k < tmp.n; k++) {
            short v = tmp.items[k];
            if (an->a.global[v] >= 0)
                i_assignment_add_var(&ia, v, an->a.global[v]);
        }
        an->a.i_assignment = ia;
    }
    sss_free(&tmp);
}

static void assignments_introduce_variable(tree_dec_ralloc_node_t *n,
                                           unsigned short i, short v,
                                           const cfg_ralloc_t *G, const con_t *I) {
    /* Take a snapshot of the current list head/tail; new entries appended during
     * the loop must not be revisited. */
    size_t orig_n = n->alist_n;
    assignment_node_t *an = n->alist_head;
    size_t seen = 0;
    int a_initialized;
    assignment_t a;
    assignment_init(&a);

    while (an && seen < orig_n) {
        a_initialized = 0;
        reg_t r;
        for (r = 0; r < port->num_regs; r++) {
            if (!assignment_conflict(&an->a, I, v, r)) {
                if (!a_initialized) {
                    assignment_copy(&a, &an->a);
                    an->a.marked = 1;
                    a.marked = 0;
                    sss_insert(&a.local, v);
                    a_initialized = 1;
                }
                a.global[v] = r;
                i_assignment_add_var(&a.i_assignment, v, r);
                if (!ralloc_assignment_hopeless(&a, i, G, I, v))
                    alist_push_back_copy(n, &a);
                i_assignment_remove_var(&a.i_assignment, v);
            }
        }
        an = an->next;
        seen++;
    }
    assignment_free(&a);
}

/* -------------------- drop_worst_assignments -------------------- */

typedef struct {
    assignment_node_t *node;
    float              s;
} drop_rep_t;

static float compability_cost(const assignment_t *a, const assignment_t *ac,
                              const con_t *I) {
    float c = 0.0f;
    size_t vi;
    (void)I;
    for (vi = 0; vi < ac->local.n; vi++) {
        short v = ac->local.items[vi];
        if (a->global[v] != ac->global[v]) { c += 1000.0f; continue; }
    }
    return c;
}

static int drop_rep_cmp(const void *x, const void *y) {
    float a = ((const drop_rep_t *)x)->s;
    float b = ((const drop_rep_t *)y)->s;
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

static void drop_worst_assignments(tree_dec_ralloc_node_t *n, unsigned short i,
                                   const cfg_ralloc_t *G, const con_t *I,
                                   const assignment_t *ac,
                                   int *assignment_optimal) {
    size_t alist_size = n->alist_n;
    if (alist_size * (size_t)port->num_regs <= (size_t)options.max_allocs_per_node ||
        alist_size <= 1)
        return;

    *assignment_optimal = 0;

    drop_rep_t *arep = (drop_rep_t *)malloc(alist_size * sizeof(drop_rep_t));
    size_t idx = 0;
    assignment_node_t *an;
    for (an = n->alist_head; an; an = an->next) {
        arep[idx].node = an;
        arep[idx].s = an->a.s + ralloc_rough_cost_estimate(&an->a, i, G, I) +
                      compability_cost(&an->a, ac, I);
        idx++;
    }

    /* Keep the best `keep` entries (excluding the very first one, which we
     * preserve always — matches the std::nth_element with `arep + 1`). */
    size_t keep = (size_t)options.max_allocs_per_node / (size_t)port->num_regs;
    if (keep == 0) keep = 1;
    /* Sort the suffix [1..alist_size) by score and drop the tail. */
    qsort(arep + 1, alist_size - 1, sizeof(drop_rep_t), drop_rep_cmp);

    size_t k;
    for (k = keep + 1; k < alist_size; k++) {
        alist_erase(n, arep[k].node);
    }
    free(arep);
}

/* -------------------- tree_dec_ralloc_leaf -------------------- */

static void tree_dec_ralloc_leaf(tree_dec_ralloc_t *T, unsigned int t,
                                 const cfg_ralloc_t *G, const con_t *I) {
    (void)G;
    assignment_t a;
    assignment_init(&a);
    a.s = 0.0f;
    a.global_n = cg_num_vertices(&I->g);
    a.global = (signed char *)malloc(a.global_n * sizeof(signed char));
    size_t k;
    for (k = 0; k < a.global_n; k++) a.global[k] = -1;
    alist_push_back_move(&T->node[t], &a);
    assignment_free(&a);
}

/* -------------------- tree_dec_ralloc_introduce -------------------- */

static unsigned int first_out_neighbour(const cgraph_t *g, unsigned int v) {
    return g->out[v].dst[0];
}

static void tree_dec_ralloc_introduce(tree_dec_ralloc_t *T, unsigned int t,
                                      const cfg_ralloc_t *G, const con_t *I,
                                      const assignment_t *ac,
                                      int *assignment_optimal) {
    unsigned int c = first_out_neighbour(&T->td.g, t);

    alist_swap(&T->node[t], &T->node[c]);

    sssset_t new_vars;
    sss_init(&new_vars);
    {
        /* set_difference: T[t].alive - T[c].alive where alive is sssset. */
        size_t i = 0, j = 0;
        while (i < T->node[t].alive.n && j < T->node[c].alive.n) {
            short a = T->node[t].alive.items[i];
            short b = T->node[c].alive.items[j];
            if (a < b) { sss_insert(&new_vars, a); i++; }
            else if (a > b) j++;
            else { i++; j++; }
        }
        while (i < T->node[t].alive.n) {
            sss_insert(&new_vars, T->node[t].alive.items[i]); i++;
        }
    }

    /* set_difference over bag (unsigned int). */
    unsigned int new_i;
    {
        size_t i = 0, j = 0;
        new_i = 0;
        int found = 0;
        while (i < T->td.bag[t].n && j < T->td.bag[c].n) {
            unsigned int a = T->td.bag[t].items[i];
            unsigned int b = T->td.bag[c].items[j];
            if (a < b) { new_i = a; found = 1; break; }
            else if (a > b) j++;
            else { i++; j++; }
        }
        if (!found && i < T->td.bag[t].n) new_i = T->td.bag[t].items[i];
    }
    unsigned short i_idx = (unsigned short)new_i;

    assignments_introduce_instruction(&T->node[t], i_idx, G);

    size_t vi;
    for (vi = 0; vi < new_vars.n; vi++) {
        drop_worst_assignments(&T->node[t], i_idx, G, I, ac, assignment_optimal);
        assignments_introduce_variable(&T->node[t], i_idx, new_vars.items[vi], G, I);
    }

    /* Accumulate instruction cost; erase assignments with infinite cost. */
    {
        assignment_node_t *an = T->node[t].alist_head;
        while (an) {
            float c_inst = ralloc_instruction_cost(&an->a, i_idx, G, I);
            assignment_icost_set(&an->a, (int)i_idx, c_inst);
            an->a.s += c_inst;
            if (ra_is_inf(an->a.s)) {
                an = alist_erase(&T->node[t], an);
            } else {
                an = an->next;
            }
        }
    }

    sss_free(&new_vars);
}

/* -------------------- tree_dec_ralloc_forget -------------------- */

static void tree_dec_ralloc_forget(tree_dec_ralloc_t *T, unsigned int t,
                                   const cfg_ralloc_t *G, const con_t *I) {
    (void)G; (void)I;
    unsigned int c = first_out_neighbour(&T->td.g, t);

    alist_swap(&T->node[t], &T->node[c]);

    /* old_inst: T[c].bag - T[t].bag. Take first element. */
    unsigned int old_i;
    int old_i_found = 0;
    {
        size_t i = 0, j = 0;
        while (i < T->td.bag[c].n && j < T->td.bag[t].n) {
            unsigned int a = T->td.bag[c].items[i];
            unsigned int b = T->td.bag[t].items[j];
            if (a < b) { old_i = a; old_i_found = 1; break; }
            else if (a > b) j++;
            else { i++; j++; }
        }
        if (!old_i_found && i < T->td.bag[c].n) { old_i = T->td.bag[c].items[i]; old_i_found = 1; }
    }

    /* old_vars: T[c].alive - T[t].alive. */
    sssset_t old_vars;
    sss_init(&old_vars);
    {
        size_t i = 0, j = 0;
        while (i < T->node[c].alive.n && j < T->node[t].alive.n) {
            short a = T->node[c].alive.items[i];
            short b = T->node[t].alive.items[j];
            if (a < b) { sss_insert(&old_vars, a); i++; }
            else if (a > b) j++;
            else { i++; j++; }
        }
        while (i < T->node[c].alive.n) {
            sss_insert(&old_vars, T->node[c].alive.items[i]); i++;
        }
    }

    /* Restrict each assignment's local to current. */
    {
        assignment_node_t *an;
        for (an = T->node[t].alist_head; an; an = an->next) {
            size_t k;
            for (k = 0; k < old_vars.n; k++)
                sss_erase(&an->a.local, old_vars.items[k]);
            if (old_i_found)
                assignment_icost_erase(&an->a, (int)old_i);
        }
    }

    alist_sort(&T->node[t]);

    /* Collapse locally-identical assignments, keeping the minimum-cost one. */
    {
        assignment_node_t *ai = T->node[t].alist_head;
        while (ai) {
            assignment_node_t *aif = ai;
            ai = ai->next;
            while (ai && assignments_locally_same(&aif->a, &ai->a)) {
                if (aif->a.s > ai->a.s) {
                    alist_erase(&T->node[t], aif);
                    aif = ai;
                    ai = ai->next;
                } else {
                    ai = alist_erase(&T->node[t], ai);
                }
            }
        }
    }

    sss_free(&old_vars);
}

/* -------------------- tree_dec_ralloc_join -------------------- */

static void tree_dec_ralloc_join(tree_dec_ralloc_t *T, unsigned int t,
                                 const cfg_ralloc_t *G, const con_t *I) {
    (void)G; (void)I;
    unsigned int c2 = T->td.g.out[t].dst[0];
    unsigned int c3 = T->td.g.out[t].dst[1];

    tree_dec_ralloc_node_t *alist1 = &T->node[t];
    tree_dec_ralloc_node_t *alist2 = &T->node[c2];
    tree_dec_ralloc_node_t *alist3 = &T->node[c3];

    alist_sort(alist2);
    alist_sort(alist3);

    assignment_node_t *ai2 = alist2->alist_head;
    assignment_node_t *ai3 = alist3->alist_head;
    while (ai2 && ai3) {
        if (assignments_locally_same(&ai2->a, &ai3->a)) {
            ai2->a.s += ai3->a.s;
            /* Avoid double-counting instruction costs in shared bag. */
            size_t bi;
            for (bi = 0; bi < T->td.bag[t].n; bi++) {
                int key = (int)T->td.bag[t].items[bi];
                float v;
                if (assignment_icost_get(&ai2->a, key, &v))
                    ai2->a.s -= v;
            }
            size_t k;
            for (k = 0; k < ai2->a.global_n; k++)
                if (ai2->a.global[k] == -1) ai2->a.global[k] = ai3->a.global[k];
            alist_push_back_copy(alist1, &ai2->a);
            ai2 = ai2->next;
            ai3 = ai3->next;
        } else {
            int cmp = assignment_compare(&ai2->a, &ai3->a);
            if (cmp < 0) ai2 = ai2->next;
            else if (cmp > 0) ai3 = ai3->next;
            else { /* same by cmp but not "locally same" should be impossible; guard */
                ai2 = ai2->next; ai3 = ai3->next;
            }
        }
    }

    alist_clear(alist2);
    alist_clear(alist3);
}

/* -------------------- tree_dec_ralloc_nodes -------------------- */

static void tree_dec_ralloc_nodes(tree_dec_ralloc_t *T, unsigned int t,
                                  const cfg_ralloc_t *G, const con_t *I,
                                  const assignment_t *ac,
                                  int *assignment_optimal) {
    size_t od = cg_out_degree(&T->td.g, t);
    switch (od) {
    case 0:
        tree_dec_ralloc_leaf(T, t, G, I);
        break;
    case 1: {
        unsigned int c0 = T->td.g.out[t].dst[0];
        tree_dec_ralloc_nodes(T, c0, G, I, ac, assignment_optimal);
        if (T->td.bag[c0].n < T->td.bag[t].n)
            tree_dec_ralloc_introduce(T, t, G, I, ac, assignment_optimal);
        else
            tree_dec_ralloc_forget(T, t, G, I);
        break;
    }
    case 2: {
        unsigned int c0 = T->td.g.out[t].dst[0];
        unsigned int c1 = T->td.g.out[t].dst[1];
        tree_dec_ralloc_nodes(T, c0, G, I, ac, assignment_optimal);
        {
            assignment_t ac2;
            assignment_init(&ac2);
            ralloc_get_best_local_assignment_biased(&ac2, c0, T);
            tree_dec_ralloc_nodes(T, c1, G, I, &ac2, assignment_optimal);
            assignment_free(&ac2);
        }
        tree_dec_ralloc_join(T, t, G, I);
        break;
    }
    default:
        fprintf(stderr, "Not nice.\n");
        break;
    }
}

/* -------------------- re_root / good_re_root -------------------- */

/* (vertex, size) pair used by find_best_root. */
typedef struct { unsigned int v; size_t s; } vs_pair_t;

static vs_pair_t find_best_root(const tree_dec_ralloc_t *T, unsigned int t,
                                size_t t_s, unsigned int t_old, size_t t_old_s) {
    size_t od = cg_out_degree(&T->td.g, t);
    vs_pair_t r;
    switch (od) {
    case 0:
        if (t_s > t_old_s) { r.v = t; r.s = t_s; }
        else               { r.v = t_old; r.s = t_old_s; }
        return r;
    case 1: {
        unsigned int c = T->td.g.out[t].dst[0];
        size_t cs = T->node[c].alive.n ? T->node[c].alive.n : t_s;
        return find_best_root(T, c, cs, t_old, t_old_s);
    }
    case 2: {
        unsigned int c0 = T->td.g.out[t].dst[0];
        unsigned int c1 = T->td.g.out[t].dst[1];
        size_t c0s = T->node[c0].alive.n ? T->node[c0].alive.n : t_s;
        vs_pair_t t0 = find_best_root(T, c0, c0s, t_old, t_old_s);
        size_t c1s = T->node[c1].alive.n ? T->node[c1].alive.n : t_s;
        unsigned int new_old = t0.s > t_old_s ? t0.v : t_old;
        size_t new_old_s = t0.s > t_old_s ? t0.s : t_old_s;
        return find_best_root(T, c1, c1s, new_old, new_old_s);
    }
    default:
        fprintf(stderr, "Not nice.\n");
        r.v = t_old; r.s = t_old_s; return r;
    }
}

static void re_root(tree_dec_ralloc_t *T, unsigned int t) {
    if (T->td.g.in[t].n == 0) return;
    unsigned int s0 = t;
    unsigned int s1 = T->td.g.in[t].dst[0];
    while (T->td.g.in[s1].n > 0) {
        unsigned int s2 = T->td.g.in[s1].dst[0];
        cg_remove_edge(&T->td.g, s1, s0);
        cg_add_edge(&T->td.g, s0, s1, 0.0f);
        s0 = s1;
        s1 = s2;
    }
    cg_remove_edge(&T->td.g, s1, s0);
    cg_add_edge(&T->td.g, s0, s1, 0.0f);
}

void ralloc_good_re_root(tree_dec_ralloc_t *T) {
    unsigned int t = tree_dec_find_root(&T->td);

    /* Walk down while the first child has empty alive. */
    while (cg_out_degree(&T->td.g, t) > 0) {
        unsigned int c = T->td.g.out[t].dst[0];
        if (T->node[c].alive.n) break;
        t = c;
    }

    size_t t_s = (cg_out_degree(&T->td.g, t) > 0 ?
                  T->node[T->td.g.out[t].dst[0]].alive.n : 0);
    vs_pair_t best = find_best_root(T, t, t_s, t, t_s);
    t = best.v;

    if (T->node[t].alive.n) {
        fprintf(stderr, "Error: Invalid root.\n");
        return;
    }

    re_root(T, t);
}

/* -------------------- tree_dec_ralloc top-level -------------------- */

int ralloc_tree_dec_ralloc(tree_dec_ralloc_t *T, const cfg_ralloc_t *G,
                           const con_t *I, assignment_t *winner_out) {
    int assignment_optimal = 1;
    assignment_t ac;
    assignment_init(&ac);

    unsigned int root = tree_dec_find_root(&T->td);
    tree_dec_ralloc_nodes(T, root, G, I, &ac, &assignment_optimal);

    /* Winner = first assignment in root's list (should be the only/best one). */
    if (!T->node[root].alist_head) {
        fprintf(stderr, "ERROR: No Assignments at root\n");
        exit(-1);
    }
    assignment_copy(winner_out, &T->node[root].alist_head->a);

    if (winner_out->global_n != cg_num_vertices(&I->g)) {
        fprintf(stderr, "ERROR: No Assignments at root\n");
        exit(-1);
    }

    assignment_free(&ac);
    return !assignment_optimal;
}
