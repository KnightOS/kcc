/* Lifetime-optimal speculative partial redundancy elimination.
 *
 * Original C++ implementation:
 *   Philipp Klaus Krause, 2012. (c) Goethe-Universitat Frankfurt.
 *
 * C port of SDCClospre.cc / SDCClospre.hpp. Uses the in-tree
 * cgraph_t / uiset_t / tree_dec_t containers in place of Boost.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "SDCCopt.h"

#include "util/cgraph.h"
#include "util/uiset.h"
#include "SDCCtree_dec.h"

/* ================================================================= *
 *  Data structures                                                    *
 * ================================================================= */

/* Per-CFG-vertex bundle. */
typedef struct {
    iCode *ic;
    int uses;                 /* bool */
    int invalidates;          /* bool */
    int forward_first;        /* pair<int,int>.first,  -1 sentinel */
    int forward_second;       /* pair<int,int>.second, -1 sentinel */
} cfg_lospre_node_t;

typedef struct {
    cgraph_t g;                 /* CG_BIDIRECTIONAL, has_weights=1 */
    cfg_lospre_node_t *node;
    size_t cap;
} cfg_lospre_t;

/* Assignment (std::list<assignment_lospre> element). */
typedef struct assignment_lospre {
    float s0;                   /* calculation costs                  */
    float s1;                   /* lifetime costs                     */
    usset_t local;              /* std::set<unsigned short int>       */
    char *global;               /* vector<bool>, size == num_vertices */
    size_t global_n;

    struct assignment_lospre *prev, *next;
} assignment_lospre_t;

/* Doubly-linked list of assignments. */
typedef struct {
    assignment_lospre_t *head, *tail;
    size_t n;
} alist_t;

/* Per-tree-dec bag: assignment list lives on the node too. The bag itself
 * is held by the underlying tree_dec_t. We mirror assignments in a parallel
 * array indexed by tree vertex. */
typedef struct {
    alist_t *assignments;       /* size = t->cap, grown alongside tree vertices */
    size_t cap;
} alist_per_vertex_t;

/* ================================================================= *
 *  cfg_lospre lifecycle                                               *
 * ================================================================= */

static void cfg_init(cfg_lospre_t *c) {
    cg_init(&c->g, CG_BIDIRECTIONAL, 1);
    c->node = NULL;
    c->cap  = 0;
}

static void cfg_free(cfg_lospre_t *c) {
    cg_free(&c->g);
    free(c->node);
    c->node = NULL;
    c->cap  = 0;
}

static void cfg_grow(cfg_lospre_t *c) {
    if (c->g.nvertices < c->cap) return;
    size_t nc = c->cap ? c->cap * 2 : 16;
    c->node = (cfg_lospre_node_t *)realloc(c->node, nc * sizeof(cfg_lospre_node_t));
    c->cap  = nc;
}

static unsigned int cfg_add_vertex(cfg_lospre_t *c) {
    cfg_grow(c);
    unsigned int v = cg_add_vertex(&c->g);
    c->node[v].ic = NULL;
    c->node[v].uses = 0;
    c->node[v].invalidates = 0;
    c->node[v].forward_first  = -1;
    c->node[v].forward_second = -1;
    return v;
}

/* ================================================================= *
 *  alist helpers                                                      *
 * ================================================================= */

static void alist_init(alist_t *l) { l->head = l->tail = NULL; l->n = 0; }

static assignment_lospre_t *assignment_new_empty(size_t global_n) {
    assignment_lospre_t *a = (assignment_lospre_t *)calloc(1, sizeof(*a));
    a->s0 = 0; a->s1 = 0;
    usset_init(&a->local);
    a->global = (char *)calloc(global_n > 0 ? global_n : 1, 1);
    a->global_n = global_n;
    return a;
}

static assignment_lospre_t *assignment_clone(const assignment_lospre_t *src) {
    assignment_lospre_t *a = (assignment_lospre_t *)calloc(1, sizeof(*a));
    a->s0 = src->s0;
    a->s1 = src->s1;
    usset_init(&a->local);
    usset_copy(&a->local, &src->local);
    a->global_n = src->global_n;
    a->global = (char *)malloc(src->global_n > 0 ? src->global_n : 1);
    if (src->global_n) memcpy(a->global, src->global, src->global_n);
    return a;
}

static void assignment_free(assignment_lospre_t *a) {
    if (!a) return;
    usset_free(&a->local);
    free(a->global);
    free(a);
}

static void alist_push_back(alist_t *l, assignment_lospre_t *a) {
    a->prev = l->tail;
    a->next = NULL;
    if (l->tail) l->tail->next = a;
    else         l->head = a;
    l->tail = a;
    l->n++;
}

/* Remove node `a` from list, return the successor. Does not free `a`. */
static assignment_lospre_t *alist_unlink(alist_t *l, assignment_lospre_t *a) {
    assignment_lospre_t *nx = a->next;
    if (a->prev) a->prev->next = a->next; else l->head = a->next;
    if (a->next) a->next->prev = a->prev; else l->tail = a->prev;
    a->prev = a->next = NULL;
    l->n--;
    return nx;
}

/* Remove+free node; return successor. */
static assignment_lospre_t *alist_erase(alist_t *l, assignment_lospre_t *a) {
    assignment_lospre_t *nx = alist_unlink(l, a);
    assignment_free(a);
    return nx;
}

static void alist_clear(alist_t *l) {
    assignment_lospre_t *a = l->head;
    while (a) {
        assignment_lospre_t *nx = a->next;
        assignment_free(a);
        a = nx;
    }
    l->head = l->tail = NULL;
    l->n = 0;
}

