/*-------------------------------------------------------------------------
  main.c - Z80 specific definitions.

  Michael Hope <michaelh@juju.net.nz> 2001

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   In other words, you are welcome to use, share and improve this program.
   You are forbidden to forbid anyone else to use, share and improve
   what you give them.   Help stamp out software-hoarding!
-------------------------------------------------------------------------*/

#include <sys/stat.h>
#include "z80.h"
#include "SDCCsystem.h"
#include "SDCCutil.h"
#include "SDCCargs.h"
#include "dbuf_string.h"

#define OPTION_BO              "-bo"
#define OPTION_BA              "-ba"
#define OPTION_CODE_SEG        "--codeseg"
#define OPTION_CONST_SEG       "--constseg"
#define OPTION_CALLEE_SAVES_BC "--callee-saves-bc"
#define OPTION_PORTMODE        "--portmode="
#define OPTION_ASM             "--asm="
#define OPTION_NO_STD_CRT0     "--no-std-crt0"
#define OPTION_RESERVE_IY      "--reserve-regs-iy"
#define OPTION_OLDRALLOC       "--oldralloc"
#define OPTION_FRAMEPOINTER    "--fno-omit-frame-pointer"

static char _z80_defaultRules[] = {
#include "peeph.rul"
#include "peeph-z80.rul"
};


Z80_OPTS z80_opts;

static OPTION _z80_options[] = {
  {0, OPTION_CALLEE_SAVES_BC, &z80_opts.calleeSavesBC, "Force a called function to always save BC"},
  {0, OPTION_CODE_SEG,        &options.code_seg, "<name> use this name for the code segment", CLAT_STRING},
  {0, OPTION_CONST_SEG,       &options.const_seg, "<name> use this name for the const segment", CLAT_STRING},
  {0, OPTION_NO_STD_CRT0,     &options.no_std_crt0, "For the z80 do not link default crt0.rel"},
  {0, OPTION_RESERVE_IY,      &z80_opts.reserveIY, "Do not use IY (incompatible with --fomit-frame-pointer)"},
  {0, OPTION_OLDRALLOC,       &options.oldralloc, "Use old register allocator"},
  {0, OPTION_FRAMEPOINTER,    &z80_opts.noOmitFramePtr, "Do not omit frame pointer"},
  {0, NULL}
};

typedef enum
{
  /* Must be first */
  ASM_TYPE_ASXXXX,
  ASM_TYPE_RGBDS,
  ASM_TYPE_ISAS,
  ASM_TYPE_Z80ASM
}
ASM_TYPE;

static struct
{
  ASM_TYPE asmType;
  /* determine if we can register a parameter */
  int regParams;
}
_G;

static char *_keywords[] = {
  "sfr",
  "nonbanked",
  "banked",
  "at",                         //.p.t.20030714 adding support for 'sfr at ADDR' construct
  "_naked",                     //.p.t.20030714 adding support for '_naked' functions
  "critical",
  "interrupt",
  NULL
};

extern PORT z80_port;
extern PORT r2k_port;
extern PORT gbz80_port;

#include "mappings.i"

static builtins _z80_builtins[] = {
  {"__builtin_memcpy", "vg*", 3, {"vg*", "Cvg*", "ui"}},
  {"__builtin_strcpy", "cg*", 2, {"cg*", "Ccg*"}},
  {"__builtin_strncpy", "cg*", 3, {"cg*", "Ccg*", "ui"}},
  {"__builtin_strchr", "cg*", 2, {"Ccg*", "i"}},
  {"__builtin_memset", "vg*", 3, {"vg*", "i", "ui"}},
  {NULL, NULL, 0, {NULL}}
};

static void
_z80_init (void)
{
  z80_opts.sub = SUB_Z80;
  asm_addTree (&_asxxxx_z80);
}

static void
_reset_regparm (void)
{
  _G.regParams = 0;
}

static int
_reg_parm (sym_link * l, bool reentrant)
{
  if (options.noRegParams)
    {
      return FALSE;
    }
  else
    {
      if (!IS_REGISTER (l) || getSize (l) > 2)
        {
          return FALSE;
        }
      if (_G.regParams == 2)
        {
          return FALSE;
        }
      else
        {
          _G.regParams++;
          return TRUE;
        }
    }
}

enum
{
  P_BANK = 1,
  P_PORTMODE,
  P_CODESEG,
  P_CONSTSEG,
};

