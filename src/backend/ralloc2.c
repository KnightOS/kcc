/* backend/ralloc2.c — pure-C port of backend/ralloc2.cc.
 *
 * Z80 port-specific instruction-cost function and public entry point for the
 * optimal register allocator (Krause 2013). The generic DP lives in
 * SDCCralloc.c; this file supplies the six customization hooks plus
 * z80_ralloc2_cc().
 */

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

#include "SDCCralloc.h"
#include "z80.h"
#include "gen.h"
#include "ralloc.h"

/* dryZ80iCode lives in gen.c; not declared in any public header. */
unsigned char dryZ80iCode(iCode *ic);

bool z80_assignment_optimal;
bool should_omit_frame_ptr;

#define REG_C 0
#define REG_B 1
#define REG_E 2
#define REG_D 3
#define REG_L 4
#define REG_H 5
#define REG_IYL 6
#define REG_IYH 7
#define REG_A (port->num_regs - 1)

static const float RA_INF = 1.0f / 0.0f;
static int ra_is_inf(float f) { return f == RA_INF || !(f < RA_INF); }

/* -------------------- default_operand_cost -------------------- */

static float default_operand_cost(const operand *o, const assignment_t *a,
                                  unsigned short i, const cfg_ralloc_t *G,
                                  const con_t *I) {
    float c = 0.0f;
    short byteregs[4];
    unsigned short size;

    if (!o || !IS_SYMOP(o)) return 0.0f;

    size_t oe;
    size_t os = cfg_operands_equal_range(&G->node[i], OP_SYMBOL_CONST(o)->key, &oe);
    if (os >= oe) return 0.0f;

    short v = G->node[i].operands[os].var;
    if (sss_contains(&a->local, v)) {
        /* In registers. */
        c += 1.0f;
        byteregs[I->node[v].byte] = a->global[v];
        size = 1;

        size_t p;
        for (p = os + 1; p < oe; p++) {
            v = G->node[i].operands[p].var;
            c += (sss_contains(&a->local, v) ? 1.0f : RA_INF);
            byteregs[I->node[v].byte] = a->global[v];
            size++;
        }

        if ((size == 2 || size == 4) &&
            (byteregs[1] != byteregs[0] + 1 ||
             (byteregs[0] != REG_C && byteregs[0] != REG_E && byteregs[0] != REG_L)))
            c += 2.0f;
        if (size == 4 &&
            (byteregs[3] != byteregs[2] + 1 ||
             (byteregs[2] != REG_C && byteregs[2] != REG_E && byteregs[0] != REG_L)))
            c += 2.0f;

        /* Code generator cannot handle variables only partially in A. */
        if (size > 1) {
            unsigned short k;
            for (k = 0; k < size; k++)
                if (byteregs[k] == REG_A) c += RA_INF;
        }

        if (byteregs[0] == REG_A) c -= 0.4f;
        else if (OPTRALLOC_HL && byteregs[0] == REG_L) c -= 0.1f;
        else if ((OPTRALLOC_IY && byteregs[0] == REG_IYL) || byteregs[0] == REG_IYH)
            c += 0.1f;
    } else {
        /* Spilt. */
        c += OP_SYMBOL_CONST(o)->remat ? 1.5f : 4.0f;
        size_t p;
        for (p = os + 1; p < oe; p++) {
            v = G->node[i].operands[p].var;
            c += (sss_contains(&a->local, v) ? RA_INF : 4.0f);
        }
    }

    return c;
}

static int operand_sane(const operand *o, const assignment_t *a,
                        unsigned short i, const cfg_ralloc_t *G,
                        const con_t *I) {
    (void)I;
    if (!o || !IS_SYMOP(o)) return 1;
    size_t oe;
    size_t os = cfg_operands_equal_range(&G->node[i], OP_SYMBOL_CONST(o)->key, &oe);
    if (os >= oe) return 1;
    int first_in = sss_contains(&a->local, G->node[i].operands[os].var);
    size_t p;
    for (p = os + 1; p < oe; p++) {
        int p_in = sss_contains(&a->local, G->node[i].operands[p].var);
        if (first_in && !p_in) return 0;
        if (!first_in && p_in) return 0;
    }
    return 1;
}

static float default_instruction_cost(const assignment_t *a, unsigned short i,
                                      const cfg_ralloc_t *G, const con_t *I) {
    const iCode *ic = G->node[i].ic;
    float c = 0.0f;
    c += default_operand_cost(IC_RESULT(ic), a, i, G, I);
    c += default_operand_cost(IC_LEFT(ic), a, i, G, I);
    c += default_operand_cost(IC_RIGHT(ic), a, i, G, I);
    return c;
}

static int inst_sane(const assignment_t *a, unsigned short i,
                     const cfg_ralloc_t *G, const con_t *I) {
    const iCode *ic = G->node[i].ic;
    if (ic->op == SEND && ic->builtinSEND && ic->next && ic->next->op == SEND) {
        unsigned int nbr = G->g.out[i].dst[0];
        if (!inst_sane(a, (unsigned short)nbr, G, I)) return 0;
    }
    return operand_sane(IC_RESULT(ic), a, i, G, I) &&
           operand_sane(IC_LEFT(ic), a, i, G, I) &&
           operand_sane(IC_RIGHT(ic), a, i, G, I);
}

/* -------------------- assign_cost (coalescing) -------------------- */

static float assign_cost(const assignment_t *a, unsigned short i,
                         const cfg_ralloc_t *G, const con_t *I) {
    float c = 0.0f;
    const iCode *ic = G->node[i].ic;
    const operand *right = IC_RIGHT(ic);
    const operand *result = IC_RESULT(ic);

    if (!right || !IS_SYMOP(right) || !result || !IS_SYMOP(result) ||
        POINTER_GET(ic) || POINTER_SET(ic))
        return default_instruction_cost(a, i, G, I);

    reg_t byteregs[4] = {-1, -1, -1, -1};
    int size1 = 0, size2 = 0;
    size_t oe;
    size_t os = cfg_operands_equal_range(&G->node[i], OP_SYMBOL_CONST(right)->key, &oe);
    if (os < oe) {
        short v = G->node[i].operands[os].var;
        if (!sss_contains(&a->local, v))
            return default_instruction_cost(a, i, G, I);
        c += 1.0f;
        byteregs[I->node[v].byte] = a->global[v];
        size1 = 1;
        size_t p;
        for (p = os + 1; p < oe; p++) {
            v = G->node[i].operands[p].var;
            c += (sss_contains(&a->local, v) ? 1.0f : RA_INF);
            byteregs[I->node[v].byte] = a->global[v];
            size1++;
        }
        if (size1 > 1) {
            int k;
            for (k = 0; k < size1; k++)
                if (byteregs[k] == REG_A) c += RA_INF;
        }
        if (byteregs[0] == REG_A) c -= 0.4f;
        else if ((OPTRALLOC_IY && byteregs[0] == REG_IYL) || byteregs[0] == REG_IYH)
            c += 0.1f;
    }
    if (!size1) return default_instruction_cost(a, i, G, I);

    os = cfg_operands_equal_range(&G->node[i], OP_SYMBOL_CONST(result)->key, &oe);
    if (os < oe) {
        short v = G->node[i].operands[os].var;
        if (!sss_contains(&a->local, v))
            return default_instruction_cost(a, i, G, I);
        c += 1.0f;
        if (byteregs[I->node[v].byte] == a->global[v]) c -= 2.0f;
        size2 = 1;
        size_t p;
        for (p = os + 1; p < oe; p++) {
            v = G->node[i].operands[p].var;
            c += (sss_contains(&a->local, v) ? 1.0f : RA_INF);
            if (byteregs[I->node[v].byte] == a->global[v]) c -= 2.0f;
            size2++;
        }
        if (byteregs[0] == REG_A) c -= 0.4f;
        else if ((OPTRALLOC_IY && byteregs[0] == REG_IYL) || byteregs[0] == REG_IYH)
            c += 0.1f;
    }
    if (!size2) return default_instruction_cost(a, i, G, I);

    return c;
}