/* Swap contents of two lists. */
static void alist_swap(alist_t *a, alist_t *b) {
    alist_t tmp = *a; *a = *b; *b = tmp;
}

/* lexicographic: (local element, then global[elem]). Returns <0,0,>0. */
static int assignment_cmp(const assignment_lospre_t *a,
                          const assignment_lospre_t *b) {
    size_t ia = 0, ib = 0;
    for (;;) {
        int aend = (ia >= a->local.n);
        int bend = (ib >= b->local.n);
        if (aend && bend) return 0;
        if (aend) return -1;     /* a is "less" */
        if (bend) return  1;
        unsigned short ae = a->local.items[ia];
        unsigned short be = b->local.items[ib];
        if (ae != be) return ae < be ? -1 : 1;
        int ag = a->global[ae] ? 1 : 0;
        int bg = b->global[be] ? 1 : 0;
        if (ag != bg) return ag < bg ? -1 : 1;
        ia++; ib++;
    }
}

/* `a->s > b->s` in boost::tuple sense: lex by s0 then s1. */
static int assignment_s_greater(const assignment_lospre_t *a,
                                const assignment_lospre_t *b) {
    if (a->s0 != b->s0) return a->s0 > b->s0;
    return a->s1 > b->s1;
}

/* Equal ignoring list links. */
static int assignments_locally_same(const assignment_lospre_t *a,
                                    const assignment_lospre_t *b) {
    if (!usset_equal(&a->local, &b->local)) return 0;
    for (size_t i = 0; i < a->local.n; i++) {
        unsigned short idx = a->local.items[i];
        if ((a->global[idx] ? 1 : 0) != (b->global[idx] ? 1 : 0)) return 0;
    }
    return 1;
}

/* Sort alist via qsort on an array of pointers, then re-link. */
static int alist_sort_cmp(const void *x, const void *y) {
    const assignment_lospre_t *a = *(const assignment_lospre_t *const *)x;
    const assignment_lospre_t *b = *(const assignment_lospre_t *const *)y;
    return assignment_cmp(a, b);
}

static void alist_sort(alist_t *l) {
    if (l->n < 2) return;
    assignment_lospre_t **arr =
        (assignment_lospre_t **)malloc(l->n * sizeof(*arr));
    size_t i = 0;
    for (assignment_lospre_t *a = l->head; a; a = a->next) arr[i++] = a;
    qsort(arr, l->n, sizeof(*arr), alist_sort_cmp);

    l->head = arr[0];
    l->tail = arr[l->n - 1];
    arr[0]->prev = NULL;
    arr[l->n - 1]->next = NULL;
    for (size_t k = 0; k + 1 < l->n; k++) {
        arr[k]->next     = arr[k + 1];
        arr[k + 1]->prev = arr[k];
    }
    free(arr);
}

/* ================================================================= *
 *  alist_per_vertex helpers                                           *
 * ================================================================= */

static void apv_init(alist_per_vertex_t *a) { a->assignments = NULL; a->cap = 0; }

static void apv_ensure(alist_per_vertex_t *a, size_t n) {
    if (n <= a->cap) return;
    size_t nc = a->cap ? a->cap : 8;
    while (nc < n) nc *= 2;
    a->assignments = (alist_t *)realloc(a->assignments, nc * sizeof(alist_t));
    for (size_t i = a->cap; i < nc; i++) alist_init(&a->assignments[i]);
    a->cap = nc;
}

static void apv_free(alist_per_vertex_t *a, size_t n_used) {
    if (!a->assignments) { a->cap = 0; return; }
    for (size_t i = 0; i < n_used && i < a->cap; i++) alist_clear(&a->assignments[i]);
    free(a->assignments);
    a->assignments = NULL;
    a->cap = 0;
}

/* ================================================================= *
 *  CFG construction                                                   *
 * ================================================================= */

/* key -> index map (small: linear search is O(N^2) but acceptable; matches
 * what SDCCnaddr.cc used with std::map). We build a sorted array for
 * binary search. */
typedef struct {
    int key;
    unsigned int idx;
} key_pair_t;

static int key_pair_cmp(const void *a, const void *b) {
    int ka = ((const key_pair_t *)a)->key;
    int kb = ((const key_pair_t *)b)->key;
    return ka < kb ? -1 : (ka > kb ? 1 : 0);
}

static unsigned int key_lookup(const key_pair_t *arr, size_t n, int key) {
    size_t lo = 0, hi = n;
    while (lo < hi) {
        size_t mid = lo + ((hi - lo) >> 1);
        if (arr[mid].key < key) lo = mid + 1;
        else                    hi = mid;
    }
    /* Original std::map[] would default-construct; we trust the key is present. */
    return arr[lo].idx;
}