static int
do_pragma (int id, const char *name, const char *cp)
{
  struct pragma_token_s token;
  int err = 0;
  int processed = 1;

  init_pragma_token (&token);

  switch (id)
    {
    case P_BANK:
      {
        struct dbuf_s buffer;

        dbuf_init (&buffer, 128);

        cp = get_pragma_token (cp, &token);

        switch (token.type)
          {
          case TOKEN_EOL:
            err = 1;
            break;

          case TOKEN_INT:
            switch (_G.asmType)
              {
              case ASM_TYPE_ASXXXX:
                dbuf_printf (&buffer, "CODE_%d", token.val.int_val);
                break;

              case ASM_TYPE_RGBDS:
                dbuf_printf (&buffer, "CODE,BANK[%d]", token.val.int_val);
                break;

              case ASM_TYPE_ISAS:
                /* PENDING: what to use for ISAS? */
                dbuf_printf (&buffer, "CODE,BANK(%d)", token.val.int_val);
                break;

              default:
                wassert (0);
              }
            break;

          default:
            {
              const char *str = get_pragma_string (&token);

              dbuf_append_str (&buffer, (0 == strcmp ("BASE", str)) ? "HOME" : str);
            }
            break;
          }

        cp = get_pragma_token (cp, &token);
        if (TOKEN_EOL != token.type)
          {
            err = 1;
            break;
          }

        dbuf_c_str (&buffer);
      }
      break;

    case P_PORTMODE:
      {                         /*.p.t.20030716 - adding pragma to manipulate z80 i/o port addressing modes */
        const char *str;

        cp = get_pragma_token (cp, &token);

        if (TOKEN_EOL == token.type)
          {
            err = 1;
            break;
          }

        str = get_pragma_string (&token);

        cp = get_pragma_token (cp, &token);
        if (TOKEN_EOL != token.type)
          {
            err = 1;
            break;
          }

        if (!strcmp (str, "z80"))
          {
            z80_opts.port_mode = 80;
          }
        else
          err = 1;
      }
      break;

    case P_CODESEG:
    case P_CONSTSEG:
      {
        char *segname;

        cp = get_pragma_token (cp, &token);
        if (token.type == TOKEN_EOL)
          {
            err = 1;
            break;
          }

        segname = Safe_strdup (get_pragma_string (&token));

        cp = get_pragma_token (cp, &token);
        if (token.type != TOKEN_EOL)
          {
            Safe_free (segname);
            err = 1;
            break;
          }

        if (id == P_CODESEG)
          {
            if (options.code_seg)
              Safe_free (options.code_seg);
            options.code_seg = segname;
          }
        else
          {
            if (options.const_seg)
              Safe_free (options.const_seg);
            options.const_seg = segname;
          }
      }
      break;

    default:
      processed = 0;
      break;
    }

  get_pragma_token (cp, &token);

  if (1 == err)
    werror (W_BAD_PRAGMA_ARGUMENTS, name);

  free_pragma_token (&token);
  return processed;
}

static struct pragma_s pragma_tbl[] = {
  {"bank", P_BANK, 0, do_pragma},
  {"portmode", P_PORTMODE, 0, do_pragma},
  {"codeseg", P_CODESEG, 0, do_pragma},
  {"constseg", P_CONSTSEG, 0, do_pragma},
  {NULL, 0, 0, NULL},
};

static int
_process_pragma (const char *s)
{
  return process_pragma_tbl (pragma_tbl, s);
}

static bool
_parseOptions (int *pargc, char **argv, int *i)
{
  return FALSE;
}