/* -------------------- add_operand_conflicts_in_node -------------------- */

void ralloc_add_operand_conflicts_in_node(const cfg_node_t *n, con_t *I) {
    const iCode *ic = n->ic;
    const operand *result = IC_RESULT(ic);
    const operand *left = IC_LEFT(ic);
    const operand *right = IC_RIGHT(ic);

    if (!result || !IS_SYMOP(result)) return;
    if (!(ic->op == '~' || ic->op == UNARYMINUS || ic->op == '+' ||
          ic->op == '-' || ic->op == '^' || ic->op == '|' ||
          ic->op == BITWISEAND))
        return;

    size_t re, rs = cfg_operands_equal_range(n, OP_SYMBOL_CONST(result)->key, &re);
    if (rs >= re) return;

    if (left && IS_SYMOP(left)) {
        size_t oe, os = cfg_operands_equal_range(n, OP_SYMBOL_CONST(left)->key, &oe);
        size_t p, q;
        for (p = os; p < oe; p++) {
            for (q = rs; q < re; q++) {
                short rvar = n->operands[q].var;
                short ovar = n->operands[p].var;
                if (I->node[rvar].byte < I->node[ovar].byte) {
                    if (!cg_has_edge(&I->g, (unsigned int)rvar, (unsigned int)ovar))
                        cg_add_edge(&I->g, (unsigned int)rvar, (unsigned int)ovar, 0.0f);
                }
            }
        }
    }

    if (right && IS_SYMOP(right)) {
        size_t oe, os = cfg_operands_equal_range(n, OP_SYMBOL_CONST(right)->key, &oe);
        size_t p, q;
        for (p = os; p < oe; p++) {
            for (q = rs; q < re; q++) {
                short rvar = n->operands[q].var;
                short ovar = n->operands[p].var;
                if (I->node[rvar].byte < I->node[ovar].byte) {
                    if (!cg_has_edge(&I->g, (unsigned int)rvar, (unsigned int)ovar))
                        cg_add_edge(&I->g, (unsigned int)rvar, (unsigned int)ovar, 0.0f);
                }
            }
        }
    }
}

/* -------------------- operand_in_reg / operand_on_stack / operand_is_pair -------------------- */

static int operand_in_reg_r(const operand *o, reg_t r, const i_assignment_t *ia,
                            unsigned short i, const cfg_ralloc_t *G) {
    if (!o || !IS_SYMOP(o)) return 0;
    if (r >= port->num_regs) return 0;
    size_t oe, os = cfg_operands_equal_range(&G->node[i], OP_SYMBOL_CONST(o)->key, &oe);
    size_t p;
    for (p = os; p < oe; p++) {
        short v = G->node[i].operands[p].var;
        if (v == ia->registers[r][1] || v == ia->registers[r][0]) return 1;
    }
    return 0;
}

static int operand_in_reg_any(const operand *o, const i_assignment_t *ia,
                              unsigned short i, const cfg_ralloc_t *G) {
    if (!o || !IS_SYMOP(o)) return 0;
    size_t oe, os = cfg_operands_equal_range(&G->node[i], OP_SYMBOL_CONST(o)->key, &oe);
    size_t p;
    for (p = os; p < oe; p++) {
        short v = G->node[i].operands[p].var;
        reg_t r;
        for (r = 0; r < port->num_regs; r++)
            if (v == ia->registers[r][1] || v == ia->registers[r][0]) return 1;
    }
    return 0;
}

static int operand_on_stack(const operand *o, const assignment_t *a,
                            unsigned short i, const cfg_ralloc_t *G) {
    if (!o || !IS_SYMOP(o)) return 0;
    if (OP_SYMBOL_CONST(o)->remat) return 0;
    if (OP_SYMBOL_CONST(o)->_isparm && !IS_REGPARM(OP_SYMBOL_CONST(o)->etype))
        return 1;
    size_t oe, os = cfg_operands_equal_range(&G->node[i], OP_SYMBOL_CONST(o)->key, &oe);
    size_t p;
    for (p = os; p < oe; p++) {
        short v = G->node[i].operands[p].var;
        if (a->global[v] < 0) return 1;
    }
    return 0;
}

static int operand_is_pair(const operand *o, const assignment_t *a,
                           unsigned short i, const cfg_ralloc_t *G) {
    if (!o || !IS_SYMOP(o)) return 0;
    size_t oe, os = cfg_operands_equal_range(&G->node[i], OP_SYMBOL_CONST(o)->key, &oe);
    if (os >= oe) return 0;
    if (os + 1 >= oe) return 0; /* need exactly 2 bytes */
    if (os + 2 < oe) return 0;  /* but not more */
    short v0 = G->node[i].operands[os].var;
    short v1 = G->node[i].operands[os + 1].var;
    if (a->global[v0] % 2) return 0;
    if (a->global[v0] + 1 != a->global[v1]) return 0;
    return 1;
}

/* -------------------- Ainst_ok -------------------- */