static void create_cfg_lospre(cfg_lospre_t *cfg, iCode *start_ic, ebbIndex *ebbi) {
    size_t n = 0;
    for (iCode *ic = start_ic; ic; ic = ic->next) n++;

    key_pair_t *k2i = (key_pair_t *)malloc((n ? n : 1) * sizeof(*k2i));
    size_t i = 0;
    for (iCode *ic = start_ic; ic; ic = ic->next, i++) {
        unsigned int v = cfg_add_vertex(cfg);
        cfg->node[v].ic = ic;
        k2i[i].key = ic->key;
        k2i[i].idx = v;
    }
    qsort(k2i, n, sizeof(*k2i), key_pair_cmp);

    for (iCode *ic = start_ic; ic; ic = ic->next) {
        unsigned int src = key_lookup(k2i, n, ic->key);

        if ((ic->op == '>' || ic->op == '<' || ic->op == LE_OP ||
             ic->op == GE_OP || ic->op == EQ_OP || ic->op == NE_OP ||
             ic->op == '^' || ic->op == '|' || ic->op == BITWISEAND) &&
            ifxForOp(IC_RESULT(ic), ic)) {
            cg_add_edge(&cfg->g, src,
                        key_lookup(k2i, n, ic->next->key), 4.0f);
        } else if (ic->op != GOTO && ic->op != RETURN &&
                   ic->op != JUMPTABLE && ic->next) {
            cg_add_edge(&cfg->g, src,
                        key_lookup(k2i, n, ic->next->key), 3.0f);
        }

        if (ic->op == GOTO) {
            cg_add_edge(&cfg->g, src,
                        key_lookup(k2i, n, eBBWithEntryLabel(ebbi, ic->label)->sch->key),
                        6.0f);
        } else if (ic->op == RETURN) {
            cg_add_edge(&cfg->g, src,
                        key_lookup(k2i, n, eBBWithEntryLabel(ebbi, returnLabel)->sch->key),
                        6.0f);
        } else if (ic->op == IFX) {
            symbol *tgt = IC_TRUE(ic) ? IC_TRUE(ic) : IC_FALSE(ic);
            cg_add_edge(&cfg->g, src,
                        key_lookup(k2i, n, eBBWithEntryLabel(ebbi, tgt)->sch->key),
                        6.0f);
        } else if (ic->op == JUMPTABLE) {
            for (symbol *lbl = (symbol *)setFirstItem(IC_JTLABELS(ic)); lbl;
                 lbl = (symbol *)setNextItem(IC_JTLABELS(ic)))
                cg_add_edge(&cfg->g, src,
                            key_lookup(k2i, n, eBBWithEntryLabel(ebbi, lbl)->sch->key),
                            6.0f);
        }
    }

    free(k2i);
}

/* ================================================================= *
 *  Candidate expressions                                              *
 * ================================================================= */

static int candidate_expression(const iCode *ic, int lkey) {
    (void)lkey;
    wassert(ic);

    if (ic->op != '!' && ic->op != '~' && ic->op != UNARYMINUS &&
        ic->op != '+' && ic->op != '-' && ic->op != '*' && ic->op != '/' &&
        ic->op != '%' && ic->op != '>' && ic->op != '<' &&
        ic->op != LE_OP && ic->op != GE_OP && ic->op != NE_OP &&
        ic->op != EQ_OP && ic->op != AND_OP && ic->op != OR_OP &&
        ic->op != '^' && ic->op != '|' && ic->op != BITWISEAND &&
        ic->op != RRC && ic->op != RLC && ic->op != GETABIT &&
        ic->op != GETHBIT && ic->op != LEFT_OP && ic->op != RIGHT_OP &&
        !(ic->op == '=' && !POINTER_SET(ic) && !(IS_ITEMP(IC_RIGHT(ic)))) &&
        ic->op != GET_VALUE_AT_ADDRESS && ic->op != CAST)
        return 0;

    operand *left   = IC_LEFT(ic);
    operand *right  = IC_RIGHT(ic);
    operand *result = IC_RESULT(ic);

    if (ic->op == '=' && IS_OP_LITERAL(right))
        return 0;

    if (IS_OP_VOLATILE(left) || IS_OP_VOLATILE(right))
        return 0;

    if (POINTER_GET(ic) && IS_VOLATILE(operandType(IC_LEFT(ic))->next))
        return 0;

    if ((ic->op != CAST && left && !(IS_SYMOP(left) || IS_OP_LITERAL(left))) ||
        (right  && !(IS_SYMOP(right)  || IS_OP_LITERAL(right))) ||
        (result && !(IS_SYMOP(result) || IS_OP_LITERAL(result))))
        return 0;

    return 1;
}

static int same_expression(const iCode *lic, const iCode *ric) {
    wassert(lic);
    wassert(ric);

    if (lic->op != ric->op) return 0;

    operand *lleft   = IC_LEFT(lic);
    operand *lright  = IC_RIGHT(lic);
    operand *lresult = IC_RESULT(lic);
    operand *rleft   = IC_LEFT(ric);
    operand *rright  = IC_RIGHT(ric);
    operand *rresult = IC_RESULT(ric);

    int ops_match =
        (isOperandEqual(lleft, rleft) && isOperandEqual(lright, rright)) ||
        (IS_COMMUTATIVE(lic) && isOperandEqual(lleft, rright) &&
         isOperandEqual(lright, rleft));

    if (ops_match && lresult && rresult &&
        compareTypeInexact(operandType(lresult), operandType(rresult)) > 0)
        return 1;

    return 0;
}

static void get_candidate_set(iset_t *c, const iCode *sic, int lkey) {
    for (const iCode *ic = sic; ic; ic = ic->next) {
        if (!candidate_expression(ic, lkey)) continue;
        for (const iCode *pic = sic; pic != ic; pic = pic->next) {
            if (candidate_expression(pic, lkey) &&
                same_expression(ic, pic) &&
                !iset_contains(c, pic->key)) {
                iset_insert(c, pic->key);
                break;
            }
        }
    }
}

