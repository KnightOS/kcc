/* SDCCralloc.h — public API for the C port of SDCCralloc.hpp.
 *
 * An optimal, polynomial-time register allocator (Krause 2013). The generic
 * DP algorithm lives in SDCCralloc.c; the six port-specific customization
 * hooks are defined by the backend (see backend/ralloc2.c for Z80).
 */
#ifndef KCC_SDCCRALLOC_H
#define KCC_SDCCRALLOC_H

#include <stddef.h>

#include "SDCCtree_dec.h"
#include "util/cgraph.h"
#include "util/uiset.h"

#ifdef __cplusplus
extern "C" {
#endif

struct iCode;    /* from common.h / SDCCicode.h */
struct ebbIndex; /* from SDCCBBlock.h */

/* A signed-short variable index; matches var_t in the original. */
typedef short var_t;
typedef signed char reg_t;

/* Integer upper bound on port->num_regs; used to size fixed per-instruction
 * state. */
#define MAX_NUM_REGS 9

/* -------------------- per-instruction assignment state -------------------- */

typedef struct {
    short registers[MAX_NUM_REGS][2];
} i_assignment_t;

void i_assignment_init(i_assignment_t *ia);
void i_assignment_add_var(i_assignment_t *ia, short v, signed char r);
void i_assignment_remove_var(i_assignment_t *ia, short v);

/* -------------------- map<int, float> helper -------------------- */

typedef struct {
    int   key;
    float val;
} icost_entry_t;

/* -------------------- assignment (DP state) -------------------- */

typedef struct {
    float          s;
    sssset_t       local;      /* set<var_t> */
    signed char   *global;     /* global[var] = reg (-1 if none); length global_n */
    size_t         global_n;

    icost_entry_t *i_costs;    /* sorted by key */
    size_t         i_costs_n;
    size_t         i_costs_cap;

    i_assignment_t i_assignment;
    int            marked;
} assignment_t;

void  assignment_init(assignment_t *a);
void  assignment_free(assignment_t *a);
/* Deep copy src->dst. Frees dst first. */
void  assignment_copy(assignment_t *dst, const assignment_t *src);
/* Move src -> dst (dst takes ownership; src becomes empty). */
void  assignment_move(assignment_t *dst, assignment_t *src);

/* icost map ops. */
void  assignment_icost_set(assignment_t *a, int key, float val);
int   assignment_icost_get(const assignment_t *a, int key, float *out);
void  assignment_icost_erase(assignment_t *a, int key);

/* Lexicographic compare for the sorted-merge at join nodes (same semantics
 * as the original `operator<`). Returns -1/0/1. */
int   assignment_compare(const assignment_t *x, const assignment_t *y);

/* -------------------- cfg node -------------------- */

/* Multimap entry: key (SDCC symbol key) -> var; multiple entries may share
 * key (one per byte). Kept sorted by key. */
typedef struct {
    int   key;
    short var;
} operand_entry_t;

typedef struct {
    struct iCode   *ic;
    operand_entry_t *operands;
    size_t          operands_n;
    size_t          operands_cap;
    sssset_t        alive;
    sssset_t        dying;
} cfg_node_t;

typedef struct {
    cgraph_t    g;       /* CG_BIDIRECTIONAL */
    cfg_node_t *node;
    size_t      cap;
} cfg_ralloc_t;

void cfg_ralloc_init(cfg_ralloc_t *c);
void cfg_ralloc_free(cfg_ralloc_t *c);

/* Return first index with key >= k (lower_bound in operands array). */
size_t cfg_operands_lower_bound(const cfg_node_t *n, int k);
/* Return start/end indices of entries with operand key k. end stored in *e_out. */
size_t cfg_operands_equal_range(const cfg_node_t *n, int k, size_t *e_out);

/* -------------------- conflict graph -------------------- */

typedef struct {
    int   v;
    int   byte;
    int   size;
    char *name;
} con_node_t;

typedef struct {
    cgraph_t    g;      /* CG_UNDIRECTED */
    con_node_t *node;
    size_t      cap;
} con_t;

void con_init(con_t *c);
void con_free(con_t *c);

/* -------------------- tree decomposition node data -------------------- */

typedef struct assignment_node {
    assignment_t            a;
    struct assignment_node *prev;
    struct assignment_node *next;
} assignment_node_t;

typedef struct {
    sssset_t           alive;
    assignment_node_t *alist_head;
    assignment_node_t *alist_tail;
    size_t             alist_n;
} tree_dec_ralloc_node_t;

typedef struct {
    tree_dec_t              td;
    tree_dec_ralloc_node_t *node;
    size_t                  cap;
} tree_dec_ralloc_t;

void tree_dec_ralloc_init(tree_dec_ralloc_t *T);
void tree_dec_ralloc_free(tree_dec_ralloc_t *T);

/* -------------------- generic algorithm entry points -------------------- */

/* Build the CFG + conflict graph from SDCC iCodes. Returns the first ic. */
struct iCode *ralloc_create_cfg(cfg_ralloc_t *cfg, con_t *conflict_graph,
                                struct ebbIndex *ebbi);

/* Populate tree_dec.alive sets from the CFG. */
void ralloc_alive_tree_dec(tree_dec_ralloc_t *T, const cfg_ralloc_t *G);

/* Re-root tree decomposition to improve the assignment-removal heuristic. */
void ralloc_good_re_root(tree_dec_ralloc_t *T);

/* Top-level allocator. Returns 0 if assignment is optimal, 1 if pruning kicked
 * in. Writes the best assignment to the winner out-parameter. Caller owns the
 * returned assignment's internal buffers and must assignment_free() it. */
int  ralloc_tree_dec_ralloc(tree_dec_ralloc_t *T, const cfg_ralloc_t *G,
                            const con_t *I, assignment_t *winner_out);

/* -------------------- port-specific customization hooks -------------------- */

/* Computed cost of running instruction i under assignment a. */
float ralloc_instruction_cost(const assignment_t *a, unsigned int i,
                              const cfg_ralloc_t *G, const con_t *I);

/* Early-prune: may the given partial assignment be extended to a valid one? */
int   ralloc_assignment_hopeless(const assignment_t *a, unsigned int i,
                                 const cfg_ralloc_t *G, const con_t *I,
                                 short lastvar);

/* Rough cost estimate, used for dropping worst assignments. */
float ralloc_rough_cost_estimate(const assignment_t *a, unsigned int i,
                                 const cfg_ralloc_t *G, const con_t *I);

/* Add conflict-graph edges due to operand-usage patterns at this cfg node. */
void  ralloc_add_operand_conflicts_in_node(const cfg_node_t *n, con_t *I);

/* Like get_best_local_assignment but biased to avoid "risky" registers. */
void  ralloc_get_best_local_assignment_biased(assignment_t *out,
                                              unsigned int t,
                                              const tree_dec_ralloc_t *T);

/* Mark iCodes whose code gets generated as a side effect of another. */
void  ralloc_extra_ic_generated(struct iCode *ic);

/* Public entry point used by ralloc.c. */
struct iCode *z80_ralloc2_cc(struct ebbIndex *ebbi);

#ifdef __cplusplus
}
#endif

#endif