static int Ainst_ok(const assignment_t *a, unsigned short i,
                    const cfg_ralloc_t *G, const con_t *I) {
    const iCode *ic = G->node[i].ic;
    const i_assignment_t *ia = &a->i_assignment;
    const operand *left = IC_LEFT(ic);
    const operand *right = IC_RIGHT(ic);
    const operand *result = IC_RESULT(ic);

    if (ia->registers[REG_A][1] < 0) return 1; /* A not in use */

    bool exstk = (should_omit_frame_ptr || (currFunc && currFunc->stack > 127) || IS_GB);

    if (I->node[ia->registers[REG_A][1]].size > 1 ||
        (ia->registers[REG_A][0] >= 0 && I->node[ia->registers[REG_A][0]].size > 1))
        return 0;

    int result_in_A = operand_in_reg_r(result, REG_A, ia, i, G);
    int input_in_A;
    switch (ic->op) {
    case IFX:       input_in_A = operand_in_reg_r(IC_COND(ic),    REG_A, ia, i, G); break;
    case JUMPTABLE: input_in_A = operand_in_reg_r(IC_JTCOND(ic),  REG_A, ia, i, G); break;
    default:
        input_in_A = operand_in_reg_r(left, REG_A, ia, i, G) ||
                     operand_in_reg_r(right, REG_A, ia, i, G);
        break;
    }

    /* Bit instructions don't disturb A. */
    if (ic->op == BITWISEAND && ifxForOp(IC_RESULT(ic), ic) &&
        ((IS_OP_LITERAL(left) &&
          (!((IS_GB && IS_TRUE_SYMOP(right)) ||
             (exstk && operand_on_stack(right, a, i, G))) ||
           (operand_in_reg_any(right, ia, i, G) &&
            !operand_in_reg_r(right, REG_IYL, ia, i, G) &&
            !operand_in_reg_r(right, REG_IYH, ia, i, G)))) ||
         (IS_OP_LITERAL(right) &&
          (!((IS_GB && IS_TRUE_SYMOP(left)) ||
             (exstk && operand_on_stack(left, a, i, G))) ||
           (operand_in_reg_any(left, ia, i, G) &&
            !operand_in_reg_r(left, REG_IYL, ia, i, G) &&
            !operand_in_reg_r(left, REG_IYH, ia, i, G)))))) {
        operand *litop = IS_OP_LITERAL(left) ? IC_LEFT(ic) : IC_RIGHT(ic);
        unsigned int ix;
        int ok = 1;
        for (ix = 0; ix < getSize(operandType(result)); ix++) {
            unsigned char byte = (ulFromVal(OP_VALUE(litop)) >> (ix * 8)) & 0xff;
            if (byte != 0x00 && byte != 0x01 && byte != 0x02 && byte != 0x04 &&
                byte != 0x08 && byte != 0x10 && byte != 0x20 && byte != 0x40 &&
                byte != 0x80) { ok = 0; break; }
        }
        if (ok) return 1;
    }

    const sssset_t *dying = &G->node[i].dying;

    if (ic->op == GET_VALUE_AT_ADDRESS)
        return (result_in_A || !IS_BITVAR(getSpec(operandType(result))));
    if (ic->op == '=' && POINTER_SET(ic))
        return (sss_contains(dying, ia->registers[REG_A][1]) ||
                sss_contains(dying, ia->registers[REG_A][0]) ||
                !(IS_BITVAR(getSpec(operandType(result))) ||
                  IS_BITVAR(getSpec(operandType(right)))));

    /* Variable in A is not used by this instruction. */
    if (ic->op == '+' && IS_ITEMP(IC_LEFT(ic)) && IS_ITEMP(IC_RESULT(ic)) &&
        IS_OP_LITERAL(right) && ulFromVal(OP_VALUE(IC_RIGHT(ic))) == 1 &&
        OP_KEY(IC_RESULT(ic)) == OP_KEY(IC_LEFT(ic)))
        return 1;

    if ((ic->op == '=' || ic->op == CAST) && !POINTER_SET(ic) &&
        isOperandEqual(result, right))
        return 1;

    if ((ic->op == '=' || ic->op == CAST) && !POINTER_SET(ic) &&
        !(ic->op == CAST && IS_BOOL(operandType(result))) &&
        (operand_in_reg_r(right, REG_A, ia, i, G) ||
         operand_in_reg_r(right, REG_B, ia, i, G) ||
         operand_in_reg_r(right, REG_C, ia, i, G) ||
         operand_in_reg_r(right, REG_D, ia, i, G) ||
         operand_in_reg_r(right, REG_E, ia, i, G) ||
         operand_in_reg_r(right, REG_H, ia, i, G) ||
         operand_in_reg_r(right, REG_L, ia, i, G)) &&
        (operand_in_reg_r(right, REG_A, ia, i, G) ||
         operand_in_reg_r(result, REG_B, ia, i, G) ||
         operand_in_reg_r(result, REG_C, ia, i, G) ||
         operand_in_reg_r(result, REG_D, ia, i, G) ||
         operand_in_reg_r(result, REG_E, ia, i, G) ||
         operand_in_reg_r(right, REG_H, ia, i, G) ||
         operand_in_reg_r(right, REG_L, ia, i, G)))
        return 1;

    if (ic->op == GOTO || ic->op == LABEL) return 1;

    if (ic->op == IPUSH && getSize(operandType(IC_LEFT(ic))) <= 2 &&
        (operand_in_reg_r(left, REG_A, ia, i, G) ||
         (operand_in_reg_r(left, REG_B, ia, i, G) &&
          (getSize(operandType(left)) < 2 ||
           (operand_in_reg_r(left, REG_C, ia, i, G) &&
            I->node[ia->registers[REG_C][1]].byte == 0))) ||
         (operand_in_reg_r(left, REG_D, ia, i, G) &&
          (getSize(operandType(left)) < 2 ||
           (operand_in_reg_r(left, REG_E, ia, i, G) &&
            I->node[ia->registers[REG_E][1]].byte == 0))) ||
         (operand_in_reg_r(left, REG_H, ia, i, G) &&
          (getSize(operandType(left)) < 2 ||
           (operand_in_reg_r(left, REG_L, ia, i, G) &&
            I->node[ia->registers[REG_L][1]].byte == 0))) ||
         (operand_in_reg_r(left, REG_IYL, ia, i, G) &&
          I->node[ia->registers[REG_IYL][1]].byte == 0 &&
          (getSize(operandType(left)) < 2 ||
           operand_in_reg_r(left, REG_IYH, ia, i, G)))))
        return 1;

    if (!result_in_A && !input_in_A) return 0;

    /* Last use of operand in A. */
    if (input_in_A &&
        (result_in_A || sss_contains(dying, ia->registers[REG_A][1]) ||
         sss_contains(dying, ia->registers[REG_A][0]))) {
        if (ic->op != IFX && ic->op != RETURN &&
            !((ic->op == RIGHT_OP || ic->op == LEFT_OP) &&
              (IS_OP_LITERAL(right) || operand_in_reg_r(right, REG_A, ia, i, G))) &&
            !((ic->op == '=' || ic->op == CAST) &&
              !(IY_RESERVED && POINTER_SET(ic))) &&
            !IS_BITWISE_OP(ic) && !(ic->op == '~') &&
            !(ic->op == '*' &&
              (IS_ITEMP(IC_LEFT(ic)) || IS_OP_LITERAL(IC_LEFT(ic))) &&
              (IS_ITEMP(IC_RIGHT(ic)) || IS_OP_LITERAL(IC_RIGHT(ic)))) &&
            !((ic->op == '-' || ic->op == '+' || ic->op == EQ_OP) &&
              IS_OP_LITERAL(IC_RIGHT(ic))))
            return 0;
    } else if (input_in_A && ic->op != IFX && ic->op != JUMPTABLE) {
        return 0;
    }

    if (result_in_A && !POINTER_GET(ic) && ic->op != '+' && ic->op != '-' &&
        (ic->op != '*' || (!IS_OP_LITERAL(IC_LEFT(ic)) && !IS_OP_LITERAL(right))) &&
        !IS_BITWISE_OP(ic) && ic->op != GET_VALUE_AT_ADDRESS && ic->op != '=' &&
        ic->op != EQ_OP && ic->op != '<' && ic->op != '>' && ic->op != CAST &&
        ic->op != CALL && ic->op != PCALL && ic->op != GETHBIT &&
        !((ic->op == LEFT_OP || ic->op == RIGHT_OP) && IS_OP_LITERAL(right)))
        return 0;

    return 1;
}

/* -------------------- HLinst_ok -------------------- */