static int setup_cfg_for_expression(cfg_lospre_t *cfg, const iCode *eic) {
    operand *eleft  = IC_LEFT(eic);
    operand *eright = IC_RIGHT(eic);
    int uses_global =
        (eic->op == GET_VALUE_AT_ADDRESS) ||
        isOperandGlobal(eleft) || isOperandGlobal(eright) ||
        (IS_SYMOP(eleft)  && OP_SYMBOL_CONST(eleft)->addrtaken) ||
        (IS_SYMOP(eright) && OP_SYMBOL_CONST(eright)->addrtaken);
    int safety_required = 0;

    if (eic->op == CALL || eic->op == PCALL)
        safety_required = 1;
    if (eic->op == GET_VALUE_AT_ADDRESS && !optimize.lospre_unsafe_read)
        safety_required = 1;
    if (optimize.codeSpeed)
        safety_required = 1;

    size_t nv = cg_num_vertices(&cfg->g);
    for (unsigned int i = 0; i < nv; i++) {
        iCode *ic = cfg->node[i].ic;
        cfg->node[i].uses        = same_expression(eic, ic);
        cfg->node[i].invalidates = 0;
        if (IC_RESULT(ic) && !IS_OP_LITERAL(IC_RESULT(ic)) && !POINTER_SET(ic) &&
            ((eleft  && isOperandEqual(eleft,  IC_RESULT(ic))) ||
             (eright && isOperandEqual(eright, IC_RESULT(ic)))))
            cfg->node[i].invalidates = 1;
        if (ic->op == FUNCTION || ic->op == ENDFUNCTION || ic->op == RECEIVE)
            cfg->node[i].invalidates = 1;
        if (uses_global && (ic->op == CALL || ic->op == PCALL))
            cfg->node[i].invalidates = 1;
        if (uses_global && POINTER_SET(ic))
            cfg->node[i].invalidates = 1;

        cfg->node[i].forward_first  = -1;
        cfg->node[i].forward_second = -1;
    }

    return safety_required;
}

/* ================================================================= *
 *  Graph dump (optional)                                              *
 * ================================================================= */

static void dump_cfg_lospre(const cfg_lospre_t *cfg) {
    if (!currFunc) return;

    size_t nlen = strlen(dstFileName) + strlen(currFunc->rname) + 64;
    char *path  = (char *)malloc(nlen);
    snprintf(path, nlen, "%s.dumplosprecfg%s.dot", dstFileName, currFunc->rname);
    FILE *f = fopen(path, "w");
    free(path);
    if (!f) return;

    fprintf(f, "digraph G {\n");
    size_t nv = cg_num_vertices(&cfg->g);
    for (size_t i = 0; i < nv; i++) {
        const char *iLine = printILine(cfg->node[i].ic);
        /* Escape embedded quotes/newlines minimally. */
        fprintf(f, "  %zu [label=\"%zu, %d : ", i, i, cfg->node[i].ic->key);
        for (const char *p = iLine; p && *p; p++) {
            if (*p == '"') fputs("\\\"", f);
            else if (*p == '\n') fputs("\\l", f);
            else fputc(*p, f);
        }
        fprintf(f, "\"];\n");
        dbuf_free(iLine);
    }
    for (size_t u = 0; u < nv; u++) {
        for (size_t k = 0; k < cfg->g.out[u].n; k++) {
            unsigned int v = cfg->g.out[u].dst[k];
            fprintf(f, "  %zu -> %u;\n", u, v);
        }
    }
    fprintf(f, "}\n");
    fclose(f);
}

/* ================================================================= *
 *  Tree-decomposition DP                                              *
 * ================================================================= */

/* Return the single child of t in T (exactly one expected). */
static unsigned int t_only_child(const tree_dec_t *T, unsigned int t) {
    return T->g.out[t].dst[0];
}

/* Leaf. */
static void tree_dec_lospre_leaf(alist_per_vertex_t *A, unsigned int t,
                                 const cfg_lospre_t *G) {
    apv_ensure(A, t + 1);
    alist_clear(&A->assignments[t]);
    assignment_lospre_t *a = assignment_new_empty(cg_num_vertices(&G->g));
    alist_push_back(&A->assignments[t], a);
}

/* Introduce. Returns 0 on success, -1 if we pruned. */
static int tree_dec_lospre_introduce(alist_per_vertex_t *A,
                                     const tree_dec_t *T,
                                     unsigned int t,
                                     const cfg_lospre_t *G) {
    (void)G;
    unsigned int c = t_only_child(T, t);
    apv_ensure(A, t + 1);
    alist_t *alist  = &A->assignments[c];
    alist_t *alist2 = &A->assignments[t];
    alist_clear(alist2);

    if (alist->n > (size_t)options.max_allocs_per_node / 2) {
        alist_clear(alist);
        return -1;
    }

    /* new_inst = T[t].bag - T[c].bag  (we take the first element). */
    uiset_t new_inst;
    uiset_init(&new_inst);
    uiset_difference(&T->bag[t], &T->bag[c], &new_inst);
    if (new_inst.n == 0) { uiset_free(&new_inst); alist_clear(alist); return 0; }
    unsigned short i = (unsigned short)new_inst.items[0];
    uiset_free(&new_inst);

    for (assignment_lospre_t *ai = alist->head; ai; ai = ai->next) {
        usset_insert(&ai->local, i);
        /* Emit copy with global[i]=false. */
        ai->global[i] = 0;
        assignment_lospre_t *c0 = assignment_clone(ai);
        alist_push_back(alist2, c0);
        /* Emit copy with global[i]=true. */
        ai->global[i] = 1;
        assignment_lospre_t *c1 = assignment_clone(ai);
        alist_push_back(alist2, c1);
    }
    alist_clear(alist);
    return 0;
}

