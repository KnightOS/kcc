/** @file main.c
    pic16 specific general functions.

    Note that mlh prepended _pic16_ on the static functions.  Makes
    it easier to set a breakpoint using the debugger.
*/
#include "common.h"
#include "main.h"
#include "ralloc.h"
#include "device.h"
#include "SDCCutil.h"
//#include "gen.h"


static char _defaultRules[] =
{
#include "peeph.rul"
};

/* list of key words used by pic16 */
static char *_pic16_keywords[] =
{
  "at",
  "bit",
  "code",
  "critical",
  "data",
  "far",
  "idata",
  "interrupt",
  "near",
  "pdata",
  "reentrant",
  "sfr",
  "sbit",
  "using",
  "xdata",
  "_data",
  "_code",
  "_generic",
  "_near",
  "_xdata",
  "_pdata",
  "_idata",
  NULL
};

void  pic16_pCodeInitRegisters(void);

void pic16_assignRegisters (eBBlock ** ebbs, int count);

static int regParmFlg = 0;	/* determine if we can register a parameter */

static void
_pic16_init (void)
{
  asm_addTree (&asm_asxxxx_mapping);
  pic16_pCodeInitRegisters();
}

static void
_pic16_reset_regparm ()
{
  regParmFlg = 0;
}

static int
_pic16_regparm (sym_link * l)
{
  /* for this processor it is simple
     can pass only the first parameter in a register */
  //if (regParmFlg)
  //  return 0;

  regParmFlg++;// = 1;
  return 1;
}

static int
_process_pragma(const char *sz)
{
  static const char *WHITE = " \t";
  char	*ptr = strtok((char *)sz, WHITE);

  if (startsWith (ptr, "memmap"))
    {
      char	*start;
      char	*end;
      char	*type;
      char	*alias;

      start = strtok((char *)NULL, WHITE);
      end = strtok((char *)NULL, WHITE);
      type = strtok((char *)NULL, WHITE);
      alias = strtok((char *)NULL, WHITE);

      if (start != (char *)NULL
	  && end != (char *)NULL
	  && type != (char *)NULL) {
	value		*startVal = constVal(start);
	value		*endVal = constVal(end);
	value		*aliasVal;
	memRange	r;

	if (alias == (char *)NULL) {
	  aliasVal = constVal(0);
	} else {
	  aliasVal = constVal(alias);
	}

	r.start_address = (int)floatFromVal(startVal);
	r.end_address = (int)floatFromVal(endVal);
	r.alias = (int)floatFromVal(aliasVal);
	r.bank = (r.start_address >> 7) & 0xf;

	if (strcmp(type, "RAM") == 0) {
	  pic16_addMemRange(&r, 0);
	} else if (strcmp(type, "SFR") == 0) {
	  pic16_addMemRange(&r, 1);
	} else {
	  return 1;
	}
      }

      return 0;
    } else if (startsWith (ptr, "maxram")) {
      char *maxRAM = strtok((char *)NULL, WHITE);

      if (maxRAM != (char *)NULL) {
	int	maxRAMaddress;
	value 	*maxRAMVal;

	maxRAMVal = constVal(maxRAM);
	maxRAMaddress = (int)floatFromVal(maxRAMVal);
	pic16_setMaxRAM(maxRAMaddress);
      }
	
      return 0;
    }
  return 1;
}

static bool
_pic16_parseOptions (int *pargc, char **argv, int *i)
{
  /* TODO: allow port-specific command line options to specify
   * segment names here.
   */
  return FALSE;
}