static int HLinst_ok(const assignment_t *a, unsigned short i,
                     const cfg_ralloc_t *G, const con_t *I) {
    const iCode *ic = G->node[i].ic;
    bool exstk = (should_omit_frame_ptr || (currFunc && currFunc->stack > 127) || IS_GB);
    const i_assignment_t *ia = &a->i_assignment;
    bool unused_L = (ia->registers[REG_L][1] < 0);
    bool unused_H = (ia->registers[REG_H][1] < 0);
    if (unused_L && unused_H) return 1;

    const operand *left = IC_LEFT(ic);
    const operand *right = IC_RIGHT(ic);
    const operand *result = IC_RESULT(ic);

    bool result_in_L = operand_in_reg_r(result, REG_L, ia, i, G);
    bool result_in_H = operand_in_reg_r(result, REG_H, ia, i, G);
    bool result_in_HL = result_in_L || result_in_H;

    bool input_in_L, input_in_H;
    switch (ic->op) {
    case IFX:
        input_in_L = operand_in_reg_r(IC_COND(ic), REG_L, ia, i, G);
        input_in_H = operand_in_reg_r(IC_COND(ic), REG_L, ia, i, G);
        break;
    case JUMPTABLE:
        input_in_L = operand_in_reg_r(IC_JTCOND(ic), REG_L, ia, i, G);
        input_in_H = operand_in_reg_r(IC_JTCOND(ic), REG_L, ia, i, G);
        break;
    default:
        input_in_L = operand_in_reg_r(left, REG_L, ia, i, G) ||
                     operand_in_reg_r(right, REG_L, ia, i, G);
        input_in_H = operand_in_reg_r(left, REG_H, ia, i, G) ||
                     operand_in_reg_r(right, REG_H, ia, i, G);
        break;
    }
    bool input_in_HL = input_in_L || input_in_H;

    const sssset_t *dying = &G->node[i].dying;
    bool dying_L = result_in_L ||
                   sss_contains(dying, ia->registers[REG_L][1]) ||
                   sss_contains(dying, ia->registers[REG_L][0]);
    bool dying_H = result_in_H ||
                   sss_contains(dying, ia->registers[REG_H][1]) ||
                   sss_contains(dying, ia->registers[REG_H][0]);
    bool result_only_HL = (result_in_L || unused_L || dying_L) &&
                          (result_in_H || unused_H || dying_H);

    if (ic->op == RETURN || ic->op == SEND) return 1;

    if ((IS_GB || IY_RESERVED) && (IS_TRUE_SYMOP(left) || IS_TRUE_SYMOP(right)))
        return 0;
    if ((IS_GB || IY_RESERVED) && IS_TRUE_SYMOP(result) &&
        getSize(operandType(IC_RESULT(ic))) > 2)
        return 0;

    if (result_only_HL && ic->op == PCALL) return 1;

    if (exstk &&
        (operand_on_stack(result, a, i, G) + operand_on_stack(left, a, i, G) +
             operand_on_stack(right, a, i, G) >= 2) &&
        ((result && IS_SYMOP(result) && getSize(operandType(result)) >= 2) ||
         !result_only_HL))
        return 0;
    if (exstk && (operand_on_stack(left, a, i, G) || operand_on_stack(right, a, i, G)) &&
        (ic->op == '>' || ic->op == '<'))
        return 0;

    if (ic->op == '+' && getSize(operandType(result)) == 2 &&
        ((IS_OP_LITERAL(right) && ulFromVal(OP_VALUE(IC_RIGHT(ic))) <= 3) ||
         (IS_OP_LITERAL(left) && ulFromVal(OP_VALUE(IC_LEFT(ic))) <= 3)) &&
        (operand_in_reg_r(result, REG_L, ia, i, G) &&
         I->node[ia->registers[REG_L][1]].byte == 0 &&
         operand_in_reg_r(result, REG_H, ia, i, G)))
        return 1;

    if (ic->op == '+' && getSize(operandType(result)) == 2 &&
        !IS_TRUE_SYMOP(result) &&
        (result_only_HL ||
         (operand_in_reg_r(result, REG_IYL, ia, i, G) &&
          operand_in_reg_r(result, REG_IYH, ia, i, G))) &&
        ((ia->registers[REG_C][1] < 0 && ia->registers[REG_B][1] < 0) ||
         (ia->registers[REG_E][1] < 0 && ia->registers[REG_D][1] < 0)))
        return 1;

    if (ic->op == '+' && getSize(operandType(result)) >= 2 &&
        ((IS_TRUE_SYMOP(result) && !operand_on_stack(result, a, i, G)) ||
         (operand_on_stack(left, a, i, G) ? exstk : IS_TRUE_SYMOP(left)) ||
         (operand_on_stack(right, a, i, G) ? exstk : IS_TRUE_SYMOP(right))))
        return 0;

    if (ic->op == '+' && input_in_HL &&
        (operand_on_stack(result, a, i, G) ? exstk : IS_TRUE_SYMOP(result)))
        return 0;

    if (result_only_HL && !POINTER_SET(ic) &&
        (ic->op == ADDRESS_OF || ic->op == GET_VALUE_AT_ADDRESS ||
         ic->op == '+' || ic->op == '*' || ic->op == '=' || ic->op == CAST))
        return 1;

    if (!exstk && !isOperandInDirSpace(IC_LEFT(ic)) &&
        !isOperandInDirSpace(IC_RIGHT(ic)) && !isOperandInDirSpace(IC_RESULT(ic)) &&
        (ic->op == '-' || ic->op == '<' || ic->op == '>'))
        return 1;

    if (ic->op == LEFT_OP && getSize(operandType(result)) <= 2 &&
        IS_OP_LITERAL(right) && result_only_HL)
        return 1;
    if ((ic->op == LEFT_OP || ic->op == RIGHT_OP) &&
        (!exstk ||
         ((!operand_on_stack(left, a, i, G) || (!input_in_HL && result_only_HL)) &&
          (!operand_on_stack(right, a, i, G) || (!input_in_HL && result_only_HL)) &&
          !operand_on_stack(result, a, i, G))))
        return 1;

    if (result && IS_SYMOP(result) && isOperandInDirSpace(IC_RESULT(ic))) return 0;
    if ((input_in_HL || !result_only_HL) && left && IS_SYMOP(left) &&
        isOperandInDirSpace(IC_LEFT(ic))) return 0;
    if ((input_in_HL || !result_only_HL) && right && IS_SYMOP(right) &&
        isOperandInDirSpace(IC_RIGHT(ic))) return 0;

    if (ic->op == IFX) return 1;
    if (SKIP_IC2(ic)) return 1;
    if (ic->op == IPUSH && input_in_H &&
        (getSize(operandType(IC_LEFT(ic))) <= 2 ||
         (ia->registers[REG_L][1] > 0 &&
          I->node[ia->registers[REG_L][1]].byte == 2 &&
          ia->registers[REG_H][1] > 0 &&
          I->node[ia->registers[REG_H][1]].byte == 3)))
        return 1;
    if (ic->op == IPUSH && ic->next && ic->next->op == CALL) return 1;
    if (ic->op == IPUSH && getSize(operandType(left)) == 2 &&
        ((ia->registers[REG_C][1] < 0 && ia->registers[REG_B][1] < 0) ||
         (ia->registers[REG_E][1] < 0 && ia->registers[REG_D][1] < 0)))
        return 1;
    if (ic->op == IPUSH && getSize(operandType(left)) <= 2 &&
        ((operand_in_reg_r(left, REG_C, ia, i, G) &&
          I->node[ia->registers[REG_C][1]].byte == 0 &&
          (getSize(operandType(left)) < 2 ||
           operand_in_reg_r(left, REG_B, ia, i, G))) ||
         (operand_in_reg_r(left, REG_E, ia, i, G) &&
          I->node[ia->registers[REG_E][1]].byte == 0 &&
          (getSize(operandType(left)) < 2 ||
           operand_in_reg_r(left, REG_D, ia, i, G))) ||
         (operand_in_reg_r(left, REG_IYL, ia, i, G) &&
          I->node[ia->registers[REG_IYL][1]].byte == 0 &&
          (getSize(operandType(left)) < 2 ||
           operand_in_reg_r(left, REG_IYH, ia, i, G)))))
        return 1;
    if (POINTER_GET(ic) && input_in_L && input_in_H &&
        (getSize(operandType(IC_RESULT(ic))) == 1 || !result_in_HL))
        return 1;
    if (!IS_GB && ic->op == ADDRESS_OF &&
        ((operand_in_reg_r(result, REG_IYL, ia, i, G) &&
          ia->registers[REG_IYL][1] > 0 &&
          I->node[ia->registers[REG_IYL][1]].byte == 0 &&
          operand_in_reg_r(result, REG_IYH, ia, i, G)) ||
         (!OP_SYMBOL_CONST(left)->onStack &&
          operand_in_reg_r(result, REG_C, ia, i, G) &&
          ia->registers[REG_C][1] > 0 &&
          I->node[ia->registers[REG_C][1]].byte == 0 &&
          operand_in_reg_r(result, REG_B, ia, i, G)) ||
         (!OP_SYMBOL_CONST(left)->onStack &&
          operand_in_reg_r(result, REG_E, ia, i, G) &&
          ia->registers[REG_E][1] > 0 &&
          I->node[ia->registers[REG_E][1]].byte == 0 &&
          operand_in_reg_r(result, REG_D, ia, i, G))))
        return 1;

    if (ic->op == LEFT_OP && isOperandLiteral(IC_RIGHT(ic))) return 1;

    if (exstk && !result_only_HL &&
        (operand_on_stack(left, a, i, G) || operand_on_stack(right, a, i, G) ||
         operand_on_stack(result, a, i, G)) &&
        ic->op == '+')
        return 0;

    if ((!POINTER_SET(ic) && !POINTER_GET(ic) &&
         (ic->op == '=' || ic->op == CAST || ic->op == UNARYMINUS ||
          ic->op == RIGHT_OP || IS_BITWISE_OP(ic) ||
          (ic->op == '+' && getSize(operandType(IC_RESULT(ic))) == 1) ||
          (ic->op == '+' && getSize(operandType(IC_RESULT(ic))) <= 2 &&
           (result_only_HL || !IS_GB)))))
        return 1;

    if ((ic->op == '<' || ic->op == '>') &&
        (IS_ITEMP(left) || IS_OP_LITERAL(left) || IS_ITEMP(right) ||
         IS_OP_LITERAL(right)))
        return 1;

    if (ic->op == EQ_OP && IS_VALOP(right)) return 1;

    if (ic->op == CALL) return 1;

    if (result_only_HL && ic->op == PCALL) return 1;

    if (POINTER_GET(ic) && getSize(operandType(IC_RESULT(ic))) == 1 &&
        !IS_BITVAR(getSpec(operandType(result))) &&
        ((operand_in_reg_r(right, REG_C, ia, i, G) &&
          I->node[ia->registers[REG_C][1]].byte == 0 &&
          operand_in_reg_r(right, REG_B, ia, i, G)) ||
         (operand_in_reg_r(right, REG_E, ia, i, G) &&
          I->node[ia->registers[REG_E][1]].byte == 0 &&
          operand_in_reg_r(right, REG_D, ia, i, G)) ||
         (operand_in_reg_r(right, REG_IYL, ia, i, G) &&
          I->node[ia->registers[REG_IYL][1]].byte == 0 &&
          operand_in_reg_r(right, REG_IYH, ia, i, G))))
        return 1;

    if ((ic->op == '=') && POINTER_SET(ic) &&
        operand_in_reg_r(result, REG_IYL, ia, i, G) &&
        I->node[ia->registers[REG_IYL][1]].byte == 0 &&
        operand_in_reg_r(result, REG_IYH, ia, i, G))
        return 1;

    if ((ic->op == '=' || ic->op == CAST) && POINTER_SET(ic) && !result_only_HL)
        return 0;

    if ((ic->op == '=' || ic->op == CAST) && !POINTER_GET(ic) && !input_in_HL)
        return 1;

    return 0;
}