/* Collapse: assumes alist is sorted. */
static void alist_collapse_locally_same(alist_t *alist) {
    assignment_lospre_t *ai = alist->head;
    while (ai) {
        assignment_lospre_t *aif = ai;
        assignment_lospre_t *aj  = ai->next;
        while (aj && assignments_locally_same(aif, aj)) {
            if (assignment_s_greater(aif, aj)) {
                alist_unlink(alist, aif);
                assignment_free(aif);
                aif = aj;
                aj  = aj->next;
            } else {
                assignment_lospre_t *nx = aj->next;
                alist_unlink(alist, aj);
                assignment_free(aj);
                aj = nx;
            }
        }
        ai = aif->next;
    }
}

/* Forget. */
static void tree_dec_lospre_forget(alist_per_vertex_t *A,
                                   const tree_dec_t *T,
                                   unsigned int t,
                                   const cfg_lospre_t *G) {
    unsigned int c = t_only_child(T, t);
    apv_ensure(A, t + 1);
    alist_t *alist = &A->assignments[t];
    alist_clear(alist);
    alist_swap(alist, &A->assignments[c]);

    uiset_t old_inst;
    uiset_init(&old_inst);
    uiset_difference(&T->bag[c], &T->bag[t], &old_inst);
    if (old_inst.n == 0) { uiset_free(&old_inst); return; }
    unsigned short i = (unsigned short)old_inst.items[0];
    uiset_free(&old_inst);

    for (assignment_lospre_t *ai = alist->head; ai; ai = ai->next) {
        usset_erase(&ai->local, i);
        ai->s1 += ai->global[i] ? 1.0f : 0.0f;

        /* Out-edges: (i -> tgt). */
        size_t idx; unsigned int tgt; float wt;
        CG_FOREACH_OUT(&G->g, i, idx, tgt, wt) {
            /* ai->local.find(tgt) == end? */
            if (!usset_contains(&ai->local, (unsigned short)tgt))
                continue;
            int l = (ai->global[i] && !G->node[i].invalidates) ? 1 : 0;
            int r = (ai->global[tgt] || G->node[tgt].uses) ? 1 : 0;
            if (l >= r) continue;
            ai->s0 += wt;
        }
        /* In-edges: (src -> i). */
        unsigned int src;
        CG_FOREACH_IN(&G->g, i, idx, src, wt) {
            if (!usset_contains(&ai->local, (unsigned short)src))
                continue;
            int l = (ai->global[src] && !G->node[src].invalidates) ? 1 : 0;
            int r = (ai->global[i]   || G->node[i].uses) ? 1 : 0;
            if (l >= r) continue;
            ai->s0 += wt;
        }
    }

    alist_sort(alist);
    alist_collapse_locally_same(alist);

    if (!alist->n)
        fprintf(stderr, "No surviving assignments at forget node (lospre).\n");
}

/* Join (used by both lospre and safety). */
static void tree_dec_lospre_join(alist_per_vertex_t *A,
                                 const tree_dec_t *T,
                                 unsigned int t,
                                 const cfg_lospre_t *G) {
    (void)G;
    /* Two children. */
    unsigned int c2 = T->g.out[t].dst[0];
    unsigned int c3 = T->g.out[t].dst[1];

    apv_ensure(A, t + 1);
    alist_t *alist1 = &A->assignments[t];
    alist_t *alist2 = &A->assignments[c2];
    alist_t *alist3 = &A->assignments[c3];
    alist_clear(alist1);

    alist_sort(alist2);
    alist_sort(alist3);

    assignment_lospre_t *ai2 = alist2->head;
    assignment_lospre_t *ai3 = alist3->head;

    while (ai2 && ai3) {
        if (assignments_locally_same(ai2, ai3)) {
            /* Merge: combine costs into ai2, OR global, push copy to alist1. */
            ai2->s0 += ai3->s0;
            ai2->s1 += ai3->s1;
            size_t N = ai2->global_n;
            for (size_t i = 0; i < N; i++)
                ai2->global[i] = (ai2->global[i] || ai3->global[i]) ? 1 : 0;
            alist_push_back(alist1, assignment_clone(ai2));
            ai2 = ai2->next;
            ai3 = ai3->next;
        } else {
            int cmp = assignment_cmp(ai2, ai3);
            if (cmp < 0)      ai2 = ai2->next;
            else if (cmp > 0) ai3 = ai3->next;
            else {
                /* Equal by cmp but not locally_same -> advance both to avoid
                 * looping. The original `continue` loops forever in that
                 * theoretically-unreachable case; we advance conservatively. */
                ai2 = ai2->next;
                ai3 = ai3->next;
            }
        }
    }

    alist_clear(alist2);
    alist_clear(alist3);
}

/* Dispatcher: returns 0 or -1 propagated. */
static int tree_dec_lospre_nodes(alist_per_vertex_t *A,
                                 const tree_dec_t *T,
                                 unsigned int t,
                                 const cfg_lospre_t *G) {
    size_t od = cg_out_degree(&T->g, t);
    switch (od) {
    case 0:
        tree_dec_lospre_leaf(A, t, G);
        break;
    case 1: {
        unsigned int c0 = T->g.out[t].dst[0];
        if (tree_dec_lospre_nodes(A, T, c0, G) < 0) return -1;
        if (T->bag[c0].n < T->bag[t].n) {
            if (tree_dec_lospre_introduce(A, T, t, G) < 0) return -1;
        } else {
            tree_dec_lospre_forget(A, T, t, G);
        }
        break;
    }
    case 2: {
        unsigned int c0 = T->g.out[t].dst[0];
        unsigned int c1 = T->g.out[t].dst[1];
        if (tree_dec_lospre_nodes(A, T, c0, G) < 0) return -1;
        if (tree_dec_lospre_nodes(A, T, c1, G) < 0) {
            alist_clear(&A->assignments[c0]);
            return -1;
        }
        tree_dec_lospre_join(A, T, t, G);
        break;
    }
    default:
        fprintf(stderr, "Not nice.\n");
        break;
    }
    return 0;
}