static void
_pic16_finaliseOptions (void)
{

      port->mem.default_local_map = data;
      port->mem.default_globl_map = data;
#if 0
  /* Hack-o-matic: if we are using the flat24 model,
   * adjust pointer sizes.
   */
  if (options.model == MODEL_FLAT24)
    {

      fprintf (stderr, "*** WARNING: you should use the '-mds390' option "
	       "for DS80C390 support. This code generator is "
	       "badly out of date and probably broken.\n");

      port->s.fptr_size = 3;
      port->s.gptr_size = 4;
      port->stack.isr_overhead++;	/* Will save dpx on ISR entry. */
#if 1
      port->stack.call_overhead++;	/* This acounts for the extra byte 
					 * of return addres on the stack.
					 * but is ugly. There must be a 
					 * better way.
					 */
#endif
      fReturn = fReturn390;
      fReturnSize = 5;
    }

  if (options.model == MODEL_LARGE)
    {
      port->mem.default_local_map = xdata;
      port->mem.default_globl_map = xdata;
    }
  else
    {
      port->mem.default_local_map = data;
      port->mem.default_globl_map = data;
    }

  if (options.stack10bit)
    {
      if (options.model != MODEL_FLAT24)
	{
	  fprintf (stderr,
		   "*** warning: 10 bit stack mode is only supported in flat24 model.\n");
	  fprintf (stderr, "\t10 bit stack mode disabled.\n");
	  options.stack10bit = 0;
	}
      else
	{
	  /* Fixup the memory map for the stack; it is now in
	   * far space and requires a FPOINTER to access it.
	   */
	  istack->fmap = 1;
	  istack->ptrType = FPOINTER;
	}
    }
#endif
}

static void
_pic16_setDefaultOptions (void)
{
}

static const char *
_pic16_getRegName (struct regs *reg)
{
  if (reg)
    return reg->name;
  return "err";
}

extern char *pic16_processor_base_name(void);

static void
_pic16_genAssemblerPreamble (FILE * of)
{
  char * name = pic16_processor_base_name();

  if(!name) {

    name = "p18f452";
    fprintf(stderr,"WARNING: No Pic has been selected, defaulting to %s\n",name);
  }

  fprintf (of, "\tlist\tp=%s\n",&name[1]);
  fprintf (of, "\tinclude \"%s.inc\"\n",name);

#if 0
  fprintf (of, "\t__config _CONFIG1H,0x%x\n",pic16_getConfigWord(0x300001));
  fprintf (of, "\t__config _CONFIG2L,0x%x\n",pic16_getConfigWord(0x300002));
  fprintf (of, "\t__config _CONFIG2H,0x%x\n",pic16_getConfigWord(0x300003));
  fprintf (of, "\t__config _CONFIG3H,0x%x\n",pic16_getConfigWord(0x300005));
  fprintf (of, "\t__config _CONFIG4L,0x%x\n",pic16_getConfigWord(0x300006));
  fprintf (of, "\t__config _CONFIG5L,0x%x\n",pic16_getConfigWord(0x300008));
  fprintf (of, "\t__config _CONFIG5H,0x%x\n",pic16_getConfigWord(0x300009));
  fprintf (of, "\t__config _CONFIG6L,0x%x\n",pic16_getConfigWord(0x30000a));
  fprintf (of, "\t__config _CONFIG6H,0x%x\n",pic16_getConfigWord(0x30000b));
  fprintf (of, "\t__config _CONFIG7L,0x%x\n",pic16_getConfigWord(0x30000c));
  fprintf (of, "\t__config _CONFIG7H,0x%x\n",pic16_getConfigWord(0x30000d));
#endif

  fprintf (of, "\tradix dec\n");
}

/* Generate interrupt vector table. */
static int
_pic16_genIVT (FILE * of, symbol ** interrupts, int maxInterrupts)
{
  int i;

  if (options.model != MODEL_FLAT24)
    {
      /* Let the default code handle it. */
      return FALSE;
    }

  fprintf (of, "\t;ajmp\t__sdcc_gsinit_startup\n");

  /* now for the other interrupts */
  for (i = 0; i < maxInterrupts; i++)
    {
      if (interrupts[i])
	{
	  fprintf (of, "\t;ljmp\t%s\n\t.ds\t4\n", interrupts[i]->rname);
	}
      else
	{
	  fprintf (of, "\t;reti\n\t.ds\t7\n");
	}
    }

  return TRUE;
}