/* -------------------- IYinst_ok -------------------- */

static int IYinst_ok(const assignment_t *a, unsigned short i,
                     const cfg_ralloc_t *G, const con_t *I) {
    const iCode *ic = G->node[i].ic;
    const i_assignment_t *ia = &a->i_assignment;
    bool exstk = (should_omit_frame_ptr || (currFunc && currFunc->stack > 127));
    bool unused_IYL = (ia->registers[REG_IYL][1] < 0);
    bool unused_IYH = (ia->registers[REG_IYH][1] < 0);
    const operand *left = IC_LEFT(ic);
    const operand *right = IC_RIGHT(ic);
    const operand *result = IC_RESULT(ic);

    bool result_in_IYL = operand_in_reg_r(result, REG_IYL, ia, i, G);
    bool result_in_IYH = operand_in_reg_r(result, REG_IYH, ia, i, G);
    bool result_in_IY = result_in_IYL || result_in_IYH;

    bool input_in_IYL, input_in_IYH;
    switch (ic->op) {
    case IFX:
        input_in_IYL = operand_in_reg_r(IC_COND(ic), REG_IYL, ia, i, G);
        input_in_IYH = operand_in_reg_r(IC_COND(ic), REG_IYL, ia, i, G);
        break;
    case JUMPTABLE:
        input_in_IYL = operand_in_reg_r(IC_JTCOND(ic), REG_IYL, ia, i, G);
        input_in_IYH = operand_in_reg_r(IC_JTCOND(ic), REG_IYL, ia, i, G);
        break;
    default:
        input_in_IYL = operand_in_reg_r(left, REG_IYL, ia, i, G) ||
                       operand_in_reg_r(right, REG_IYL, ia, i, G);
        input_in_IYH = operand_in_reg_r(left, REG_IYH, ia, i, G) ||
                       operand_in_reg_r(right, REG_IYH, ia, i, G);
        break;
    }
    bool input_in_IY = input_in_IYL || input_in_IYH;

    if (unused_IYL && unused_IYH) return 1;

    if (exstk && (operand_on_stack(result, a, i, G) ||
                  operand_on_stack(left, a, i, G) ||
                  operand_on_stack(right, a, i, G)))
        return 0;

    if (unused_IYL ^ unused_IYH) return 0;
    if ((!unused_IYL && I->node[ia->registers[REG_IYL][1]].size != 2) ||
        (!unused_IYH && I->node[ia->registers[REG_IYH][1]].size != 2) ||
        (ia->registers[REG_IYL][0] >= 0 && I->node[ia->registers[REG_IYL][0]].size != 2) ||
        (ia->registers[REG_IYH][0] >= 0 && I->node[ia->registers[REG_IYH][0]].size != 2))
        return 0;
    if (ia->registers[REG_IYL][1] >= 0 &&
        (ia->registers[REG_IYH][1] <= 0 ||
         I->node[ia->registers[REG_IYL][1]].v != I->node[ia->registers[REG_IYH][1]].v))
        return 0;
    if (ia->registers[REG_IYH][1] >= 0 &&
        (ia->registers[REG_IYL][1] <= 0 ||
         I->node[ia->registers[REG_IYH][1]].v != I->node[ia->registers[REG_IYL][1]].v))
        return 0;
    if (ia->registers[REG_IYL][0] >= 0 &&
        (ia->registers[REG_IYH][0] <= 0 ||
         I->node[ia->registers[REG_IYL][0]].v != I->node[ia->registers[REG_IYH][0]].v))
        return 0;
    if (ia->registers[REG_IYH][0] >= 0 &&
        (ia->registers[REG_IYL][0] <= 0 ||
         I->node[ia->registers[REG_IYH][0]].v != I->node[ia->registers[REG_IYL][0]].v))
        return 0;
    if (I->node[ia->registers[REG_IYL][1]].byte != 0 ||
        I->node[ia->registers[REG_IYH][1]].byte != 1)
        return 0;
    if ((ia->registers[REG_IYL][0] >= 0 && I->node[ia->registers[REG_IYL][0]].byte != 0) ||
        (ia->registers[REG_IYH][0] >= 0 && I->node[ia->registers[REG_IYH][0]].byte != 1))
        return 0;

    if (result_in_IY &&
        (ic->op == '=' ||
         (ic->op == CAST && getSize(operandType(IC_RESULT(ic))) <=
                                  getSize(operandType(IC_RIGHT(ic)))) ||
         ic->op == '+'))
        return 1;

    if (ic->op == LEFT_OP && result_in_IY && input_in_IY &&
        IS_VALOP(IC_RIGHT(ic)) && operandLitValue(IC_RIGHT(ic)) < 8)
        return 1;

    if (ic->op == '-' && result_in_IY && input_in_IY && IS_VALOP(IC_RIGHT(ic)) &&
        operandLitValue(IC_RIGHT(ic)) < 4)
        return 1;

    if (SKIP_IC2(ic)) return 1;

    if (!result_in_IY && !input_in_IY &&
        !(IC_RESULT(ic) && isOperandInDirSpace(IC_RESULT(ic))) &&
        !(IC_RIGHT(ic) && IS_TRUE_SYMOP(IC_RIGHT(ic))) &&
        !(IC_LEFT(ic) && IS_TRUE_SYMOP(IC_LEFT(ic))))
        return 1;

    if (!result_in_IY && !input_in_IY &&
        (ic->op == '=' ||
         (ic->op == CAST && getSize(operandType(IC_RIGHT(ic))) >= 2 &&
          (getSize(operandType(IC_RESULT(ic))) <= getSize(operandType(IC_RIGHT(ic))) ||
           !IS_SPEC(operandType(IC_RIGHT(ic))) || SPEC_USIGN(operandType(IC_RIGHT(ic)))))) &&
        operand_is_pair(IC_RESULT(ic), a, i, G))
        return 1;

    if (ic->op == IPUSH) return 1;

    if (ic->op == GET_VALUE_AT_ADDRESS && isOperandInDirSpace(IC_RESULT(ic)))
        return 0;

    if (input_in_IY && !result_in_IY &&
        ((ic->op == '=' && !POINTER_SET(ic)) ||
         (ic->op == CAST && getSize(operandType(IC_RESULT(ic))) <=
                                    getSize(operandType(IC_RIGHT(ic)))) ||
         ic->op == GET_VALUE_AT_ADDRESS))
        return 1;

    return 0;
}

