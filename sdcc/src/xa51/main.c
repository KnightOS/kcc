/** @file main.c
    xa51 specific general functions.

    Note that mlh prepended _xa51_ on the static functions.  Makes
    it easier to set a breakpoint using the debugger.
*/
#include "common.h"
#include "main.h"
#include "ralloc.h"
#include "gen.h"

static char _defaultRules[] =
{
#include "peeph.rul"
};

/* list of key words used by xa51 */
static char *_xa51_keywords[] =
{
  "at",
  "bit",
  "code",
  "critical",
  "data",
  "far",
  //"idata",
  "interrupt",
  "near",
  //"pdata",
  "reentrant",
  "sfr",
  "sbit",
  "using",
  "xdata",
  //"_data",
  //"_code",
  //"_generic",
  //"_near",
  //"_xdata",
  //"_pdata",
  //"_idata",
  "_naked",
  //"_overlay",
  NULL
};

extern int rewinds;
void   _xa51_genAssemblerEnd () {
  //fprintf (stderr, "Did %d rewind%c for c-line in asm comments\n", rewinds,
  //rewinds==1 ? '\0' : 's');
}

void xa51_assignRegisters (eBBlock ** ebbs, int count);

static int regParmFlg = 0;	/* determine if we can register a parameter */

static void
_xa51_init (void)
{
  asm_addTree (&asm_xa_asm_mapping);
}

static void
_xa51_reset_regparm ()
{
  regParmFlg = 0;
}

static int
_xa51_regparm (sym_link * l)
{
  return 0; // for now
  /* for this processor it is simple
     can pass only the first parameter in a register */
  if (regParmFlg)
    return 0;

  regParmFlg = 1;
  return 1;
}

static bool
_xa51_parseOptions (int *pargc, char **argv, int *i)
{
  /* TODO: allow port-specific command line options to specify
   * segment names here.
   */
  return FALSE;
}

static void
_xa51_finaliseOptions (void)
{
  port->mem.default_local_map = istack;
  port->mem.default_globl_map = xdata;
}

static void
_xa51_setDefaultOptions (void)
{
  options.stackAuto=1;
  options.intlong_rent=1;
  options.float_rent=1;
  options.stack_loc=0x100;
}

static const char *
_xa51_getRegName (struct regs *reg)
{
  if (reg)
    return reg->name;
  return "err";
}

/* Generate interrupt vector table. */
static int
_xa51_genIVT (FILE * of, symbol ** interrupts, int maxInterrupts)
{
  return TRUE;
}

/* Generate code to copy XINIT to XISEG */
static void _xa51_genXINIT (FILE * of) {
  fprintf (of, ";	_xa51_genXINIT() start\n");
  fprintf (of, "	mov	r0,#l_XINIT\n");
  fprintf (of, "	beq	00002$\n");
  fprintf (of, "	mov	r1,#s_XINIT\n");
  fprintf (of, "        mov     r2,#s_XISEG\n");
  fprintf (of, "00001$: movc    r3l,[r1+]\n");
  fprintf (of, "        mov     [r2+],r3l\n");
  fprintf (of, "        djnz    r0,00001$\n");
  fprintf (of, "00002$:\n");
  fprintf (of, ";	_xa51_genXINIT() end\n");
}

static void
_xa51_genAssemblerPreamble (FILE * of)
{
  symbol *mainExists=newSymbol("main", 0);
  mainExists->block=0;

  if ((mainExists=findSymWithLevel(SymbolTab, mainExists))) {
    fprintf (of, "\t.area CSEG\t(CODE)\n");
    fprintf (of, "__interrupt_vect:\n");
    fprintf (of, "\t.dw\t0x8f00\n");
    fprintf (of, "\t.dw\t__sdcc_gsinit_startup\n");
    fprintf (of, "\n");
    fprintf (of, "__sdcc_gsinit_startup:\n");
    fprintf (of, "\tmov.b\t_SCR,#0x01\t; page zero mode\n");
    fprintf (of, "\tmov\tr7,#0x%04x\n", options.stack_loc);
    fprintf (of, "\tcall\t_external_startup\n");
    _xa51_genXINIT(of);
    fprintf (of, "\tcall\t_main\n");
    fprintf (of, "\treset\t;main should not return\n");
  }
}