/* Safety forget. */
static void tree_dec_safety_forget(alist_per_vertex_t *A,
                                   const tree_dec_t *T,
                                   unsigned int t,
                                   const cfg_lospre_t *G) {
    unsigned int c = t_only_child(T, t);
    apv_ensure(A, t + 1);
    alist_t *alist = &A->assignments[t];
    alist_clear(alist);
    alist_swap(alist, &A->assignments[c]);

    uiset_t old_inst;
    uiset_init(&old_inst);
    uiset_difference(&T->bag[c], &T->bag[t], &old_inst);
    if (old_inst.n == 0) { uiset_free(&old_inst); return; }
    unsigned short i = (unsigned short)old_inst.items[0];
    uiset_free(&old_inst);

    assignment_lospre_t *ai = alist->head;
    while (ai) {
        usset_erase(&ai->local, i);

        if (!ai->global[i]) { ai = ai->next; continue; }

        if (G->node[i].uses) {
            ai = alist_erase(alist, ai);
            continue;
        }

        ai->s1 -= 1.0f;

        /* At least one successor "ok". */
        int ok = 0;
        size_t idx; unsigned int nbr; float wt;
        CG_FOREACH_OUT(&G->g, i, idx, nbr, wt) {
            if (ai->global[nbr] || G->node[nbr].invalidates) { ok = 1; break; }
        }
        if (!ok) { ai = alist_erase(alist, ai); continue; }

        /* At least one predecessor "ok". */
        ok = 0;
        CG_FOREACH_IN(&G->g, i, idx, nbr, wt) {
            if (ai->global[nbr] || G->node[nbr].invalidates) { ok = 1; break; }
        }
        if (!ok) { ai = alist_erase(alist, ai); continue; }

        ai = ai->next;
    }

    alist_sort(alist);
    alist_collapse_locally_same(alist);

    if (!alist->n)
        fprintf(stderr, "No surviving assignments at forget node.\n");
}

/* Safety dispatcher. */
static int tree_dec_safety_nodes(alist_per_vertex_t *A,
                                 const tree_dec_t *T,
                                 unsigned int t,
                                 const cfg_lospre_t *G) {
    size_t od = cg_out_degree(&T->g, t);
    switch (od) {
    case 0:
        tree_dec_lospre_leaf(A, t, G);
        break;
    case 1: {
        unsigned int c0 = T->g.out[t].dst[0];
        if (tree_dec_safety_nodes(A, T, c0, G) < 0) return -1;
        if (T->bag[c0].n < T->bag[t].n) {
            if (tree_dec_lospre_introduce(A, T, t, G) < 0) return -1;
        } else {
            tree_dec_safety_forget(A, T, t, G);
        }
        break;
    }
    case 2: {
        unsigned int c0 = T->g.out[t].dst[0];
        unsigned int c1 = T->g.out[t].dst[1];
        if (T->weight[c0] < T->weight[c1]) {
            unsigned int tmp = c0; c0 = c1; c1 = tmp;
        }
        if (tree_dec_safety_nodes(A, T, c0, G) < 0) return -1;
        if (tree_dec_safety_nodes(A, T, c1, G) < 0) {
            alist_clear(&A->assignments[c0]);
            return -1;
        }
        tree_dec_lospre_join(A, T, t, G);
        break;
    }
    default:
        fprintf(stderr, "Not nice.\n");
        break;
    }
    return 0;
}

/* ================================================================= *
 *  Split edge                                                         *
 * ================================================================= */

static void split_edge(tree_dec_t *T, cfg_lospre_t *G,
                       unsigned int esrc, unsigned int edst,
                       float ewt,
                       const iCode *ic, operand *tmpop) {
    /* Insert new iCode into chain. */
    iCode *newic = newiCode(ic->op, IC_LEFT(ic), IC_RIGHT(ic));
    IC_RESULT(newic) = tmpop;
    newic->filename = ic->filename;
    newic->lineno   = ic->lineno;
    newic->prev = G->node[esrc].ic;
    newic->next = G->node[edst].ic;
    G->node[esrc].ic->next = newic;
    G->node[edst].ic->prev = newic;

    /* Insert node into cfg. */
    unsigned int n = cfg_add_vertex(G);
    G->node[n].ic   = newic;
    G->node[n].uses = 0;
    cg_add_edge(&G->g, esrc, n, ewt);
    cg_add_edge(&G->g, n, edst, 3.0f);

    /* Update tree-decomposition: find a bag containing both endpoints and
     * attach a new bag {esrc, edst, n} to it. Grow tree state first. */
    size_t n_tv = T->g.nvertices;
    for (unsigned int n1 = 0; n1 < n_tv; n1++) {
        if (!uiset_contains(&T->bag[n1], esrc)) continue;
        if (!uiset_contains(&T->bag[n1], edst)) continue;
        unsigned int n2 = tree_dec_add_vertex(T);
        uiset_insert(&T->bag[n2], esrc);
        uiset_insert(&T->bag[n2], edst);
        uiset_insert(&T->bag[n2], n);
        cg_add_edge(&T->g, n1, n2, 0.0f);
        break;
    }

    /* Remove old edge. */
    cg_remove_edge(&G->g, esrc, edst);
}