/* -------------------- DEinst_ok -------------------- */

static int DEinst_ok(const assignment_t *a, unsigned short i,
                     const cfg_ralloc_t *G, const con_t *I) {
    (void)I;
    if (!IS_GB) return 1;
    const i_assignment_t *ia = &a->i_assignment;
    bool unused_E = (ia->registers[REG_E][1] < 0);
    bool unused_D = (ia->registers[REG_D][1] < 0);
    if (unused_E && unused_D) return 1;

    const iCode *ic = G->node[i].ic;
    const operand *left = IC_LEFT(ic);
    const operand *right = IC_RIGHT(ic);
    const operand *result = IC_RESULT(ic);

    if (ic->op == PCALL) return 0;
    if (ic->op == GET_VALUE_AT_ADDRESS &&
        (getSize(operandType(result)) >= 2 || !operand_is_pair(left, a, i, G)))
        return 0;
    if (ic->op == '=' && POINTER_SET(ic) && !operand_is_pair(result, a, i, G))
        return 0;
    if ((ic->op == '=' || ic->op == CAST) && getSize(operandType(result)) >= 2 &&
        (operand_on_stack(right, a, i, G) ||
         operand_in_reg_r(right, REG_L, ia, i, G) ||
         operand_in_reg_r(right, REG_H, ia, i, G)) &&
        (operand_on_stack(result, a, i, G) ||
         operand_in_reg_r(result, REG_L, ia, i, G) ||
         operand_in_reg_r(result, REG_H, ia, i, G)))
        return 0;
    if (ic->op == '+' && getSize(operandType(result)) >= 2) return 0;
    if (ic->op == UNARYMINUS || ic->op == '-' || ic->op == '*') return 0;
    if (ic->op == '>' || ic->op == '<') return 0;
    return 1;
}

/* -------------------- set_surviving_regs / unset -------------------- */

static void set_surviving_regs(const assignment_t *a, unsigned short i,
                               const cfg_ralloc_t *G, const con_t *I) {
    iCode *ic = G->node[i].ic;
    ic->rMask = newBitVect(port->num_regs);
    ic->rSurv = newBitVect(port->num_regs);

    size_t vi;
    for (vi = 0; vi < G->node[i].alive.n; vi++) {
        short v = G->node[i].alive.items[vi];
        if (a->global[v] < 0) continue;
        ic->rMask = bitVectSetBit(ic->rMask, a->global[v]);
        if (!sss_contains(&G->node[i].dying, v)) {
            int owned_result =
                IC_RESULT(ic) && !POINTER_SET(ic) && IS_SYMOP(IC_RESULT(ic)) &&
                OP_SYMBOL_CONST(IC_RESULT(ic))->key == I->node[v].v;
            if (!owned_result)
                ic->rSurv = bitVectSetBit(ic->rSurv, a->global[v]);
        }
    }
}

static void unset_surviving_regs(unsigned short i, const cfg_ralloc_t *G) {
    iCode *ic = G->node[i].ic;
    freeBitVect(ic->rSurv);
    freeBitVect(ic->rMask);
}

/* -------------------- assign_operand_for_cost -------------------- */

static void assign_operand_for_cost(operand *o, const assignment_t *a,
                                    unsigned short i, const cfg_ralloc_t *G,
                                    const con_t *I) {
    if (!o || !IS_SYMOP(o)) return;
    symbol *sym = OP_SYMBOL(o);
    size_t oe, os = cfg_operands_equal_range(&G->node[i], OP_SYMBOL_CONST(o)->key, &oe);
    size_t p;
    for (p = os; p < oe; p++) {
        short v = G->node[i].operands[p].var;
        if (a->global[v] >= 0) {
            if (a->global[v] != REG_A &&
                ((a->global[v] != REG_IYL && a->global[v] != REG_IYH) || !OPTRALLOC_IY)) {
                sym->regs[I->node[v].byte] = regsZ80 + a->global[v];
                sym->accuse = 0;
                sym->isspilt = false;
                sym->nRegs = I->node[v].size;
            } else if (a->global[v] == REG_A) {
                sym->accuse = ACCUSE_A;
                sym->isspilt = false;
                sym->nRegs = 0;
                sym->regs[I->node[v].byte] = 0;
            } else {
                sym->accuse = ACCUSE_IY;
                sym->isspilt = false;
                sym->nRegs = 0;
                sym->regs[I->node[v].byte] = 0;
            }
        } else {
            sym->isspilt = true;
            sym->accuse = 0;
            sym->nRegs = I->node[v].size;
            sym->regs[I->node[v].byte] = 0;
        }
    }
}

static void assign_operands_for_cost(const assignment_t *a, unsigned short i,
                                     const cfg_ralloc_t *G, const con_t *I) {
    const iCode *ic = G->node[i].ic;
    if (ic->op == IFX)
        assign_operand_for_cost(IC_COND(ic), a, i, G, I);
    else if (ic->op == JUMPTABLE)
        assign_operand_for_cost(IC_JTCOND(ic), a, i, G, I);
    else {
        assign_operand_for_cost(IC_LEFT(ic), a, i, G, I);
        assign_operand_for_cost(IC_RIGHT(ic), a, i, G, I);
        assign_operand_for_cost(IC_RESULT(ic), a, i, G, I);
    }
    if (ic->op == SEND && ic->builtinSEND) {
        unsigned int nx = G->g.out[i].dst[0];
        assign_operands_for_cost(a, (unsigned short)nx, G, I);
    }
}

/* -------------------- ralloc_instruction_cost (public hook) -------------------- */

float ralloc_instruction_cost(const assignment_t *a, unsigned int i,
                              const cfg_ralloc_t *G, const con_t *I) {
    iCode *ic = G->node[i].ic;
    float c;

    if (!inst_sane(a, (unsigned short)i, G, I)) return RA_INF;
    if (ic->generated) return 0.0f;
    if (!Ainst_ok(a, (unsigned short)i, G, I)) return RA_INF;
    if (OPTRALLOC_HL && !HLinst_ok(a, (unsigned short)i, G, I)) return RA_INF;
    if (!DEinst_ok(a, (unsigned short)i, G, I)) return RA_INF;
    if (OPTRALLOC_IY && !IYinst_ok(a, (unsigned short)i, G, I)) return RA_INF;

    switch (ic->op) {
    case FUNCTION:
    case ENDFUNCTION:
    case LABEL:
    case GOTO:
    case INLINEASM:
        return 0.0f;
    case '!': case '~': case UNARYMINUS:
    case '+': case '-': case '^': case '|': case BITWISEAND:
    case IPUSH: case CALL: case PCALL: case RETURN:
    case '*': case '>': case '<':
    case EQ_OP: case AND_OP: case OR_OP:
    case GETHBIT: case LEFT_OP: case RIGHT_OP:
    case GET_VALUE_AT_ADDRESS: case '=': case IFX:
    case ADDRESS_OF: case JUMPTABLE: case CAST:
    case SEND: case DUMMY_READ_VOLATILE:
    case CRITICAL: case ENDCRITICAL:
        assign_operands_for_cost(a, (unsigned short)i, G, I);
        set_surviving_regs(a, (unsigned short)i, G, I);
        c = dryZ80iCode(ic);
        unset_surviving_regs((unsigned short)i, G);
        ic->generated = false;
        return c;
    default:
        return default_instruction_cost(a, (unsigned short)i, G, I);
    }
    (void)assign_cost; /* currently unused but kept for parity with original */
}