static void
_setValues (void)
{
  const char *s;
  struct dbuf_s dbuf;

  if (options.nostdlib == FALSE)
    {
      const char *s;
      char *path;
      struct dbuf_s dbuf;

      dbuf_init (&dbuf, PATH_MAX);

      for (s = setFirstItem (libDirsSet); s != NULL; s = setNextItem (libDirsSet))
        {
          path = buildCmdLine2 ("-k\"%s" DIR_SEPARATOR_STRING "{port}\" ", s);
          dbuf_append_str (&dbuf, path);
          Safe_free (path);
        }
      path = buildCmdLine2 ("-l\"{port}.lib\"", s);
      dbuf_append_str (&dbuf, path);
      Safe_free (path);

      setMainValue ("z80libspec", dbuf_c_str (&dbuf));
      dbuf_destroy (&dbuf);

      for (s = setFirstItem (libDirsSet); s != NULL; s = setNextItem (libDirsSet))
        {
          struct stat stat_buf;

          path = buildCmdLine2 ("%s" DIR_SEPARATOR_STRING "{port}" DIR_SEPARATOR_STRING "crt0{objext}", s);
          if (stat (path, &stat_buf) == 0)
            {
              Safe_free (path);
              break;
            }
          else
            Safe_free (path);
        }

      if (s == NULL)
        setMainValue ("z80crt0", "\"crt0{objext}\"");
      else
        {
          struct dbuf_s dbuf;

          dbuf_init (&dbuf, 128);
          dbuf_printf (&dbuf, "\"%s\"", path);
          setMainValue ("z80crt0", dbuf_c_str (&dbuf));
          dbuf_destroy (&dbuf);
        }
    }
  else
    {
      setMainValue ("z80libspec", "");
      setMainValue ("z80crt0", "");
    }

  setMainValue ("z80extralibfiles", (s = joinStrSet (libFilesSet)));
  Safe_free ((void *) s);
  setMainValue ("z80extralibpaths", (s = joinStrSet (libPathsSet)));
  Safe_free ((void *) s);

  setMainValue ("z80outputtypeflag", "-i");
  setMainValue ("z80outext", ".bin");

  setMainValue ("stdobjdstfilename", "{dstfilename}{objext}");
  setMainValue ("stdlinkdstfilename", "{dstfilename}{z80outext}");

  setMainValue ("z80extraobj", (s = joinStrSet (relFilesSet)));
  Safe_free ((void *) s);

  dbuf_init (&dbuf, 128);
  dbuf_printf (&dbuf, "-b_CODE=0x%04X -b_DATA=0x%04X", options.code_loc, options.data_loc);
  setMainValue ("z80bases", dbuf_c_str (&dbuf));
  dbuf_destroy (&dbuf);

  /* For the old register allocator (with the new one we decide to omit the frame pointer for each function individually) */
  if (options.omitFramePtr)
    port->stack.call_overhead = 2;
}

static void
_finaliseOptions (void)
{
  port->mem.default_local_map = data;
  port->mem.default_globl_map = data;

  if (IY_RESERVED)
    port->num_regs -= 2;

  _setValues ();
}

static void
_setDefaultOptions (void)
{
  /* SirCmpwn NOTE: We may want to change these defaults at some point, particular with respect to code locations */
  options.nopeep = 0;
  options.stackAuto = 1;
  /* first the options part */
  options.intlong_rent = 1;
  options.float_rent = 1;
  options.noRegParams = 1;
  /* Default code and data locations. */
  options.code_loc = 0x200;

  options.data_loc = 0x8000;
  options.out_fmt = '\0';        /* Default output format is ihx */
}

static const char *
_getRegName (const struct reg_info *reg)
{
  if (reg)
    {
      return reg->name;
    }
  /*  assert (0); */
  return "err";
}

static bool
_hasNativeMulFor (iCode *ic, sym_link *left, sym_link *right)
{
  sym_link *test = NULL;
  int result_size = IS_SYMOP (IC_RESULT(ic)) ? getSize (OP_SYM_TYPE (IC_RESULT(ic))) : 4;

  if (ic->op != '*')
    {
      return FALSE;
    }

  if (IS_LITERAL (left))
    test = left;
  else if (IS_LITERAL (right))
    test = right;
  /* 8x8 unsigned multiplication code is shorter than
     call overhead for the multiplication routine. */
  else if (IS_CHAR (right) && IS_UNSIGNED (right) && IS_CHAR (left) && IS_UNSIGNED (left))
    {
      return TRUE;
    }
  /* Same for any multiplication with 8 bit result. */
  else if (result_size == 1)
    {
      return TRUE;
    }
  else
    {
      return FALSE;
    }

  if (getSize (test) <= 2)
    {
      return TRUE;
    }

  return FALSE;
}

/* Indicate which extended bit operations this port supports */
static bool
hasExtBitOp (int op, int size)
{
  if (op == GETHBIT)
    return TRUE;
  else
    return FALSE;
}

/* Indicate the expense of an access to an output storage class */
static int
oclsExpense (struct memmap *oclass)
{
  if (IN_FARSPACE (oclass))
    return 1;

  return 0;
}


//#define LINKCMD "sdld{port} -nf {dstfilename}"
/*
#define LINKCMD \
    "sdld{port} -n -c -- {z80bases} -m -j" \
    " {z80libspec}" \
    " {z80extralibfiles} {z80extralibpaths}" \
    " {z80outputtypeflag} \"{linkdstfilename}\"" \
    " {z80crt0}" \
    " \"{dstfilename}{objext}\"" \
    " {z80extraobj}"
*/

static const char *_z80LinkCmd[] = {
  "scas", "-l", "-o", "$1", "$1.o", NULL
};