/* ================================================================= *
 *  forward_lospre_assignment                                          *
 * ================================================================= */

static void forward_lospre_assignment(cfg_lospre_t *G, unsigned int i,
                                      const iCode *ic,
                                      const assignment_lospre_t *a) {
    operand *tmpop = IC_RIGHT(ic);
    int forward_first  = IC_RESULT(ic)->key;
    int forward_second = IC_RIGHT(ic)->key;

    for (;;) {
        if (G->node[i].forward_first  == forward_first &&
            G->node[i].forward_second == forward_second)
            break; /* Already visited. */

        iCode *nic = G->node[i].ic;

        if (isOperandEqual(IC_RESULT(ic), IC_LEFT(nic)) &&
            nic->op != ADDRESS_OF &&
            (!POINTER_GET(nic) || !IS_PTR(operandType(IC_RESULT(nic))) ||
             !IS_BITFIELD(operandType(IC_LEFT(nic))->next) ||
             compareType(operandType(IC_LEFT(nic)), operandType(tmpop)) == 1)) {
            unsigned int isaddr = IC_LEFT(nic)->isaddr;
            IC_LEFT(nic) = operandFromOperand(tmpop);
            IC_LEFT(nic)->isaddr = isaddr;
        }
        if (isOperandEqual(IC_RESULT(ic), IC_RIGHT(nic))) {
            IC_RIGHT(nic) = operandFromOperand(tmpop);
        }
        if (POINTER_SET(nic) && isOperandEqual(IC_RESULT(ic), IC_RESULT(nic)) &&
            (!IS_PTR(operandType(IC_RESULT(nic))) ||
             !IS_BITFIELD(operandType(IC_RESULT(nic))->next) ||
             compareType(operandType(IC_RESULT(nic)), operandType(tmpop)) == 1)) {
            IC_RESULT(nic) = operandFromOperand(tmpop);
            IC_RESULT(nic)->isaddr = 1;
        }

        if (nic->op == LABEL) {
            /* Continue only if all in-edges are already forwarded. */
            int all_forwarded = 1;
            size_t idx; unsigned int src; float wt;
            CG_FOREACH_IN(&G->g, i, idx, src, wt) {
                (void)wt;
                if (G->node[src].forward_first  != forward_first ||
                    G->node[src].forward_second != forward_second) {
                    all_forwarded = 0; break;
                }
            }
            if (!all_forwarded) break;
        }

        if (isOperandEqual(IC_RESULT(ic), IC_RESULT(nic)) && !POINTER_SET(nic))
            break;
        if ((nic->op == CALL || nic->op == PCALL || POINTER_SET(nic)) &&
            IS_TRUE_SYMOP(IC_RESULT(ic)))
            break;

        G->node[i].forward_first  = forward_first;
        G->node[i].forward_second = forward_second;

        if (nic->op == GOTO || nic->op == IFX || nic->op == JUMPTABLE) {
            size_t idx; unsigned int cnbr; float wt;
            CG_FOREACH_OUT(&G->g, i, idx, cnbr, wt) {
                (void)wt;
                int l = (a->global[i] && !G->node[i].invalidates) ? 1 : 0;
                int r = (a->global[cnbr]) ? 1 : 0;
                if (!l && r) continue; /* Calculation edge */
                forward_lospre_assignment(G, cnbr, ic, a);
            }
            break;
        }

        if (G->g.out[i].n == 0) break;
        unsigned int cnbr = G->g.out[i].dst[0];
        int l = (a->global[i] && !G->node[i].invalidates) ? 1 : 0;
        int r = (a->global[cnbr]) ? 1 : 0;
        if (!l && r) break; /* Calculation edge */
        i = cnbr;
    }
}

/* ================================================================= *
 *  implement_lospre_assignment / implement_safety                     *
 * ================================================================= */

typedef struct {
    unsigned int src, dst;
    float wt;
} edge_rec_t;