/* -------------------- weird_byte_order / local_assignment_insane -------------------- */

static float weird_byte_order(const assignment_t *a, const con_t *I) {
    float c = 0.0f;
    size_t vi;
    for (vi = 0; vi < a->local.n; vi++) {
        short v = a->local.items[vi];
        if (a->global[v] % 2 != I->node[v].byte % 2) c += 8.0f;
    }
    return c;
}

static int local_assignment_insane(const assignment_t *a, const con_t *I,
                                   short lastvar) {
    size_t i;
    for (i = 0; i < a->local.n;) {
        short v_old = a->local.items[i];
        i++;
        if (i == a->local.n) {
            if (v_old != lastvar && I->node[v_old].byte != I->node[v_old].size - 1)
                return 1;
            break;
        }
        short v_cur = a->local.items[i];
        if (I->node[v_old].v == I->node[v_cur].v) {
            if (I->node[v_old].byte != I->node[v_cur].byte - 1) return 1;
        } else {
            if ((v_old != lastvar && I->node[v_old].byte != I->node[v_old].size - 1) ||
                I->node[v_cur].byte)
                return 1;
        }
    }
    return 0;
}

int ralloc_assignment_hopeless(const assignment_t *a, unsigned int i,
                               const cfg_ralloc_t *G, const con_t *I,
                               short lastvar) {
    if (!G->node[i].ic->generated && !Ainst_ok(a, (unsigned short)i, G, I))
        return 1;
    if (local_assignment_insane(a, I, lastvar)) return 1;

    const i_assignment_t *ia = &a->i_assignment;

    if (OPTRALLOC_IY &&
        ((ia->registers[REG_IYL][1] >= 0 &&
          (I->node[ia->registers[REG_IYL][1]].size != 2 ||
           I->node[ia->registers[REG_IYL][1]].byte != 0)) ||
         (ia->registers[REG_IYH][1] >= 0 &&
          (I->node[ia->registers[REG_IYH][1]].size != 2 ||
           I->node[ia->registers[REG_IYH][1]].byte != 1)) ||
         (ia->registers[REG_IYL][0] >= 0 &&
          (I->node[ia->registers[REG_IYL][0]].size != 2 ||
           I->node[ia->registers[REG_IYL][0]].byte != 0)) ||
         (ia->registers[REG_IYH][0] >= 0 &&
          (I->node[ia->registers[REG_IYH][0]].size != 2 ||
           I->node[ia->registers[REG_IYH][0]].byte != 1))))
        return 1;

    if (OPTRALLOC_HL &&
        (ia->registers[REG_L][1] >= 0 && ia->registers[REG_H][1] >= 0) &&
        (ia->registers[REG_L][0] >= 0 && ia->registers[REG_H][0] >= 0) &&
        !HLinst_ok(a, (unsigned short)i, G, I))
        return 1;

    if (OPTRALLOC_IY &&
        (ia->registers[REG_IYL][1] >= 0 && ia->registers[REG_IYH][1] >= 0) &&
        !((ia->registers[REG_IYL][0] >= 0) ^ (ia->registers[REG_IYH][0] >= 0)) &&
        !IYinst_ok(a, (unsigned short)i, G, I))
        return 1;

    return 0;
}

/* -------------------- ralloc_get_best_local_assignment_biased -------------------- */

void ralloc_get_best_local_assignment_biased(assignment_t *out, unsigned int t,
                                             const tree_dec_ralloc_t *T) {
    const assignment_node_t *best = T->node[t].alist_head;
    const assignment_node_t *an;
    for (an = T->node[t].alist_head; an; an = an->next) {
        if (an->a.s < best->a.s) {
            int risky = 0;
            size_t vi;
            for (vi = 0; vi < an->a.local.n; vi++) {
                short v = an->a.local.items[vi];
                signed char r = an->a.global[v];
                if (r == REG_A ||
                    (OPTRALLOC_HL && (r == REG_H || r == REG_L)) ||
                    (OPTRALLOC_IY && (r == REG_IYH || r == REG_IYL))) {
                    risky = 1; break;
                }
            }
            if (!risky) best = an;
        }
    }
    if (!best) return;
    assignment_copy(out, &best->a);
    size_t vi;
    for (vi = 0; vi < T->node[t].alive.n; vi++)
        sss_insert(&out->local, T->node[t].alive.items[vi]);
}

/* -------------------- ralloc_rough_cost_estimate -------------------- */

float ralloc_rough_cost_estimate(const assignment_t *a, unsigned int i,
                                 const cfg_ralloc_t *G, const con_t *I) {
    const i_assignment_t *ia = &a->i_assignment;
    float c = 0.0f;

    c += weird_byte_order(a, I);

    if (OPTRALLOC_HL &&
        (ia->registers[REG_L][1] >= 0 && ia->registers[REG_H][1] >= 0) &&
        !((ia->registers[REG_L][0] >= 0) ^ (ia->registers[REG_H][0] >= 0)) &&
        !HLinst_ok(a, (unsigned short)i, G, I))
        c += 8.0f;

    if (ia->registers[REG_A][1] < 0) c += 0.03f;
    if (OPTRALLOC_HL && ia->registers[REG_L][1] < 0) c += 0.02f;

    if (OPTRALLOC_IY) {
        size_t vi;
        for (vi = 0; vi < a->local.n; vi++) {
            short v = a->local.items[vi];
            if (a->global[v] == REG_IYL || a->global[v] == REG_IYH) c += 8.0f;
        }
    }

    if (ia->registers[REG_E][1] < 0) c += 0.0001f;
    if (ia->registers[REG_D][1] < 0) c += 0.00001f;

    if (a->marked) c -= 0.5f;

    size_t vi;
    for (vi = 0; vi < a->local.n; vi++) {
        short v = a->local.items[vi];
        const symbol *sym = (symbol *)hTabItemWithKey(liveRanges, I->node[v].v);
        if (a->global[v] < 0 && IS_REGISTER(sym->type)) c += 32.0f;
        if ((I->node[v].byte % 2) &&
            (a->global[v] == REG_L || a->global[v] == REG_E || a->global[v] == REG_C))
            c += 8.0f;
        if (!(I->node[v].byte % 2) && I->node[v].size > 1 &&
            (a->global[v] == REG_H || a->global[v] == REG_D || a->global[v] == REG_B))
            c += 8.0f;
        if ((I->node[v].byte == 0 && I->node[v].size > 1) ||
            (I->node[v].byte == 2 && I->node[v].size > 3)) {
            if (a->global[v] == REG_L && v + 1 < (short)a->global_n &&
                a->global[v + 1] >= 0 && a->global[v + 1] != REG_H)
                c += 16.0f;
            if (a->global[v] == REG_E && v + 1 < (short)a->global_n &&
                a->global[v + 1] >= 0 && a->global[v + 1] != REG_D)
                c += 16.0f;
            if (a->global[v] == REG_C && v + 1 < (short)a->global_n &&
                a->global[v + 1] >= 0 && a->global[v + 1] != REG_B)
                c += 16.0f;
        } else if (I->node[v].byte == 1 || I->node[v].byte == 3) {
            if (a->global[v] == REG_H && v - 1 >= 0 && a->global[v - 1] >= 0 &&
                a->global[v - 1] != REG_L) c += 16.0f;
            if (a->global[v] == REG_D && v - 1 >= 0 && a->global[v - 1] >= 0 &&
                a->global[v - 1] != REG_E) c += 16.0f;
            if (a->global[v] == REG_B && v - 1 >= 0 && a->global[v - 1] >= 0 &&
                a->global[v - 1] != REG_C) c += 16.0f;
        }
    }

    c -= a->local.n * 0.2f;
    return c;
}