/* dummy linker for now */
void xa_link(void) {
}

/* Do CSE estimation */
static bool cseCostEstimation (iCode *ic, iCode *pdic)
{
    operand *result = IC_RESULT(ic);
    sym_link *result_type = operandType(result);

    /* if it is a pointer then return ok for now */
    if (IC_RESULT(ic) && IS_PTR(result_type)) return 1;
    
    /* if bitwise | add & subtract then no since xa51 is pretty good at it 
       so we will cse only if they are local (i.e. both ic & pdic belong to
       the same basic block */
    if (IS_BITWISE_OP(ic) || ic->op == '+' || ic->op == '-') {
	/* then if they are the same Basic block then ok */
	if (ic->eBBlockNum == pdic->eBBlockNum) return 1;
	else return 0;
    }
	
    /* for others it is cheaper to do the cse */
    return 1;
}

/** $1 is always the basename.
    $2 is always the output file.
    $3 varies
    $l is the list of extra options that should be there somewhere...
    MUST be terminated with a NULL.
*/
static const char *_linkCmd[] =
{
  "xa_link", "", NULL
};

/* $3 is replaced by assembler.debug_opts resp. port->assembler.plain_opts */
static const char *_asmCmd[] =
{
  "xa_rasm", "$l", "$3", "$1.xa", NULL
};

/* Globals */
PORT xa51_port =
{
  TARGET_ID_XA51,
  "xa51",
  "MCU 80C51XA",       		/* Target name */
  {
    FALSE,			/* Emit glue around main */
    MODEL_LARGE,
    MODEL_LARGE
  },
  {
    _asmCmd,
    NULL,
    "",				/* Options with debug */
    "",				/* Options without debug */
    0,
    ".xa",
    NULL			/* no do_assemble function */
  },
  {
    _linkCmd,
    NULL,
    NULL,
    ".rel"
  },
  {
    _defaultRules
  },
  {
	/* Sizes: char, short, int, long, ptr, fptr, gptr, bit, float, max */
    1, 2, 2, 4, 2, 2, 3, 1, 4, 4
  },
  {
    "XSEG    (XDATA)",
    "STACK   (XDATA)",
    "CSEG    (CODE)",
    "DSEG    (DATA)",
    NULL, //"ISEG    (DATA)",
    "XSEG    (XDATA)",
    "BSEG    (BIT)",
    NULL, //"RSEG    (DATA)",
    "GSINIT  (CODE)",
    NULL, //"OSEG    (OVR,XDATA)",
    "GSFINAL (CODE)",
    "HOME    (CODE)",
    "XISEG   (XDATA)", // initialized xdata
    "XINIT   (CODE)", // a code copy of xiseg
    NULL, // default local map
    NULL, // default global map
    1
  },
  {
    -1, // stack grows down
    0, // bank overhead NUY
    4, // isr overhead, page zero mode
    2, // function call overhead, page zero mode
    0, // reentrant overhead NUY
    0 // banked overhead NUY
  },
    /* xa51 has an 16 bit mul */
  {
    2, -2
  },
  "_",
  _xa51_init,
  _xa51_parseOptions,
  _xa51_finaliseOptions,
  _xa51_setDefaultOptions,
  xa51_assignRegisters,
  _xa51_getRegName,
  _xa51_keywords,
  _xa51_genAssemblerPreamble,
  _xa51_genAssemblerEnd,
  _xa51_genIVT,
  _xa51_genXINIT,
  _xa51_reset_regparm,
  _xa51_regparm,
  NULL, // process_pragma()
  NULL, // getMangledFunctionName()
  NULL, // hasNativeMulFor()
  TRUE, // use_dw_for_init
  0,				/* leave lt */
  0,				/* leave gt */
  0,				/* transform <= to ! > */
  0,				/* transform >= to ! < */
  0,				/* transform != to !(a == b) */
  0,				/* leave == */
  FALSE,                        /* No array initializer support. */
  cseCostEstimation,
  NULL, 			/* no builtin functions */
  GPOINTER,			/* treat unqualified pointers as "generic" pointers */
  1,				/* reset labelKey to 1 */
  1,				/* globals & local static allowed */
  PORT_MAGIC
};