static int implement_lospre_assignment(const assignment_lospre_t *a_in,
                                       tree_dec_t *T, cfg_lospre_t *G,
                                       const iCode *ic) {
    /* Clone assignment so it survives tree-dec mutations. */
    assignment_lospre_t *a = assignment_clone(a_in);

    unsigned substituted = 0, split = 0;

    /* Collect calculation edges. */
    edge_rec_t *edges = NULL;
    size_t n_edges = 0, cap_edges = 0;
    size_t nv = cg_num_vertices(&G->g);
    for (unsigned int u = 0; u < nv; u++) {
        size_t idx; unsigned int v; float wt;
        CG_FOREACH_OUT(&G->g, u, idx, v, wt) {
            int l = (a->global[u] && !G->node[u].invalidates) ? 1 : 0;
            int r = (a->global[v]) ? 1 : 0;
            if (l >= r) continue;
            if (n_edges == cap_edges) {
                cap_edges = cap_edges ? cap_edges * 2 : 8;
                edges = (edge_rec_t *)realloc(edges, cap_edges * sizeof(*edges));
            }
            edges[n_edges].src = u;
            edges[n_edges].dst = v;
            edges[n_edges].wt  = wt;
            n_edges++;
        }
    }

    if (!n_edges) {
        free(edges);
        assignment_free(a);
        return 0;
    }

    operand *tmpop = newiTempOperand(operandType(IC_RESULT(ic)), TRUE);
    tmpop->isvolatile = 0;

    for (size_t k = 0; k < n_edges; k++) {
        split_edge(T, G, edges[k].src, edges[k].dst, edges[k].wt, ic, tmpop);
        split++;
    }
    free(edges);

    /* After splitting, `a->global` doesn't cover new vertices (they were
     * appended). It still indexes original vertices, which is what we need. */
    nv = cg_num_vertices(&G->g);
    for (unsigned int v = 0; v < nv; v++) {
        if (!G->node[v].uses) continue;
        if (a->global_n <= v) continue;
        int has_in = (G->g.in[v].n > 0);
        unsigned int esrc = has_in ? G->g.in[v].dst[0] : 0;

        int cond1 = (a->global[v] && !G->node[v].invalidates) ? 1 : 0;
        int cond2 = (has_in && esrc < a->global_n && a->global[esrc]) ? 1 : 0;
        if (!cond1 && !cond2) continue;

        substituted++;
        iCode *iic = G->node[v].ic;
        IC_RIGHT(iic) = tmpop;
        if (!POINTER_SET(iic)) {
            IC_LEFT(iic) = 0;
            iic->op = '=';
            IC_RESULT(iic) = operandFromOperand(IC_RESULT(iic));
            IC_RESULT(iic)->isaddr = 0;
        }
        if (IS_OP_VOLATILE(IC_RESULT(iic))) continue;

        if (G->g.out[v].n > 0) {
            unsigned int cnbr = G->g.out[v].dst[0];
            forward_lospre_assignment(G, cnbr, iic, a);
        }
    }

    if (substituted <= 0) {
        fprintf(stderr, "Introduced %s, but did not substitute any calculations.\n",
                OP_SYMBOL_CONST(tmpop)->name);
        assignment_free(a);
        return -1;
    }

    if (substituted < split) {
        fprintf(stdout,
                "Introduced %s, but did substitute only %u calculations, "
                "while introducing %u.\n",
                OP_SYMBOL_CONST(tmpop)->name, substituted, split);
        fflush(stdout);
    }

    assignment_free(a);
    return 1;
}

static void implement_safety(const assignment_lospre_t *a, cfg_lospre_t *G) {
    size_t nv = cg_num_vertices(&G->g);
    for (unsigned int v = 0; v < nv; v++) {
        if (v < a->global_n)
            G->node[v].invalidates |= a->global[v] ? 1 : 0;
    }
}

/* ================================================================= *
 *  tree_dec_lospre / tree_dec_safety                                  *
 * ================================================================= */

static int tree_dec_lospre(tree_dec_t *T, cfg_lospre_t *G, const iCode *ic) {
    alist_per_vertex_t A;
    apv_init(&A);
    apv_ensure(&A, T->g.nvertices);

    unsigned int root = tree_dec_find_root(T);
    int err = tree_dec_lospre_nodes(&A, T, root, G);
    if (err) {
        apv_free(&A, T->g.nvertices);
        return -1;
    }

    /* Grow in case root was mutated. */
    apv_ensure(&A, T->g.nvertices);

    alist_t *rlist = &A.assignments[root];
    wassert(rlist->head != NULL);
    assignment_lospre_t *winner = rlist->head;

    int change = implement_lospre_assignment(winner, T, G, ic);
    if (change) tree_dec_nicify(T);
    alist_clear(rlist);

    apv_free(&A, T->g.nvertices);
    return change;
}

static int tree_dec_safety(tree_dec_t *T, cfg_lospre_t *G, const iCode *ic) {
    (void)ic;
    alist_per_vertex_t A;
    apv_init(&A);
    apv_ensure(&A, T->g.nvertices);

    unsigned int root = tree_dec_find_root(T);
    int err = tree_dec_safety_nodes(&A, T, root, G);
    if (err) {
        apv_free(&A, T->g.nvertices);
        return -1;
    }

    apv_ensure(&A, T->g.nvertices);
    alist_t *rlist = &A.assignments[root];
    wassert(rlist->head != NULL);
    assignment_lospre_t *winner = rlist->head;
    implement_safety(winner, G);
    alist_clear(rlist);

    apv_free(&A, T->g.nvertices);
    return 0;
}

/* ================================================================= *
 *  Public entry point                                                 *
 * ================================================================= */

void lospre(iCode *sic, ebbIndex *ebbi) {
    cfg_lospre_t cfg;
    tree_dec_t   td;

    wassert(sic);

    cfg_init(&cfg);
    tree_dec_init(&td);

    create_cfg_lospre(&cfg, sic, ebbi);

    if (options.dump_graphs)
        dump_cfg_lospre(&cfg);

    tree_dec_thorup(&td, &cfg.g);
    tree_dec_nicify(&td);

    int lkey = operandKey;

    int change = 1;
    while (change) {
        change = 0;

        iset_t candidates;
        iset_init(&candidates);
        get_candidate_set(&candidates, sic, lkey);

        for (size_t k = 0; k < candidates.n; k++) {
            int ckey = candidates.items[k];
            const iCode *ic;
            for (ic = sic; ic && ic->key != ckey; ic = ic->next) ;
            if (!ic || !candidate_expression(ic, lkey)) continue;

            int safety = setup_cfg_for_expression(&cfg, ic);
            if (safety && tree_dec_safety(&td, &cfg, ic) < 0) continue;
            if (tree_dec_lospre(&td, &cfg, ic) > 0) change = 1;
        }

        iset_free(&candidates);
    }

    tree_dec_free(&td);
    cfg_free(&cfg);
}