/* -------------------- ralloc_extra_ic_generated -------------------- */

void ralloc_extra_ic_generated(iCode *ic) {
    if (ic->op == '>' || ic->op == '<' || ic->op == LE_OP || ic->op == GE_OP ||
        ic->op == EQ_OP || ic->op == NE_OP ||
        ((ic->op == '^' || ic->op == '|' || ic->op == BITWISEAND) &&
         (IS_OP_LITERAL(IC_LEFT(ic)) || IS_OP_LITERAL(IC_RIGHT(ic))))) {
        iCode *ifx;
        if ((ifx = ifxForOp(IC_RESULT(ic), ic))) {
            OP_SYMBOL(IC_RESULT(ic))->for_newralloc = false;
            OP_SYMBOL(IC_RESULT(ic))->regType = REG_CND;
            ifx->generated = true;
        }
    }

    if (ic->op == SEND && ic->builtinSEND &&
        (!ic->prev || ic->prev->op != SEND || !ic->prev->builtinSEND)) {
        iCode *icn;
        for (icn = ic->next; icn->op != CALL; icn = icn->next)
            icn->generated = true;
        icn->generated = true;
        ic->generated = false;
    }
}

/* -------------------- omit_frame_ptr / move_parms -------------------- */

static bool omit_frame_ptr(const cfg_ralloc_t *G) {
    if (IS_GB || IY_RESERVED || z80_opts.noOmitFramePtr) return false;
    if (options.omitFramePtr) return true;

    signed char omitcost = -16;
    size_t ncfg = cg_num_vertices(&G->g);
    unsigned int i;
    for (i = 0; i < ncfg; i++) {
        if ((int)G->node[i].alive.n > port->num_regs - 4) return false;
        const iCode *ic = G->node[i].ic;
        const operand *o;
        o = IC_RESULT(ic);
        if (o && IS_SYMOP(o) && OP_SYMBOL_CONST(o)->_isparm &&
            !IS_REGPARM(OP_SYMBOL_CONST(o)->etype)) omitcost += 6;
        o = IC_LEFT(ic);
        if (o && IS_SYMOP(o) && OP_SYMBOL_CONST(o)->_isparm &&
            !IS_REGPARM(OP_SYMBOL_CONST(o)->etype)) omitcost += 6;
        o = IC_RIGHT(ic);
        if (o && IS_SYMOP(o) && OP_SYMBOL_CONST(o)->_isparm &&
            !IS_REGPARM(OP_SYMBOL_CONST(o)->etype)) omitcost += 6;
        if (omitcost > 14) return false;
    }
    return true;
}

static void move_parms(void) {
    if (!currFunc || IS_GB || options.omitFramePtr || !should_omit_frame_ptr) return;
    value *val;
    for (val = FUNC_ARGS(currFunc->type); val; val = val->next) {
        if (IS_REGPARM(val->sym->etype) || !val->sym->onStack) continue;
        val->sym->stack -= 2;
    }
}

/* -------------------- z80_ralloc2_cc (public entry) -------------------- */

iCode *z80_ralloc2_cc(ebbIndex *ebbi) {
    iCode *ic;
    cfg_ralloc_t cfg;
    con_t conflict_graph;
    tree_dec_ralloc_t T;

    cfg_ralloc_init(&cfg);
    con_init(&conflict_graph);
    tree_dec_ralloc_init(&T);

    ic = ralloc_create_cfg(&cfg, &conflict_graph, ebbi);

    should_omit_frame_ptr = omit_frame_ptr(&cfg);
    move_parms();

    /* Build tree decomposition. We feed it the underlying cgraph. */
    tree_dec_thorup(&T.td, &cfg.g);
    /* Grow the side-array to match tree_dec's vertex count. */
    {
        size_t n = cg_num_vertices(&T.td.g);
        if (T.cap < n) {
            T.node = (tree_dec_ralloc_node_t *)realloc(T.node, n * sizeof(*T.node));
            size_t k;
            for (k = T.cap; k < n; k++) {
                sss_init(&T.node[k].alive);
                T.node[k].alist_head = T.node[k].alist_tail = NULL;
                T.node[k].alist_n = 0;
            }
            T.cap = n;
        }
    }
    tree_dec_nicify(&T.td);
    /* Re-grow side-array after nicify (may have added vertices). */
    {
        size_t n = cg_num_vertices(&T.td.g);
        if (T.cap < n) {
            T.node = (tree_dec_ralloc_node_t *)realloc(T.node, n * sizeof(*T.node));
            size_t k;
            for (k = T.cap; k < n; k++) {
                sss_init(&T.node[k].alive);
                T.node[k].alist_head = T.node[k].alist_tail = NULL;
                T.node[k].alist_n = 0;
            }
            T.cap = n;
        }
    }

    ralloc_alive_tree_dec(&T, &cfg);
    ralloc_good_re_root(&T);
    tree_dec_nicify(&T.td);
    {
        size_t n = cg_num_vertices(&T.td.g);
        if (T.cap < n) {
            T.node = (tree_dec_ralloc_node_t *)realloc(T.node, n * sizeof(*T.node));
            size_t k;
            for (k = T.cap; k < n; k++) {
                sss_init(&T.node[k].alive);
                T.node[k].alist_head = T.node[k].alist_tail = NULL;
                T.node[k].alist_n = 0;
            }
            T.cap = n;
        }
    }
    ralloc_alive_tree_dec(&T, &cfg);

    assignment_t winner;
    assignment_init(&winner);
    int not_optimal = ralloc_tree_dec_ralloc(&T, &cfg, &conflict_graph, &winner);
    z80_assignment_optimal = !not_optimal;

    /* Apply winner to symbols. */
    {
        size_t ncon = cg_num_vertices(&conflict_graph.g);
        unsigned int v;
        for (v = 0; v < ncon; v++) {
            symbol *sym = (symbol *)hTabItemWithKey(liveRanges, conflict_graph.node[v].v);
            if (winner.global[v] >= 0) {
                if (winner.global[v] != REG_A &&
                    ((winner.global[v] != REG_IYL && winner.global[v] != REG_IYH) ||
                     !OPTRALLOC_IY)) {
                    sym->regs[conflict_graph.node[v].byte] = regsZ80 + winner.global[v];
                    sym->accuse = 0;
                    sym->isspilt = false;
                    sym->nRegs = conflict_graph.node[v].size;
                } else if (winner.global[v] == REG_A) {
                    sym->accuse = ACCUSE_A;
                    sym->isspilt = false;
                    sym->nRegs = 0;
                    sym->regs[0] = 0;
                } else {
                    sym->accuse = ACCUSE_IY;
                    sym->isspilt = false;
                    sym->nRegs = 0;
                    sym->regs[conflict_graph.node[v].byte] = 0;
                }
            } else {
                int k;
                for (k = 0; k < conflict_graph.node[v].size; k++)
                    sym->regs[k] = 0;
                sym->accuse = 0;
                sym->nRegs = conflict_graph.node[v].size;
                sym->isspilt = false;
            }
        }
    }

    {
        size_t ncfg = cg_num_vertices(&cfg.g);
        unsigned int vi;
        for (vi = 0; vi < ncfg; vi++)
            set_surviving_regs(&winner, (unsigned short)vi, &cfg, &conflict_graph);
    }

    assignment_free(&winner);
    tree_dec_ralloc_free(&T);
    con_free(&conflict_graph);
    cfg_ralloc_free(&cfg);

    (void)ra_is_inf;
    return ic;
}