/* $3 is replaced by assembler.debug_opts resp. port->assembler.plain_opts */
static const char *_z80AsmCmd[] = {
  "scas", "-O", "-o", "$2.o", "$1.asm", NULL
};

static const char *const _crt[] = { "knightos-crt0.o", NULL, };
static const char *const _libs_z80[] = { "z80", NULL, };

/* Globals */
PORT z80_port = {
  3 /* SirCmpwn NOTE: This was TARGET_ID_Z80, which isn't in use in theory. We should eventually get rid of port targets. */,
  "z80",
  "Zilog Z80",                  /* Target name */
  NULL,                         /* Processor name */
  {
   glue,
   FALSE,
   NO_MODEL,
   NO_MODEL,
   NULL,                        /* model == target */
   },
  {                             /* Assembler */
   _z80AsmCmd,
   NULL,
   "",                /* Options with debug */
   "",                 /* Options without debug */
   0,
   ".asm"},
  {                             /* Linker */
   _z80LinkCmd,                 //NULL,
   NULL,                        //LINKCMD,
   NULL,
   "",
   0,
   _crt,                        /* crt */
   _libs_z80,                   /* libs */
   },
  {                             /* Peephole optimizer */
   _z80_defaultRules,
   z80instructionSize,
   0,
   0,
   0,
   z80notUsed,
   z80canAssign,
   z80notUsedFrom,
   },
  {
   /* Sizes: char, short, int, long, long long, ptr, fptr, gptr, bit, float, max */
   1, 2, 2, 4, 8, 2, 2, 2, 1, 4, 4},
  /* tags for generic pointers */
  {0x00, 0x40, 0x60, 0x80},     /* far, near, xstack, code */
  {
   "XSEG",
   "STACK",
   "CODE",
   "DATA",
   NULL,                        /* idata */
   NULL,                        /* pdata */
   NULL,                        /* xdata */
   NULL,                        /* bit */
   "RSEG (ABS)",
   "GSINIT",                    /* static initialization */
   NULL,                        /* overlay */
   "GSFINAL",
   "HOME",
   NULL,                        /* xidata */
   NULL,                        /* xinit */
   NULL,                        /* const_name */
   "CABS (ABS)",                /* cabs_name */
   "DABS (ABS)",                /* xabs_name */
   NULL,                        /* iabs_name */
   "INITIALIZED",               /* name of segment for initialized variables */
   "INITIALIZER",               /* name of segment for copies of initialized variables in code space */
   NULL,
   NULL,
   1,                           /* CODE  is read-only */
   1                            /* No fancy alignments supported. */
   },
  {NULL, NULL},
  {
   -1, 0, 0, 4, 0, 2},
  /* Z80 has no native mul/div commands */
  {
   0, -1},
  {
   z80_emitDebuggerSymbol},
  {
   255,                         /* maxCount */
   3,                           /* sizeofElement */
   /* The rest of these costs are bogus. They approximate */
   /* the behavior of src/SDCCicode.c 1.207 and earlier.  */
   {4, 4, 4},                   /* sizeofMatchJump[] */
   {0, 0, 0},                   /* sizeofRangeCompare[] */
   0,                           /* sizeofSubtract */
   3,                           /* sizeofDispatch */
   },
  "_",
  _z80_init,
  _parseOptions,
  _z80_options,
  NULL,
  _finaliseOptions,
  _setDefaultOptions,
  z80_assignRegisters,
  _getRegName,
  NULL,
  _keywords,
  0,                            /* no assembler preamble */
  NULL,                         /* no genAssemblerEnd */
  0,                            /* no local IVT generation code */
  0,                            /* no genXINIT code */
  NULL,                         /* genInitStartup */
  _reset_regparm,
  _reg_parm,
  _process_pragma,
  NULL,
  _hasNativeMulFor,
  hasExtBitOp,                  /* hasExtBitOp */
  oclsExpense,                  /* oclsExpense */
  TRUE,
  TRUE,                         /* little endian */
  0,                            /* leave lt */
  0,                            /* leave gt */
  1,                            /* transform <= to ! > */
  1,                            /* transform >= to ! < */
  1,                            /* transform != to !(a == b) */
  0,                            /* leave == */
  FALSE,                        /* Array initializer support. */
  0,                            /* no CSE cost estimation yet */
  _z80_builtins,                /* builtin functions */
  GPOINTER,                     /* treat unqualified pointers as "generic" pointers */
  1,                            /* reset labelKey to 1 */
  1,                            /* globals & local statics allowed */
  9,                            /* Number of registers handled in the tree-decomposition-based register allocator in SDCCralloc.hpp */
  PORT_MAGIC
};