static bool
_hasNativeMulFor (iCode *ic, sym_link *left, sym_link *right)
{
  //  sym_link *test = NULL;
  //  value *val;

  fprintf(stderr,"checking for native mult\n");

  if ( ic->op != '*')
    {
      return FALSE;
    }

  return TRUE;
/*
  if ( IS_LITERAL (left))
    {
      fprintf(stderr,"left is lit\n");
      test = left;
      val = OP_VALUE (IC_LEFT (ic));
    }
  else if ( IS_LITERAL (right))
    {
      fprintf(stderr,"right is lit\n");
      test = left;
      val = OP_VALUE (IC_RIGHT (ic));
    }
  else
    {
      fprintf(stderr,"oops, neither is lit so no\n");
      return FALSE;
    }

  if ( getSize (test) <= 2)
    {
      fprintf(stderr,"yep\n");
      return TRUE;
    }
  fprintf(stderr,"nope\n");

  return FALSE;
*/
}

/** $1 is always the basename.
    $2 is always the output file.
    $3 varies
    $l is the list of extra options that should be there somewhere...
    MUST be terminated with a NULL.
*/
static const char *_linkCmd[] =
{
  "gplink", "\"$1.o\"", "-o $1", "$l", NULL
};

/* Sigh. This really is not good. For now, I recommend:
 * sdcc -S -mpic16 file.c
 * the -S option does not compile or link
 */
static const char *_asmCmd[] =
{
  "gpasm", "-c  -I/usr/local/share/gputils/header", "\"$1.asm\"", NULL

};

/* Globals */
PORT pic16_port =
{
  TARGET_ID_PIC16,
  "pic16",
  "MCU PIC16",			/* Target name */
  "p18f452",                    /* Processor */
  {
    TRUE,			/* Emit glue around main */
    MODEL_SMALL | MODEL_LARGE | MODEL_FLAT24,
    MODEL_SMALL
  },
  {
    _asmCmd,
    NULL,
    NULL,
    NULL,
	//"-plosgffc",          /* Options with debug */
	//"-plosgff",           /* Options without debug */
    0,
    ".asm",
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
    1, 2, 2, 4, 2, 2, 2, 1, 4, 4
	/* TSD - I changed the size of gptr from 3 to 1. However, it should be
	   2 so that we can accomodate the PIC's with 4 register banks (like the
	   16f877)
	 */
  },
  {
    "XSEG    (XDATA)",
    "STACK   (DATA)",
    "CSEG    (CODE)",
    "DSEG    (DATA)",
    "ISEG    (DATA)",
    "XSEG    (XDATA)",
    "BSEG    (BIT)",
    "RSEG    (DATA)",
    "GSINIT  (CODE)",
    "OSEG    (OVR,DATA)",
    "GSFINAL (CODE)",
    "HOME	 (CODE)",
    NULL, // xidata
    NULL, // xinit
    NULL,
    NULL,
    1        // code is read only
  },
  {
    +1, 1, 4, 1, 1, 0
  },
    /* pic16 has an 8 bit mul */
  {
    1, -1
  },
  "_",
  _pic16_init,
  _pic16_parseOptions,
  NULL,
  _pic16_finaliseOptions,
  _pic16_setDefaultOptions,
  pic16_assignRegisters,
  _pic16_getRegName,
  _pic16_keywords,
  _pic16_genAssemblerPreamble,
  NULL,				/* no genAssemblerEnd */
  _pic16_genIVT,
  NULL, // _pic16_genXINIT
  _pic16_reset_regparm,
  _pic16_regparm,
  _process_pragma,				/* process a pragma */
  NULL,
  _hasNativeMulFor,
  FALSE,
  0,				/* leave lt */
  0,				/* leave gt */
  1,				/* transform <= to ! > */
  1,				/* transform >= to ! < */
  1,				/* transform != to !(a == b) */
  0,				/* leave == */
  FALSE,                        /* No array initializer support. */
  0,                            /* no CSE cost estimation yet */
  NULL, 			/* no builtin functions */
  GPOINTER,			/* treat unqualified pointers as "generic" pointers */
  1,				/* reset labelKey to 1 */
  1,				/* globals & local static allowed */
  PORT_MAGIC
};
