/*-------------------------------------------------------------------------
  gen.c - Z80 specific code generator.

  Benchmarks on dhry.c 2.1 with 32766 loops and a 10ms clock:
            ticks dhry  size
  Base with asm strcpy / strcmp / memcpy: 23198 141 1A14
  Improved WORD push                    22784 144 19AE
  With label1 on                        22694 144 197E
  With label2 on                        22743 144 198A
  With label3 on                        22776 144 1999
  With label4 on                        22776 144 1999
  With all 'label' on                   22661 144 196F
  With loopInvariant on                 20919 156 19AB
  With loopInduction on                 Breaks    198B
  With all working on                   20796 158 196C
  Slightly better genCmp(signed)        20597 159 195B
  Better reg packing, first peephole    20038 163 1873
  With assign packing                   19281 165 1849
  5/3/00                                17741 185 17B6
  With reg params for mul and div       16234 202 162D

  Michael Hope <michaelh@earthling.net> 2000
  Based on the mcs51 generator -
      Sandeep Dutta . sandeep.dutta@usa.net (1998)
   and -  Jean-Louis VERN.jlvern@writeme.com (1999)

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef HAVE_SYS_ISA_DEFS_H
#include <sys/isa_defs.h>
#endif

#include "z80.h"
#include "SDCCglobl.h"
#include "SDCCpeeph.h"
#include "gen.h"
#include "SDCCglue.h"
#include "newalloc.h"

/* this is the down and dirty file with all kinds of kludgy & hacky
   stuff. This is what it is all about CODE GENERATION for a specific MCU.
   Some of the routines may be reusable, will have to see */

/* Z80 calling convention description.
   Parameters are passed right to left.  As the stack grows downwards,
   the parameters are arranged in left to right in memory.
   Parameters may be passed in the HL and DE registers with one
   parameter per pair.
   PENDING: What if the parameter is a long?
   Everything is caller saves. i.e. the caller must save any registers
   that it wants to preserve over the call.
   The return value is returned in DEHL.  DE is normally used as a
   working register pair.  Caller saves allows it to be used for a
   return value.
   va args functions do not use register parameters.  All arguments
   are passed on the stack.
   IX is used as an index register to the top of the local variable
   area.  ix-0 is the top most local variable.
*/
static char *_z80_return[] =
{"l", "h", "e", "d"};
static char *_gbz80_return[] =
{"e", "d", "l", "h"};
static char *_fReceive[] =
  { "c", "b", "e", "d" };

static char **_fReturn;
static char **_fTmp;

extern FILE *codeOutFile;

typedef enum
  {
    PAIR_INVALID,
    PAIR_AF,
    PAIR_BC,
    PAIR_DE,
    PAIR_HL,
    PAIR_IY,
    PAIR_IX,
    NUM_PAIRS
  } PAIR_ID;

static struct
{
  const char *name;
  const char *l;
  const char *h;
} _pairs[NUM_PAIRS] = {
  {    "??1", "?2", "?3" },
  {    "af", "f", "a" },
  {    "bc", "c", "b" },
  {    "de", "e", "d" },
  {    "hl", "l", "h" },
  {    "iy", "iy.l?", "iy.h?" },
  {    "ix", "ix.l?", "ix.h?" }
};

// PENDING
#define ACC_NAME	_pairs[PAIR_AF].h

#define RESULTONSTACK(x) \
                         (IC_RESULT(x) && IC_RESULT(x)->aop && \
                         IC_RESULT(x)->aop->type == AOP_STK )

enum 
  {
    LSB,
    MSB16,
    MSB24,
    MSB32
  };

static struct 
{
  struct 
  {
    AOP_TYPE last_type;
    const char *lit;
    int offset;
  } pairs[NUM_PAIRS];
  struct 
  {
    int last;
    int pushed;
    int param_offset;
    int offset;
    int pushedBC;
    int pushedDE;
  } stack;
  int frameId;
  int receiveOffset;
  bool flushStatics;
  bool in_home;
  const char *lastFunctionName;

  set *sendSet;

  struct
  {
    /** TRUE if the registers have already been saved. */
    bool saved;
  } saves;

  struct 
  {
    lineNode *head;
    lineNode *current;
    int isInline;
  } lines;

} _G;

static const char *aopGet (asmop * aop, int offset, bool bit16);

static void
_tidyUp (char *buf)
{
  /* Clean up the line so that it is 'prettier' */
  if (strchr (buf, ':'))
    {
      /* Is a label - cant do anything */
      return;
    }
  /* Change the first (and probably only) ' ' to a tab so
     everything lines up.
  */
  while (*buf)
    {
      if (*buf == ' ')
        {
          *buf = '\t';
          break;
        }
      buf++;
    }
}

static void
emit2 (const char *szFormat,...)
{
  char buffer[256];
  va_list ap;

  va_start (ap, szFormat);

  tvsprintf (buffer, szFormat, ap);

  _tidyUp (buffer);
  _G.lines.current = (_G.lines.current ?
	      connectLine (_G.lines.current, newLineNode (buffer)) :
	      (_G.lines.head = newLineNode (buffer)));

  _G.lines.current->isInline = _G.lines.isInline;
}

/*-----------------------------------------------------------------*/
/* emit2 - writes the code into a file : for now it is simple    */
/*-----------------------------------------------------------------*/
void
_emit2 (const char *inst, const char *fmt,...)
{
  va_list ap;
  char lb[INITIAL_INLINEASM];
  char *lbp = lb;

  va_start (ap, fmt);

  if (*inst != '\0')
    {
      sprintf (lb, "%s\t", inst);
      vsprintf (lb + (strlen (lb)), fmt, ap);
    }
  else
    vsprintf (lb, fmt, ap);

  while (isspace (*lbp))
    lbp++;

  if (lbp && *lbp)
    {
      _G.lines.current = (_G.lines.current ?
                  connectLine (_G.lines.current, newLineNode (lb)) :
                  (_G.lines.head = newLineNode (lb)));
    }
  _G.lines.current->isInline = _G.lines.isInline;
  va_end (ap);
}

static void
_emitMove(const char *to, const char *from)
{
  if (strcasecmp(to, from) != 0) 
    {
      emit2("ld %s,%s", to, from);
    }
  else 
    {
      // Optimise it out.
      // Could leave this to the peephole, but sometimes the peephole is inhibited.
    }
}

static void
_moveA(const char *moveFrom)
{
    // Let the peephole optimiser take care of redundent loads
    _emitMove(ACC_NAME, moveFrom);
}

static void
_clearCarry(void)
{
    emit2("xor a,a");
}

const char *
getPairName (asmop * aop)
{
  if (aop->type == AOP_REG)
    {
      switch (aop->aopu.aop_reg[0]->rIdx)
	{
	case C_IDX:
	  return "bc";
	  break;
	case E_IDX:
	  return "de";
	  break;
	case L_IDX:
	  return "hl";
	  break;
	}
    }
  else if (aop->type == AOP_STR)
    {
      switch (*aop->aopu.aop_str[0])
	{
	case 'c':
	  return "bc";
	  break;
	case 'e':
	  return "de";
	  break;
	case 'l':
	  return "hl";
	  break;
	}
    }
  wassert (0);
  return NULL;
}

static PAIR_ID
getPairId (asmop * aop)
{
  if (aop->size == 2)
    {
      if (aop->type == AOP_REG)
	{
	  if ((aop->aopu.aop_reg[0]->rIdx == C_IDX) && (aop->aopu.aop_reg[1]->rIdx == B_IDX))
	    {
	      return PAIR_BC;
	    }
	  if ((aop->aopu.aop_reg[0]->rIdx == E_IDX) && (aop->aopu.aop_reg[1]->rIdx == D_IDX))
	    {
	      return PAIR_DE;
	    }
	  if ((aop->aopu.aop_reg[0]->rIdx == L_IDX) && (aop->aopu.aop_reg[1]->rIdx == H_IDX))
	    {
	      return PAIR_HL;
	    }
	}
      if (aop->type == AOP_STR)
	{
	  if (!strcmp (aop->aopu.aop_str[0], "c") && !strcmp (aop->aopu.aop_str[1], "b"))
	    {
	      return PAIR_BC;
	    }
	  if (!strcmp (aop->aopu.aop_str[0], "e") && !strcmp (aop->aopu.aop_str[1], "d"))
	    {
	      return PAIR_DE;
	    }
	  if (!strcmp (aop->aopu.aop_str[0], "l") && !strcmp (aop->aopu.aop_str[1], "h"))
	    {
	      return PAIR_HL;
	    }
	}
    }
  return PAIR_INVALID;
}

/** Returns TRUE if the registers used in aop form a pair (BC, DE, HL) */
bool
isPair (asmop * aop)
{
  return (getPairId (aop) != PAIR_INVALID);
}

bool
isPtrPair (asmop * aop)
{
  PAIR_ID pairId = getPairId (aop);
  switch (pairId)
    {
    case PAIR_HL:
    case PAIR_IY:
    case PAIR_IX:
      return TRUE;
    default:
      return FALSE;
    }
}
/** Push a register pair onto the stack */
void
genPairPush (asmop * aop)
{
  emit2 ("push %s", getPairName (aop));
}


/*-----------------------------------------------------------------*/
/* newAsmop - creates a new asmOp                                  */
/*-----------------------------------------------------------------*/
static asmop *
newAsmop (short type)
{
  asmop *aop;

  aop = Safe_calloc (1, sizeof (asmop));
  aop->type = type;
  return aop;
}

/*-----------------------------------------------------------------*/
/* aopForSym - for a true symbol                                   */
/*-----------------------------------------------------------------*/
static asmop *
aopForSym (iCode * ic, symbol * sym, bool result, bool requires_a)
{
  asmop *aop;
  memmap *space;

  wassert (ic);
  wassert (sym);
  wassert (sym->etype);

  space = SPEC_OCLS (sym->etype);

  /* if already has one */
  if (sym->aop)
    return sym->aop;

  /* Assign depending on the storage class */
  if (sym->onStack || sym->iaccess)
    {
      emit2 ("; AOP_STK for %s", sym->rname);
      sym->aop = aop = newAsmop (AOP_STK);
      aop->size = getSize (sym->type);
      aop->aopu.aop_stk = sym->stack;
      return aop;
    }

  /* special case for a function */
  if (IS_FUNC (sym->type))
    {
      sym->aop = aop = newAsmop (AOP_IMMD);
      aop->aopu.aop_immd = Safe_calloc (1, strlen (sym->rname) + 1);
      strcpy (aop->aopu.aop_immd, sym->rname);
      aop->size = 2;
      return aop;
    }

  if (IS_GB)
    {
      /* if it is in direct space */
      if (IN_REGSP (space) && !requires_a)
	{
	  sym->aop = aop = newAsmop (AOP_SFR);
	  aop->aopu.aop_dir = sym->rname;
	  aop->size = getSize (sym->type);
	  emit2 ("; AOP_SFR for %s", sym->rname);
	  return aop;
	}
    }

  /* only remaining is far space */
  /* in which case DPTR gets the address */
  if (IS_GB)
    {
      emit2 ("; AOP_HL for %s", sym->rname);
      sym->aop = aop = newAsmop (AOP_HL);
    }
  else
    {
      sym->aop = aop = newAsmop (AOP_IY);
    }
  aop->size = getSize (sym->type);
  aop->aopu.aop_dir = sym->rname;

  /* if it is in code space */
  if (IN_CODESPACE (space))
    aop->code = 1;

  return aop;
}

/*-----------------------------------------------------------------*/
/* aopForRemat - rematerialzes an object                           */
/*-----------------------------------------------------------------*/
static asmop *
aopForRemat (symbol * sym)
{
  char *s = buffer;
  iCode *ic = sym->rematiCode;
  asmop *aop = newAsmop (AOP_IMMD);

  while (1)
    {
      /* if plus or minus print the right hand side */
      if (ic->op == '+' || ic->op == '-')
	{
	  /* PENDING: for re-target */
	  sprintf (s, "0x%04x %c ", (int) operandLitValue (IC_RIGHT (ic)),
		   ic->op);
	  s += strlen (s);
	  ic = OP_SYMBOL (IC_LEFT (ic))->rematiCode;
	  continue;
	}
      /* we reached the end */
      sprintf (s, "%s", OP_SYMBOL (IC_LEFT (ic))->rname);
      break;
    }

  aop->aopu.aop_immd = Safe_calloc (1, strlen (buffer) + 1);
  strcpy (aop->aopu.aop_immd, buffer);
  return aop;
}

/*-----------------------------------------------------------------*/
/* regsInCommon - two operands have some registers in common       */
/*-----------------------------------------------------------------*/
bool
regsInCommon (operand * op1, operand * op2)
{
  symbol *sym1, *sym2;
  int i;

  /* if they have registers in common */
  if (!IS_SYMOP (op1) || !IS_SYMOP (op2))
    return FALSE;

  sym1 = OP_SYMBOL (op1);
  sym2 = OP_SYMBOL (op2);

  if (sym1->nRegs == 0 || sym2->nRegs == 0)
    return FALSE;

  for (i = 0; i < sym1->nRegs; i++)
    {
      int j;
      if (!sym1->regs[i])
	continue;

      for (j = 0; j < sym2->nRegs; j++)
	{
	  if (!sym2->regs[j])
	    continue;

	  if (sym2->regs[j] == sym1->regs[i])
	    return TRUE;
	}
    }

  return FALSE;
}

/*-----------------------------------------------------------------*/
/* operandsEqu - equivalent                                        */
/*-----------------------------------------------------------------*/
bool
operandsEqu (operand * op1, operand * op2)
{
  symbol *sym1, *sym2;

  /* if they not symbols */
  if (!IS_SYMOP (op1) || !IS_SYMOP (op2))
    return FALSE;

  sym1 = OP_SYMBOL (op1);
  sym2 = OP_SYMBOL (op2);

  /* if both are itemps & one is spilt
     and the other is not then false */
  if (IS_ITEMP (op1) && IS_ITEMP (op2) &&
      sym1->isspilt != sym2->isspilt)
    return FALSE;

  /* if they are the same */
  if (sym1 == sym2)
    return 1;

  if (strcmp (sym1->rname, sym2->rname) == 0)
    return 2;


  /* if left is a tmp & right is not */
  if (IS_ITEMP (op1) &&
      !IS_ITEMP (op2) &&
      sym1->isspilt &&
      (sym1->usl.spillLoc == sym2))
    return 3;

  if (IS_ITEMP (op2) &&
      !IS_ITEMP (op1) &&
      sym2->isspilt &&
      sym1->level > 0 &&
      (sym2->usl.spillLoc == sym1))
    return 4;

  return FALSE;
}

/*-----------------------------------------------------------------*/
/* sameRegs - two asmops have the same registers                   */
/*-----------------------------------------------------------------*/
bool
sameRegs (asmop * aop1, asmop * aop2)
{
  int i;

  if (aop1->type == AOP_SFR ||
      aop2->type == AOP_SFR)
    return FALSE;

  if (aop1 == aop2)
    return TRUE;

  if (aop1->type != AOP_REG ||
      aop2->type != AOP_REG)
    return FALSE;

  if (aop1->size != aop2->size)
    return FALSE;

  for (i = 0; i < aop1->size; i++)
    if (aop1->aopu.aop_reg[i] !=
	aop2->aopu.aop_reg[i])
      return FALSE;

  return TRUE;
}

/*-----------------------------------------------------------------*/
/* aopOp - allocates an asmop for an operand  :                    */
/*-----------------------------------------------------------------*/
static void
aopOp (operand * op, iCode * ic, bool result, bool requires_a)
{
  asmop *aop;
  symbol *sym;
  int i;

  if (!op)
    return;

  /* if this a literal */
  if (IS_OP_LITERAL (op))
    {
      op->aop = aop = newAsmop (AOP_LIT);
      aop->aopu.aop_lit = op->operand.valOperand;
      aop->size = getSize (operandType (op));
      return;
    }

  /* if already has a asmop then continue */
  if (op->aop)
    return;

  /* if the underlying symbol has a aop */
  if (IS_SYMOP (op) && OP_SYMBOL (op)->aop)
    {
      op->aop = OP_SYMBOL (op)->aop;
      return;
    }

  /* if this is a true symbol */
  if (IS_TRUE_SYMOP (op))
    {
      op->aop = aopForSym (ic, OP_SYMBOL (op), result, requires_a);
      return;
    }

  /* this is a temporary : this has
     only four choices :
     a) register
     b) spillocation
     c) rematerialize
     d) conditional
     e) can be a return use only */

  sym = OP_SYMBOL (op);

  /* if the type is a conditional */
  if (sym->regType == REG_CND)
    {
      aop = op->aop = sym->aop = newAsmop (AOP_CRY);
      aop->size = 0;
      return;
    }

  /* if it is spilt then two situations
     a) is rematerialize
     b) has a spill location */
  if (sym->isspilt || sym->nRegs == 0)
    {
      /* rematerialize it NOW */
      if (sym->remat)
	{
	  sym->aop = op->aop = aop =
	    aopForRemat (sym);
	  aop->size = getSize (sym->type);
	  return;
	}

      if (sym->accuse)
	{
	  if (sym->accuse == ACCUSE_A)
	    {
	      aop = op->aop = sym->aop = newAsmop (AOP_ACC);
	      aop->size = getSize (sym->type);
              wassertl(aop->size == 1, "Internal error: Caching in A, but too big to fit in A");

              aop->aopu.aop_str[0] = _pairs[PAIR_AF].h;
	    }
	  else if (sym->accuse == ACCUSE_HL)
	    {
	      wassert (!IS_GB);
	      aop = op->aop = sym->aop = newAsmop (AOP_HLREG);
	      aop->size = getSize (sym->type);
              wassertl(aop->size <= 2, "Internal error: Caching in HL, but too big to fit in HL");
              aop->aopu.aop_str[0] = _pairs[PAIR_HL].l;
              aop->aopu.aop_str[1] = _pairs[PAIR_HL].h;
	    }
	  else 
              {
                  wassert (0);
              }
	  return;
	}

      if (sym->ruonly)
	{
	  int i;
	  aop = op->aop = sym->aop = newAsmop (AOP_STR);
	  aop->size = getSize (sym->type);
	  for (i = 0; i < 4; i++)
	    aop->aopu.aop_str[i] = _fReturn[i];
	  return;
	}

      /* else spill location  */
      sym->aop = op->aop = aop =
	aopForSym (ic, sym->usl.spillLoc, result, requires_a);
      aop->size = getSize (sym->type);
      return;
    }

  /* must be in a register */
  sym->aop = op->aop = aop = newAsmop (AOP_REG);
  aop->size = sym->nRegs;
  for (i = 0; i < sym->nRegs; i++)
    aop->aopu.aop_reg[i] = sym->regs[i];
}

/*-----------------------------------------------------------------*/
/* freeAsmop - free up the asmop given to an operand               */
/*----------------------------------------------------------------*/
static void
freeAsmop (operand * op, asmop * aaop, iCode * ic)
{
  asmop *aop;

  if (!op)
    aop = aaop;
  else
    aop = op->aop;

  if (!aop)
    return;

  if (aop->freed)
    goto dealloc;

  aop->freed = 1;

dealloc:
  /* all other cases just dealloc */
  if (op)
    {
      op->aop = NULL;
      if (IS_SYMOP (op))
	{
	  OP_SYMBOL (op)->aop = NULL;
	  /* if the symbol has a spill */
	  if (SPIL_LOC (op))
	    SPIL_LOC (op)->aop = NULL;
	}
    }
}

bool
isLitWord (asmop * aop)
{
  /*    if (aop->size != 2)
     return FALSE; */
  switch (aop->type)
    {
    case AOP_IMMD:
    case AOP_LIT:
      return TRUE;
    default:
      return FALSE;
    }
}

char *
aopGetLitWordLong (asmop * aop, int offset, bool with_hash)
{
  char *s = buffer;
  char *rs;

#if 0
  if (aop->size != 2 && aop->type != AOP_HL)
    return NULL;
#endif
  /* depending on type */
  switch (aop->type)
    {
    case AOP_HL:
    case AOP_IY:
    case AOP_IMMD:
      /* PENDING: for re-target */
      if (with_hash)
	tsprintf (s, "!hashedstr + %d", aop->aopu.aop_immd, offset);
      else
	tsprintf (s, "%s + %d", aop->aopu.aop_immd, offset);
      rs = Safe_calloc (1, strlen (s) + 1);
      strcpy (rs, s);
      return rs;
    case AOP_LIT:
      {
	value *val = aop->aopu.aop_lit;
	/* if it is a float then it gets tricky */
	/* otherwise it is fairly simple */
	if (!IS_FLOAT (val->type))
	  {
	    unsigned long v = (unsigned long) floatFromVal (val);

	    if (offset == 2)
	      v >>= 16;

	    if (with_hash)
	      tsprintf (buffer, "!immedword", v);
	    else
	      tsprintf (buffer, "!constword", v);
	    rs = Safe_calloc (1, strlen (buffer) + 1);
	    return strcpy (rs, buffer);
	  }
	else
	  {
	    /* A float */
	    Z80_FLOAT f;
	    convertFloat (&f, floatFromVal (val));
	    if (with_hash)
	      tsprintf (buffer, "!immedword", f.w[offset / 2]);
	    else
	      tsprintf (buffer, "!constword", f.w[offset / 2]);
	    rs = Safe_calloc (1, strlen (buffer) + 1);
	    return strcpy (rs, buffer);
	  }
      }
    default:
      return NULL;
    }
}

char *
aopGetWord (asmop * aop, int offset)
{
  return aopGetLitWordLong (aop, offset, TRUE);
}

bool
isPtr (const char *s)
{
  if (!strcmp (s, "hl"))
    return TRUE;
  if (!strcmp (s, "ix"))
    return TRUE;
  if (!strcmp (s, "iy"))
    return TRUE;
  return FALSE;
}

static void
adjustPair (const char *pair, int *pold, int new)
{
  wassert (pair);

  while (*pold < new)
    {
      emit2 ("inc %s", pair);
      (*pold)++;
    }
  while (*pold > new)
    {
      emit2 ("dec %s", pair);
      (*pold)--;
    }
}

static void
spillPair (PAIR_ID pairId)
{
  _G.pairs[pairId].last_type = AOP_INVALID;
  _G.pairs[pairId].lit = NULL;
}

static void
spillCached (void)
{
  spillPair (PAIR_HL);
  spillPair (PAIR_IY);
}

static bool
requiresHL (asmop * aop)
{
  switch (aop->type)
    {
    case AOP_IY:
    case AOP_HL:
    case AOP_STK:
      return TRUE;
    default:
      return FALSE;
    }
}

static char *
fetchLitSpecial (asmop * aop, bool negate, bool xor)
{
  unsigned long v;
  value *val = aop->aopu.aop_lit;

  wassert (aop->type == AOP_LIT);
  wassert (!IS_FLOAT (val->type));

  v = (unsigned long) floatFromVal (val);

  if (xor)
    v ^= 0x8000;
  if (negate)
    v = 0-v;
  v &= 0xFFFF;

  tsprintf (buffer, "!immedword", v);
  return gc_strdup (buffer);
}

static void
fetchLitPair (PAIR_ID pairId, asmop * left, int offset)
{
  const char *l;
  const char *pair = _pairs[pairId].name;
  l = aopGetLitWordLong (left, 0, FALSE);
  wassert (l && pair);

  if (isPtr (pair))
    {
      if (pairId == PAIR_HL || pairId == PAIR_IY)
	{
	  if (_G.pairs[pairId].last_type == left->type)
	    {
	      if (_G.pairs[pairId].lit && !strcmp (_G.pairs[pairId].lit, l))
		{
		  if (pairId == PAIR_HL && abs (_G.pairs[pairId].offset - offset) < 3)
		    {
		      adjustPair (pair, &_G.pairs[pairId].offset, offset);
		      return;
		    }
		  if (pairId == PAIR_IY && abs (offset) < 127)
		    {
		      return;
		    }
		}
	    }
	}
      _G.pairs[pairId].last_type = left->type;
      _G.pairs[pairId].lit = gc_strdup (l);
      _G.pairs[pairId].offset = offset;
    }
  if (IS_GB && pairId == PAIR_DE && 0)
    {
      if (_G.pairs[pairId].lit && !strcmp (_G.pairs[pairId].lit, l))
	{
	  if (abs (_G.pairs[pairId].offset - offset) < 3)
	    {
	      adjustPair (pair, &_G.pairs[pairId].offset, offset);
	      return;
	    }
	}
      _G.pairs[pairId].last_type = left->type;
      _G.pairs[pairId].lit = gc_strdup (l);
      _G.pairs[pairId].offset = offset;
    }
  /* Both a lit on the right and a true symbol on the left */
  if (offset)
    emit2 ("ld %s,!hashedstr + %u", pair, l, offset);
  else
    emit2 ("ld %s,!hashedstr", pair, l);
}

static void
fetchPairLong (PAIR_ID pairId, asmop * aop, int offset)
{
    /* if this is remateriazable */
    if (isLitWord (aop)) {
        fetchLitPair (pairId, aop, offset);
    }
    else {
        /* we need to get it byte by byte */
        if (pairId == PAIR_HL && IS_GB && requiresHL (aop)) {
            aopGet (aop, offset, FALSE);
            switch (aop->size) {
            case 1:
                emit2 ("ld l,!*hl");
                emit2 ("ld h,!immedbyte", 0);
                            break;
            case 2:
                emit2 ("!ldahli");
                emit2 ("ld h,!*hl");
                emit2 ("ld l,a");
                break;
            default:
                emit2 ("; WARNING: mlh woosed out.  This code is invalid.");
            }
        }
        else if (IS_Z80 && aop->type == AOP_IY) {
            /* Instead of fetching relative to IY, just grab directly
               from the address IY refers to */
            char *l = aopGetLitWordLong (aop, offset, FALSE);
            wassert (l);
            emit2 ("ld %s,(%s)", _pairs[pairId].name, l);

            if (aop->size < 2) {
                emit2("ld %s,!zero", _pairs[pairId].h);
            }
        }
        else {
            emit2 ("ld %s,%s", _pairs[pairId].l, aopGet (aop, offset, FALSE));
            emit2 ("ld %s,%s", _pairs[pairId].h, aopGet (aop, offset + 1, FALSE));
        }
        /* PENDING: check? */
        if (pairId == PAIR_HL)
            spillPair (PAIR_HL);
    }
}

static void
fetchPair (PAIR_ID pairId, asmop * aop)
{
  fetchPairLong (pairId, aop, 0);
}

static void
fetchHL (asmop * aop)
{
  fetchPair (PAIR_HL, aop);
}

static void
setupPair (PAIR_ID pairId, asmop * aop, int offset)
{
  assert (pairId == PAIR_HL || pairId == PAIR_IY);

  switch (aop->type)
    {
    case AOP_IY:
      fetchLitPair (pairId, aop, 0);
      break;
    case AOP_HL:
      fetchLitPair (pairId, aop, offset);
      _G.pairs[pairId].offset = offset;
      break;
    case AOP_STK:
      {
	/* Doesnt include _G.stack.pushed */
	int abso = aop->aopu.aop_stk + offset + _G.stack.offset;
	if (aop->aopu.aop_stk > 0)
	  {
	    abso += _G.stack.param_offset;
	  }
	assert (pairId == PAIR_HL);
	/* In some cases we can still inc or dec hl */
	if (_G.pairs[pairId].last_type == AOP_STK && abs (_G.pairs[pairId].offset - abso) < 3)
	  {
	    adjustPair (_pairs[pairId].name, &_G.pairs[pairId].offset, abso);
	  }
	else
	  {
	    emit2 ("!ldahlsp", abso + _G.stack.pushed);
	  }
	_G.pairs[pairId].offset = abso;
	break;
      }
    default:
      wassert (0);
    }
  _G.pairs[pairId].last_type = aop->type;
}

static void
emitLabel (int key)
{
  emit2 ("!tlabeldef", key);
  spillCached ();
}

/*-----------------------------------------------------------------*/
/* aopGet - for fetching value of the aop                          */
/*-----------------------------------------------------------------*/
static const char *
aopGet (asmop * aop, int offset, bool bit16)
{
  char *s = buffer;

  /* offset is greater than size then zero */
  /* PENDING: this seems a bit screwed in some pointer cases. */
  if (offset > (aop->size - 1) &&
      aop->type != AOP_LIT) 
    {
      tsprintf (s, "!zero");
      return gc_strdup(s);
    }

  /* depending on type */
  switch (aop->type)
    {
    case AOP_IMMD:
      /* PENDING: re-target */
      if (bit16)
	tsprintf (s, "!immedwords", aop->aopu.aop_immd);
      else
	switch (offset)
	  {
	  case 2:
	    tsprintf (s, "!bankimmeds", aop->aopu.aop_immd);
	    break;
	  case 1:
	    tsprintf (s, "!msbimmeds", aop->aopu.aop_immd);
	    break;
	  case 0:
	    tsprintf (s, "!lsbimmeds", aop->aopu.aop_immd);
	    break;
	  default:
	    wassert (0);
	  }

      return gc_strdup(s);

    case AOP_DIR:
      wassert (IS_GB);
      emit2 ("ld a,(%s+%d) ; x", aop->aopu.aop_dir, offset);
      sprintf (s, "a");

      return gc_strdup(s);

    case AOP_SFR:
      wassert (IS_GB);
      emit2 ("ldh a,(%s+%d) ; x", aop->aopu.aop_dir, offset);
      sprintf (s, "a");

      return gc_strdup(s);

    case AOP_REG:
      return aop->aopu.aop_reg[offset]->name;

    case AOP_HL:
      wassert (IS_GB);
      setupPair (PAIR_HL, aop, offset);
      tsprintf (s, "!*hl");

      return gc_strdup (s);

    case AOP_IY:
      wassert (IS_Z80);
      setupPair (PAIR_IY, aop, offset);
      tsprintf (s, "!*iyx", offset);

      return gc_strdup(s);

    case AOP_STK:
      if (IS_GB)
	{
	  setupPair (PAIR_HL, aop, offset);
	  tsprintf (s, "!*hl");
	}
      else
	{
	  if (aop->aopu.aop_stk >= 0)
	    offset += _G.stack.param_offset;
	  tsprintf (s, "!*ixx ; x", aop->aopu.aop_stk + offset);
	}

      return gc_strdup(s);

    case AOP_CRY:
      wassert (0);

    case AOP_ACC:
      if (!offset)
	{
	  return "a";
	}
      else
        {
          tsprintf(s, "!zero");
          return gc_strdup(s);
        }

    case AOP_HLREG:
      wassert (offset < 2);
      return aop->aopu.aop_str[offset];

    case AOP_LIT:
      return aopLiteral (aop->aopu.aop_lit, offset);

    case AOP_STR:
      aop->coff = offset;
      return aop->aopu.aop_str[offset];

    default:
      break;
    }
  wassertl (0, "aopget got unsupported aop->type");
  exit (0);
}

bool
isRegString (const char *s)
{
  if (!strcmp (s, "b") ||
      !strcmp (s, "c") ||
      !strcmp (s, "d") ||
      !strcmp (s, "e") ||
      !strcmp (s, "a") ||
      !strcmp (s, "h") ||
      !strcmp (s, "l"))
    return TRUE;
  return FALSE;
}

bool
isConstant (const char *s)
{
  /* This is a bit of a hack... */
  return (*s == '#' || *s == '$');
}

bool
canAssignToPtr (const char *s)
{
  if (isRegString (s))
    return TRUE;
  if (isConstant (s))
    return TRUE;
  return FALSE;
}

/*-----------------------------------------------------------------*/
/* aopPut - puts a string for a aop                                */
/*-----------------------------------------------------------------*/
static void
aopPut (asmop * aop, const char *s, int offset)
{
  char buffer2[256];

  if (aop->size && offset > (aop->size - 1))
    {
      werror (E_INTERNAL_ERROR, __FILE__, __LINE__,
	      "aopPut got offset > aop->size");
      exit (0);
    }

  // PENDING
  tsprintf(buffer2, s);
  s = buffer2;

  /* will assign value to value */
  /* depending on where it is ofcourse */
  switch (aop->type)
    {
    case AOP_DIR:
      /* Direct.  Hmmm. */
      wassert (IS_GB);
      if (strcmp (s, "a"))
	emit2 ("ld a,%s", s);
      emit2 ("ld (%s+%d),a", aop->aopu.aop_dir, offset);
      break;

    case AOP_SFR:
      wassert (IS_GB);
      if (strcmp (s, "a"))
	emit2 ("ld a,%s", s);
      emit2 ("ldh (%s+%d),a", aop->aopu.aop_dir, offset);
      break;

    case AOP_REG:
      if (!strcmp (s, "!*hl"))
	emit2 ("ld %s,!*hl", aop->aopu.aop_reg[offset]->name);
      else
	emit2 ("ld %s,%s",
	       aop->aopu.aop_reg[offset]->name, s);
      break;

    case AOP_IY:
      wassert (!IS_GB);
      setupPair (PAIR_IY, aop, offset);
      if (!canAssignToPtr (s))
	{
	  emit2 ("ld a,%s", s);
	  emit2 ("ld !*iyx,a", offset);
	}
      else
	emit2 ("ld !*iyx,%s", offset, s);
      break;

    case AOP_HL:
      wassert (IS_GB);
      /* PENDING: for re-target */
      if (!strcmp (s, "!*hl") || !strcmp (s, "(hl)") || !strcmp (s, "[hl]"))
	{
	  emit2 ("ld a,!*hl");
	  s = "a";
	}
      setupPair (PAIR_HL, aop, offset);

      emit2 ("ld !*hl,%s", s);
      break;

    case AOP_STK:
      if (IS_GB)
	{
	  /* PENDING: re-target */
	  if (!strcmp (s, "!*hl") || !strcmp (s, "(hl)") || !strcmp (s, "[hl]"))
	    {
	      emit2 ("ld a,!*hl");
	      s = "a";
	    }
	  setupPair (PAIR_HL, aop, offset);
	  if (!canAssignToPtr (s))
	    {
	      emit2 ("ld a,%s", s);
	      emit2 ("ld !*hl,a");
	    }
	  else
	    emit2 ("ld !*hl,%s", s);
	}
      else
	{
	  if (aop->aopu.aop_stk >= 0)
	    offset += _G.stack.param_offset;
	  if (!canAssignToPtr (s))
	    {
	      emit2 ("ld a,%s", s);
	      emit2 ("ld !*ixx,a", aop->aopu.aop_stk + offset);
	    }
	  else
	    emit2 ("ld !*ixx,%s", aop->aopu.aop_stk + offset, s);
	}
      break;

    case AOP_CRY:
      /* if bit variable */
      if (!aop->aopu.aop_dir)
	{
	  emit2 ("ld a,#0");
	  emit2 ("rla");
	}
      else
	{
	  /* In bit space but not in C - cant happen */
	  wassert (0);
	}
      break;

    case AOP_STR:
      aop->coff = offset;
      if (strcmp (aop->aopu.aop_str[offset], s))
	{
	  emit2 ("ld %s,%s", aop->aopu.aop_str[offset], s);
	}
      break;

    case AOP_ACC:
      aop->coff = offset;
      if (!offset && (strcmp (s, "acc") == 0))
	break;
      if (offset > 0)
	{

	  emit2 ("; Error aopPut AOP_ACC");
	}
      else
	{
	  if (strcmp (aop->aopu.aop_str[offset], s))
	    emit2 ("ld %s,%s", aop->aopu.aop_str[offset], s);
	}
      break;

    case AOP_HLREG:
      wassert (offset < 2);
      emit2 ("ld %s,%s", aop->aopu.aop_str[offset], s);
      break;

    default:
      werror (E_INTERNAL_ERROR, __FILE__, __LINE__,
	      "aopPut got unsupported aop->type");
      exit (0);
    }
}

#define AOP(op) op->aop
#define AOP_TYPE(op) AOP(op)->type
#define AOP_SIZE(op) AOP(op)->size
#define AOP_NEEDSACC(x) (AOP(x) && (AOP_TYPE(x) == AOP_CRY))

static void
commitPair (asmop * aop, PAIR_ID id)
{
  if (id == PAIR_HL && requiresHL (aop))
    {
      emit2 ("ld a,l");
      emit2 ("ld d,h");
      aopPut (aop, "a", 0);
      aopPut (aop, "d", 1);
    }
  else
    {
      aopPut (aop, _pairs[id].l, 0);
      aopPut (aop, _pairs[id].h, 1);
    }
}

/*-----------------------------------------------------------------*/
/* getDataSize - get the operand data size                         */
/*-----------------------------------------------------------------*/
int
getDataSize (operand * op)
{
  int size;
  size = AOP_SIZE (op);
  if (size == 3)
    {
      /* pointer */
      wassert (0);
    }
  return size;
}

/*-----------------------------------------------------------------*/
/* movLeft2Result - move byte from left to result                  */
/*-----------------------------------------------------------------*/
static void
movLeft2Result (operand * left, int offl,
		operand * result, int offr, int sign)
{
    const char *l;
  if (!sameRegs (AOP (left), AOP (result)) || (offl != offr))
    {
      l = aopGet (AOP (left), offl, FALSE);

      if (!sign)
	{
	  aopPut (AOP (result), l, offr);
	}
      else
	{
	  wassert (0);
	}
    }
}


/** Put Acc into a register set
 */
void
outAcc (operand * result)
{
  int size, offset;
  size = getDataSize (result);
  if (size)
    {
      aopPut (AOP (result), "a", 0);
      size--;
      offset = 1;
      /* unsigned or positive */
      while (size--)
	{
	  aopPut (AOP (result), "!zero", offset++);
	}
    }
}

/** Take the value in carry and put it into a register
 */
void
outBitCLong (operand * result, bool swap_sense)
{
  /* if the result is bit */
  if (AOP_TYPE (result) == AOP_CRY)
    {
      emit2 ("; Note: outBitC form 1");
      aopPut (AOP (result), "blah", 0);
    }
  else
    {
      emit2 ("ld a,!zero");
      emit2 ("rla");
      if (swap_sense)
	emit2 ("xor a,!immedbyte", 1);
      outAcc (result);
    }
}

void
outBitC (operand * result)
{
  outBitCLong (result, FALSE);
}

/*-----------------------------------------------------------------*/
/* toBoolean - emit code for orl a,operator(sizeop)                */
/*-----------------------------------------------------------------*/
void
toBoolean (operand * oper)
{
  int size = AOP_SIZE (oper);
  int offset = 0;
  if (size > 1)
    {
      emit2 ("ld a,%s", aopGet (AOP (oper), offset++, FALSE));
      size--;
      while (size--)
	emit2 ("or a,%s", aopGet (AOP (oper), offset++, FALSE));
    }
  else
    {
      if (AOP (oper)->type != AOP_ACC)
	{
	  _clearCarry();
	  emit2 ("or a,%s", aopGet (AOP (oper), 0, FALSE));
	}
    }
}

/*-----------------------------------------------------------------*/
/* genNot - generate code for ! operation                          */
/*-----------------------------------------------------------------*/
static void
genNot (iCode * ic)
{
  sym_link *optype = operandType (IC_LEFT (ic));

  /* assign asmOps to operand & result */
  aopOp (IC_LEFT (ic), ic, FALSE, TRUE);
  aopOp (IC_RESULT (ic), ic, TRUE, FALSE);

  /* if in bit space then a special case */
  if (AOP_TYPE (IC_LEFT (ic)) == AOP_CRY)
    {
      wassert (0);
    }

  /* if type float then do float */
  if (IS_FLOAT (optype))
    {
      wassert (0);
    }

  toBoolean (IC_LEFT (ic));

  /* Not of A:
     If A == 0, !A = 1
     else A = 0
     So if A = 0, A-1 = 0xFF and C is set, rotate C into reg. */
  emit2 ("sub a,!one");
  outBitC (IC_RESULT (ic));

  /* release the aops */
  freeAsmop (IC_LEFT (ic), NULL, ic);
  freeAsmop (IC_RESULT (ic), NULL, ic);
}

/*-----------------------------------------------------------------*/
/* genCpl - generate code for complement                           */
/*-----------------------------------------------------------------*/
static void
genCpl (iCode * ic)
{
  int offset = 0;
  int size;


  /* assign asmOps to operand & result */
  aopOp (IC_LEFT (ic), ic, FALSE, FALSE);
  aopOp (IC_RESULT (ic), ic, TRUE, FALSE);

  /* if both are in bit space then
     a special case */
  if (AOP_TYPE (IC_RESULT (ic)) == AOP_CRY &&
      AOP_TYPE (IC_LEFT (ic)) == AOP_CRY)
    {
      wassert (0);
    }

  size = AOP_SIZE (IC_RESULT (ic));
  while (size--)
    {
      const char *l = aopGet (AOP (IC_LEFT (ic)), offset, FALSE);
      _moveA (l);
      emit2("cpl");
      aopPut (AOP (IC_RESULT (ic)), "a", offset++);
    }

  /* release the aops */
  freeAsmop (IC_LEFT (ic), NULL, ic);
  freeAsmop (IC_RESULT (ic), NULL, ic);
}

/*-----------------------------------------------------------------*/
/* genUminus - unary minus code generation                         */
/*-----------------------------------------------------------------*/
static void
genUminus (iCode * ic)
{
  int offset, size;
  sym_link *optype, *rtype;

  /* assign asmops */
  aopOp (IC_LEFT (ic), ic, FALSE, FALSE);
  aopOp (IC_RESULT (ic), ic, TRUE, FALSE);

  /* if both in bit space then special
     case */
  if (AOP_TYPE (IC_RESULT (ic)) == AOP_CRY &&
      AOP_TYPE (IC_LEFT (ic)) == AOP_CRY)
    {
      wassert (0);
      goto release;
    }

  optype = operandType (IC_LEFT (ic));
  rtype = operandType (IC_RESULT (ic));

  /* if float then do float stuff */
  if (IS_FLOAT (optype))
    {
      wassert (0);
      goto release;
    }

  /* otherwise subtract from zero */
  size = AOP_SIZE (IC_LEFT (ic));
  offset = 0;
  _clearCarry();
  while (size--)
    {
      const char *l = aopGet (AOP (IC_LEFT (ic)), offset, FALSE);
      emit2 ("ld a,!zero");
      emit2 ("sbc a,%s", l);
      aopPut (AOP (IC_RESULT (ic)), "a", offset++);
    }

  /* if any remaining bytes in the result */
  /* we just need to propagate the sign   */
  if ((size = (AOP_SIZE (IC_RESULT (ic)) - AOP_SIZE (IC_LEFT (ic)))))
    {
      emit2 ("rlc a");
      emit2 ("sbc a,a");
      while (size--)
	aopPut (AOP (IC_RESULT (ic)), "a", offset++);
    }

release:
  /* release the aops */
  freeAsmop (IC_LEFT (ic), NULL, ic);
  freeAsmop (IC_RESULT (ic), NULL, ic);
}

static void
_push (PAIR_ID pairId)
{
  emit2 ("push %s", _pairs[pairId].name);
  _G.stack.pushed += 2;
}

static void
_pop (PAIR_ID pairId)
{
  emit2 ("pop %s", _pairs[pairId].name);
  _G.stack.pushed -= 2;
}


/*-----------------------------------------------------------------*/
/* assignResultValue -               */
/*-----------------------------------------------------------------*/
void
assignResultValue (operand * oper)
{
  int size = AOP_SIZE (oper);
  bool topInA = 0;

  wassert (size <= 4);
  topInA = requiresHL (AOP (oper));

#if 0
  if (!IS_GB)
    wassert (size <= 2);
#endif
  if (IS_GB && size == 4 && requiresHL (AOP (oper)))
    {
      /* We do it the hard way here. */
      _push (PAIR_HL);
      aopPut (AOP (oper), _fReturn[0], 0);
      aopPut (AOP (oper), _fReturn[1], 1);
      emit2 ("pop de");
      _G.stack.pushed -= 2;
      aopPut (AOP (oper), _fReturn[0], 2);
      aopPut (AOP (oper), _fReturn[1], 3);
    }
  else
    {
      while (size--)
	{
	  aopPut (AOP (oper), _fReturn[size], size);
	}
    }
}

static void
_saveRegsForCall(iCode *ic, int sendSetSize)
{
  /* Rules:
      o Stack parameters are pushed before this function enters
      o DE and BC may be used in this function.
      o HL and DE may be used to return the result.
      o HL and DE may be used to send variables.
      o DE and BC may be used to store the result value.
      o HL may be used in computing the sent value of DE
      o The iPushes for other parameters occur before any addSets

     Logic: (to be run inside the first iPush or if none, before sending)
      o Compute if DE and/or BC are in use over the call
      o Compute if DE is used in the send set
      o Compute if DE and/or BC are used to hold the result value
      o If (DE is used, or in the send set) and is not used in the result, push.
      o If BC is used and is not in the result, push
      o 
      o If DE is used in the send set, fetch
      o If HL is used in the send set, fetch
      o Call
      o ...
  */
  if (_G.saves.saved == FALSE) {
    bool deInUse, bcInUse;
    bool deSending;
    bool bcInRet = FALSE, deInRet = FALSE;
    bitVect *rInUse;

    if (IC_RESULT(ic))
      {
        rInUse = bitVectCplAnd (bitVectCopy (ic->rMask), z80_rUmaskForOp (IC_RESULT(ic)));
      }
    else 
      {
        /* Has no result, so in use is all of in use */
        rInUse = ic->rMask;
      }

    deInUse = bitVectBitValue (rInUse, D_IDX) || bitVectBitValue(rInUse, E_IDX);
    bcInUse = bitVectBitValue (rInUse, B_IDX) || bitVectBitValue(rInUse, C_IDX);

    deSending = (sendSetSize > 1);

    emit2 ("; _saveRegsForCall: sendSetSize: %u deInUse: %u bcInUse: %u deSending: %u", sendSetSize, deInUse, bcInUse, deSending);

    if (bcInUse && bcInRet == FALSE) {
      _push(PAIR_BC);
      _G.stack.pushedBC = TRUE;
    }
    if (deInUse && deInRet == FALSE) {
      _push(PAIR_DE);
      _G.stack.pushedDE = TRUE;
    }

    _G.saves.saved = TRUE;
  }
  else {
    /* Already saved. */
  }
}

/*-----------------------------------------------------------------*/
/* genIpush - genrate code for pushing this gets a little complex  */
/*-----------------------------------------------------------------*/
static void
genIpush (iCode * ic)
{
  int size, offset = 0;
  const char *l;

  /* if this is not a parm push : ie. it is spill push
     and spill push is always done on the local stack */
  if (!ic->parmPush)
    {
      wassertl(0, "Encountered an unsupported spill push.");
#if 0
      /* and the item is spilt then do nothing */
      if (OP_SYMBOL (IC_LEFT (ic))->isspilt)
	return;

      aopOp (IC_LEFT (ic), ic, FALSE, FALSE);
      size = AOP_SIZE (IC_LEFT (ic));
      /* push it on the stack */
      if (isPair (AOP (IC_LEFT (ic))))
	{
	  emit2 ("push %s", getPairName (AOP (IC_LEFT (ic))));
	  _G.stack.pushed += 2;
	}
      else
	{
	  offset = size;
	  while (size--)
	    {
	      /* Simple for now - load into A and PUSH AF */
	      if (AOP (IC_LEFT (ic))->type == AOP_IY)
		{
		  char *l = aopGetLitWordLong (AOP (IC_LEFT (ic)), --offset, FALSE);
		  wassert (l);
		  emit2 ("ld a,(%s)", l);
		}
	      else
		{
		  l = aopGet (AOP (IC_LEFT (ic)), --offset, FALSE);
		  emit2 ("ld a,%s", l);
		}
	      emit2 ("push af");
	      emit2 ("inc sp");
	      _G.stack.pushed++;
	    }
	}
#endif
      return;
    }

  if (_G.saves.saved == FALSE) {
    /* Caller saves, and this is the first iPush. */
    /* Scan ahead until we find the function that we are pushing parameters to.
       Count the number of addSets on the way to figure out what registers
       are used in the send set.
    */
    int nAddSets = 0;
    iCode *walk = ic->next;
    
    while (walk) {
      if (walk->op == SEND) {
        nAddSets++;
      }
      else if (walk->op == CALL || walk->op == PCALL) {
        /* Found it. */
        break;
      }
      else {
        /* Keep looking. */
      }
      walk = walk->next;
    }
    _saveRegsForCall(walk, nAddSets);
  }
  else {
    /* Already saved by another iPush. */
  }

  /* then do the push */
  aopOp (IC_LEFT (ic), ic, FALSE, FALSE);

  size = AOP_SIZE (IC_LEFT (ic));

  if (isPair (AOP (IC_LEFT (ic))))
    {
      _G.stack.pushed += 2;
      emit2 ("push %s", getPairName (AOP (IC_LEFT (ic))));
    }
  else
    {
      if (size == 2)
	{
	  fetchHL (AOP (IC_LEFT (ic)));
	  emit2 ("push hl");
	  spillPair (PAIR_HL);
	  _G.stack.pushed += 2;
	  goto release;
	}
      if (size == 4)
	{
	  fetchPairLong (PAIR_HL, AOP (IC_LEFT (ic)), 2);
	  emit2 ("push hl");
	  spillPair (PAIR_HL);
	  _G.stack.pushed += 2;
	  fetchPairLong (PAIR_HL, AOP (IC_LEFT (ic)), 0);
	  emit2 ("push hl");
	  spillPair (PAIR_HL);
	  _G.stack.pushed += 2;
	  goto release;
	}
      offset = size;
      while (size--)
	{
	  if (AOP (IC_LEFT (ic))->type == AOP_IY)
	    {
	      char *l = aopGetLitWordLong (AOP (IC_LEFT (ic)), --offset, FALSE);
	      wassert (l);
	      emit2 ("ld a,(%s)", l);
	    }
	  else
	    {
	      l = aopGet (AOP (IC_LEFT (ic)), --offset, FALSE);
	      emit2 ("ld a,%s", l);
	    }
	  emit2 ("push af");
	  emit2 ("inc sp");
	  _G.stack.pushed++;
	}
    }
release:
  freeAsmop (IC_LEFT (ic), NULL, ic);
}

/*-----------------------------------------------------------------*/
/* genIpop - recover the registers: can happen only for spilling   */
/*-----------------------------------------------------------------*/
static void
genIpop (iCode * ic)
{
  int size, offset;


  /* if the temp was not pushed then */
  if (OP_SYMBOL (IC_LEFT (ic))->isspilt)
    return;

  aopOp (IC_LEFT (ic), ic, FALSE, FALSE);
  size = AOP_SIZE (IC_LEFT (ic));
  offset = (size - 1);
  if (isPair (AOP (IC_LEFT (ic))))
    {
      emit2 ("pop %s", getPairName (AOP (IC_LEFT (ic))));
    }
  else
    {
      while (size--)
	{
	  emit2 ("dec sp");
	  emit2 ("pop hl");
	  spillPair (PAIR_HL);
	  aopPut (AOP (IC_LEFT (ic)), "l", offset--);
	}
    }

  freeAsmop (IC_LEFT (ic), NULL, ic);
}

/* This is quite unfortunate */
static void
setArea (int inHome)
{
  /*
    static int lastArea = 0;

     if (_G.in_home != inHome) {
     if (inHome) {
     const char *sz = port->mem.code_name;
     port->mem.code_name = "HOME";
     emit2("!area", CODE_NAME);
     port->mem.code_name = sz;
     }
     else
     emit2("!area", CODE_NAME); */
  _G.in_home = inHome;
  //    }
}

static bool
isInHome (void)
{
  return _G.in_home;
}

static int
_opUsesPair (operand * op, iCode * ic, PAIR_ID pairId)
{
  int ret = 0;
  asmop *aop;
  symbol *sym = OP_SYMBOL (op);

  if (sym->isspilt || sym->nRegs == 0)
    return 0;

  aopOp (op, ic, FALSE, FALSE);

  aop = AOP (op);
  if (aop->type == AOP_REG)
    {
      int i;
      for (i = 0; i < aop->size; i++)
	{
	  if (pairId == PAIR_DE)
	    {
	      emit2 ("; name %s", aop->aopu.aop_reg[i]->name);
	      if (!strcmp (aop->aopu.aop_reg[i]->name, "e"))
		ret++;
	      if (!strcmp (aop->aopu.aop_reg[i]->name, "d"))
		ret++;
	    }
          else if (pairId == PAIR_BC)
            {
	      emit2 ("; name %s", aop->aopu.aop_reg[i]->name);
	      if (!strcmp (aop->aopu.aop_reg[i]->name, "c"))
		ret++;
	      if (!strcmp (aop->aopu.aop_reg[i]->name, "b"))
		ret++;
            }
	  else
	    {
	      wassert (0);
	    }
	}
    }

  freeAsmop (IC_LEFT (ic), NULL, ic);
  return ret;
}

/** Emit the code for a call statement
 */
static void
emitCall (iCode * ic, bool ispcall)
{
  sym_link *detype = getSpec (operandType (IC_LEFT (ic)));

  /* if caller saves & we have not saved then */
  if (!ic->regsSaved)
    {
      /* PENDING */
    }

  _saveRegsForCall(ic, _G.sendSet ? elementsInSet(_G.sendSet) : 0);

  /* if send set is not empty then assign */
  if (_G.sendSet)
    {
      iCode *sic;
      int send = 0;
      int nSend = elementsInSet(_G.sendSet);
      bool swapped = FALSE;

      int _z80_sendOrder[] = {
        PAIR_BC, PAIR_DE
      };

      if (nSend > 1) {
        /* Check if the parameters are swapped.  If so route through hl instead. */
        wassertl (nSend == 2, "Pedantic check.  Code only checks for the two send items case.");

        sic = setFirstItem(_G.sendSet);
        sic = setNextItem(_G.sendSet);

        if (_opUsesPair (IC_LEFT(sic), sic, _z80_sendOrder[0])) {
          /* The second send value is loaded from one the one that holds the first
             send, i.e. it is overwritten. */
          /* Cache the first in HL, and load the second from HL instead. */
          emit2 ("ld h,%s", _pairs[_z80_sendOrder[0]].h);
          emit2 ("ld l,%s", _pairs[_z80_sendOrder[0]].l);

          swapped = TRUE;
        }
      }

      for (sic = setFirstItem (_G.sendSet); sic;
           sic = setNextItem (_G.sendSet))
        {
          int size;
          aopOp (IC_LEFT (sic), sic, FALSE, FALSE);

          size = AOP_SIZE (IC_LEFT (sic));
          wassertl (size <= 2, "Tried to send a parameter that is bigger than two bytes");
          wassertl (_z80_sendOrder[send] != PAIR_INVALID, "Tried to send more parameters than we have registers for");

          // PENDING: Mild hack
          if (swapped == TRUE && send == 1) {
            if (size > 1) {
              emit2 ("ld %s,h", _pairs[_z80_sendOrder[send]].h);
            }
            else {
              emit2 ("ld %s,!zero", _pairs[_z80_sendOrder[send]].h);
            }
            emit2 ("ld %s,l", _pairs[_z80_sendOrder[send]].l);
          }
          else {
            fetchPair(_z80_sendOrder[send], AOP (IC_LEFT (sic)));
          }

          send++;
          freeAsmop (IC_LEFT (sic), NULL, sic);
        }
      _G.sendSet = NULL;
    }

  if (ispcall)
    {
      if (IS_BANKEDCALL (detype))
	{
	  werror (W_INDIR_BANKED);
	}
      aopOp (IC_LEFT (ic), ic, FALSE, FALSE);

      if (isLitWord (AOP (IC_LEFT (ic))))
	{
	  emit2 ("; Special case where the pCall is to a constant");
	  emit2 ("call %s", aopGetLitWordLong (AOP (IC_LEFT (ic)), 0, FALSE));
	}
      else
	{
	  symbol *rlbl = newiTempLabel (NULL);
	  spillPair (PAIR_HL);
	  emit2 ("ld hl,!immed!tlabel", (rlbl->key + 100));
	  emit2 ("push hl");
	  _G.stack.pushed += 2;

	  fetchHL (AOP (IC_LEFT (ic)));
	  emit2 ("jp !*hl");
	  emit2 ("!tlabeldef", (rlbl->key + 100));
	  _G.stack.pushed -= 2;
	}
      freeAsmop (IC_LEFT (ic), NULL, ic);
    }
  else
    {
      char *name = OP_SYMBOL (IC_LEFT (ic))->rname[0] ?
      OP_SYMBOL (IC_LEFT (ic))->rname :
      OP_SYMBOL (IC_LEFT (ic))->name;
      if (IS_BANKEDCALL (detype))
	{
	  emit2 ("call banked_call");
	  emit2 ("!dws", name);
	  emit2 ("!dw !bankimmeds", name);
	}
      else
	{
	  /* make the call */
	  emit2 ("call %s", name);
	}
    }
  spillCached ();

  /* Mark the regsiters as restored. */
  _G.saves.saved = FALSE;

  /* if we need assign a result value */
  if ((IS_ITEMP (IC_RESULT (ic)) &&
       (OP_SYMBOL (IC_RESULT (ic))->nRegs ||
	OP_SYMBOL (IC_RESULT (ic))->spildir)) ||
      IS_TRUE_SYMOP (IC_RESULT (ic)))
    {

      aopOp (IC_RESULT (ic), ic, FALSE, FALSE);

      assignResultValue (IC_RESULT (ic));

      freeAsmop (IC_RESULT (ic), NULL, ic);
    }

  /* adjust the stack for parameters if required */
  if (ic->parmBytes)
    {
      int i = ic->parmBytes;

      _G.stack.pushed -= i;
      if (IS_GB)
	{
	  emit2 ("!ldaspsp", i);
	}
      else
	{
	  spillCached ();
	  if (i > 6)
	    {
	      emit2 ("ld hl,#%d", i);
	      emit2 ("add hl,sp");
	      emit2 ("ld sp,hl");
	    }
	  else
	    {
	      while (i > 1)
		{
		  emit2 ("pop hl");
		  i -= 2;
		}
	      if (i)
		emit2 ("inc sp");
	    }
	  spillCached ();
	}
    }

  if (_G.stack.pushedDE) 
    {
      _pop(PAIR_DE);
      _G.stack.pushedDE = FALSE;
    }
  
  if (_G.stack.pushedBC) 
    {
      _pop(PAIR_BC);
      _G.stack.pushedBC = FALSE;
    }
}

/*-----------------------------------------------------------------*/
/* genCall - generates a call statement                            */
/*-----------------------------------------------------------------*/
static void
genCall (iCode * ic)
{
  emitCall (ic, FALSE);
}

/*-----------------------------------------------------------------*/
/* genPcall - generates a call by pointer statement                */
/*-----------------------------------------------------------------*/
static void
genPcall (iCode * ic)
{
  emitCall (ic, TRUE);
}

/*-----------------------------------------------------------------*/
/* resultRemat - result  is rematerializable                       */
/*-----------------------------------------------------------------*/
static int
resultRemat (iCode * ic)
{
  if (SKIP_IC (ic) || ic->op == IFX)
    return 0;

  if (IC_RESULT (ic) && IS_ITEMP (IC_RESULT (ic)))
    {
      symbol *sym = OP_SYMBOL (IC_RESULT (ic));
      if (sym->remat && !POINTER_SET (ic))
	return 1;
    }

  return 0;
}

extern set *publics;

/* Steps:
    o Check genFunction
    o Check emitCall and clean up
    o Check genReturn
    o Check return puller

    PENDING: Remove this.
*/

/*-----------------------------------------------------------------*/
/* genFunction - generated code for function entry                 */
/*-----------------------------------------------------------------*/
static void
genFunction (iCode * ic)
{
  symbol *sym = OP_SYMBOL (IC_LEFT (ic));
  sym_link *fetype;

#if CALLEE_SAVES
  bool bcInUse = FALSE;
  bool deInUse = FALSE;
#endif

  setArea (IS_NONBANKED (sym->etype));

  /* PENDING: Reset the receive offset as it doesn't seem to get reset anywhere
     else.
  */
  _G.receiveOffset = 0;

#if 0
  /* PENDING: hack */
  if (!IS_STATIC (sym->etype))
    {
      addSetIfnotP (&publics, sym);
    }
#endif

  /* Record the last function name for debugging. */
  _G.lastFunctionName = sym->rname;
  
  /* Create the function header */
  emit2 ("!functionheader", sym->name);
  /* PENDING: portability. */
  emit2 ("__%s_start:", sym->rname);
  emit2 ("!functionlabeldef", sym->rname);

  fetype = getSpec (operandType (IC_LEFT (ic)));

  /* if critical function then turn interrupts off */
  if (SPEC_CRTCL (fetype))
    emit2 ("!di");

  /* if this is an interrupt service routine then save all potentially used registers. */
  if (IS_ISR (sym->etype))
    {
      emit2 ("!pusha");
    }

  /* PENDING: callee-save etc */

  _G.stack.param_offset = 0;

#if CALLEE_SAVES
  /* Detect which registers are used. */
  if (sym->regsUsed)
    {
      int i;
      for (i = 0; i < sym->regsUsed->size; i++)
	{
	  if (bitVectBitValue (sym->regsUsed, i))
	    {
	      switch (i)
		{
		case C_IDX:
		case B_IDX:
                  bcInUse = TRUE;
		  break;
		case D_IDX:
		case E_IDX:
		  if (IS_Z80) {
                    deInUse = TRUE;
                  }
                  else {
                    /* Other systems use DE as a temporary. */
                  }
		  break;
		}
	    }
	}
    }

  if (bcInUse) 
    {
      emit2 ("push bc");
      _G.stack.param_offset += 2;
    }

  _G.stack.pushedBC = bcInUse;

  if (deInUse)
    {
      emit2 ("push de");
      _G.stack.param_offset += 2;
    }

  _G.stack.pushedDE = deInUse;
#endif

  /* adjust the stack for the function */
  _G.stack.last = sym->stack;

  if (sym->stack)
    emit2 ("!enterx", sym->stack);
  else
    emit2 ("!enter");
  _G.stack.offset = sym->stack;
}

/*-----------------------------------------------------------------*/
/* genEndFunction - generates epilogue for functions               */
/*-----------------------------------------------------------------*/
static void
genEndFunction (iCode * ic)
{
  symbol *sym = OP_SYMBOL (IC_LEFT (ic));

  if (IS_ISR (sym->etype))
    {
      wassert (0);
    }
  else
    {
      if (SPEC_CRTCL (sym->etype))
	emit2 ("!ei");

      /* PENDING: calleeSave */

      if (_G.stack.offset)
        {
          emit2 ("!leavex", _G.stack.offset);
        }
      else
        {
          emit2 ("!leave");
        }

#if CALLEE_SAVES
      if (_G.stack.pushedDE) 
        {
          emit2 ("pop de");
          _G.stack.pushedDE = FALSE;
        }

      if (_G.stack.pushedDE) 
        {
          emit2 ("pop bc");
          _G.stack.pushedDE = FALSE;
        }
#endif

      /* Both baned and non-banked just ret */
      emit2 ("ret");

      /* PENDING: portability. */
      emit2 ("__%s_end:", sym->rname);
    }
  _G.flushStatics = 1;
  _G.stack.pushed = 0;
  _G.stack.offset = 0;
}

/*-----------------------------------------------------------------*/
/* genRet - generate code for return statement                     */
/*-----------------------------------------------------------------*/
static void
genRet (iCode * ic)
{
    const char *l;
  /* Errk.  This is a hack until I can figure out how
     to cause dehl to spill on a call */
  int size, offset = 0;

  /* if we have no return value then
     just generate the "ret" */
  if (!IC_LEFT (ic))
    goto jumpret;

  /* we have something to return then
     move the return value into place */
  aopOp (IC_LEFT (ic), ic, FALSE, FALSE);
  size = AOP_SIZE (IC_LEFT (ic));

  if ((size == 2) && ((l = aopGetWord (AOP (IC_LEFT (ic)), 0))))
    {
      if (IS_GB)
	{
	  emit2 ("ld de,%s", l);
	}
      else
	{
	  emit2 ("ld hl,%s", l);
	}
    }
  else
    {
      if (IS_GB && size == 4 && requiresHL (AOP (IC_LEFT (ic))))
	{
	  fetchPair (PAIR_DE, AOP (IC_LEFT (ic)));
	  fetchPairLong (PAIR_HL, AOP (IC_LEFT (ic)), 2);
	}
      else
	{
	  while (size--)
	    {
	      l = aopGet (AOP (IC_LEFT (ic)), offset,
			  FALSE);
	      if (strcmp (_fReturn[offset], l))
		emit2 ("ld %s,%s", _fReturn[offset++], l);
	    }
	}
    }
  freeAsmop (IC_LEFT (ic), NULL, ic);

jumpret:
  /* generate a jump to the return label
     if the next is not the return statement */
  if (!(ic->next && ic->next->op == LABEL &&
	IC_LABEL (ic->next) == returnLabel))

    emit2 ("jp !tlabel", returnLabel->key + 100);
}

/*-----------------------------------------------------------------*/
/* genLabel - generates a label                                    */
/*-----------------------------------------------------------------*/
static void
genLabel (iCode * ic)
{
  /* special case never generate */
  if (IC_LABEL (ic) == entryLabel)
    return;

  emitLabel (IC_LABEL (ic)->key + 100);
}

/*-----------------------------------------------------------------*/
/* genGoto - generates a ljmp                                      */
/*-----------------------------------------------------------------*/
static void
genGoto (iCode * ic)
{
  emit2 ("jp !tlabel", IC_LABEL (ic)->key + 100);
}

/*-----------------------------------------------------------------*/
/* genPlusIncr :- does addition with increment if possible         */
/*-----------------------------------------------------------------*/
static bool
genPlusIncr (iCode * ic)
{
  unsigned int icount;
  unsigned int size = getDataSize (IC_RESULT (ic));
  PAIR_ID resultId = getPairId (AOP (IC_RESULT (ic)));

  /* will try to generate an increment */
  /* if the right side is not a literal
     we cannot */
  if (AOP_TYPE (IC_RIGHT (ic)) != AOP_LIT)
    return FALSE;

  emit2 ("; genPlusIncr");

  icount = (unsigned int) floatFromVal (AOP (IC_RIGHT (ic))->aopu.aop_lit);

  /* If result is a pair */
  if (resultId != PAIR_INVALID)
    {
      if (isLitWord (AOP (IC_LEFT (ic))))
	{
	  fetchLitPair (getPairId (AOP (IC_RESULT (ic))), AOP (IC_LEFT (ic)), icount);
	  return TRUE;
	}
      if (isPair (AOP (IC_LEFT (ic))) && resultId == PAIR_HL && icount > 2)
	{
	  fetchPair (resultId, AOP (IC_RIGHT (ic)));
	  emit2 ("add hl,%s", getPairName (AOP (IC_LEFT (ic))));
	  return TRUE;
	}
      if (icount > 5)
	return FALSE;
      /* Inc a pair */
      if (!sameRegs (AOP (IC_LEFT (ic)), AOP (IC_RESULT (ic))))
	{
	  if (icount > 2)
	    return FALSE;
	  movLeft2Result (IC_LEFT (ic), 0, IC_RESULT (ic), 0, 0);
	  movLeft2Result (IC_LEFT (ic), 1, IC_RESULT (ic), 1, 0);
	}
      while (icount--)
	{
	  emit2 ("inc %s", getPairName (AOP (IC_RESULT (ic))));
	}
      return TRUE;
    }

  /* if the literal value of the right hand side
     is greater than 4 then it is not worth it */
  if (icount > 4)
    return FALSE;

  /* if increment 16 bits in register */
  if (sameRegs (AOP (IC_LEFT (ic)), AOP (IC_RESULT (ic))) &&
      size > 1 &&
      icount == 1
    )
    {
      int offset = 0;
      symbol *tlbl = NULL;
      tlbl = newiTempLabel (NULL);
      while (size--)
	{
	  emit2 ("inc %s", aopGet (AOP (IC_RESULT (ic)), offset++, FALSE));
	  if (size)
	    {
	      emit2 ("!shortjp nz,!tlabel", tlbl->key + 100);
	    }
	}
      emitLabel (tlbl->key + 100);
      return TRUE;
    }

  /* if the sizes are greater than 1 then we cannot */
  if (AOP_SIZE (IC_RESULT (ic)) > 1 ||
      AOP_SIZE (IC_LEFT (ic)) > 1)
    return FALSE;

  /* we can if the aops of the left & result match or
     if they are in registers and the registers are the
     same */
  if (sameRegs (AOP (IC_LEFT (ic)), AOP (IC_RESULT (ic))))
    {
      while (icount--)
	emit2 ("inc %s", aopGet (AOP (IC_LEFT (ic)), 0, FALSE));
      return TRUE;
    }

  return FALSE;
}

/*-----------------------------------------------------------------*/
/* outBitAcc - output a bit in acc                                 */
/*-----------------------------------------------------------------*/
void
outBitAcc (operand * result)
{
  symbol *tlbl = newiTempLabel (NULL);
  /* if the result is a bit */
  if (AOP_TYPE (result) == AOP_CRY)
    {
      wassert (0);
    }
  else
    {
      emit2 ("!shortjp z,!tlabel", tlbl->key + 100);
      emit2 ("ld a,!one");
      emitLabel (tlbl->key + 100);
      outAcc (result);
    }
}

/*-----------------------------------------------------------------*/
/* genPlus - generates code for addition                           */
/*-----------------------------------------------------------------*/
static void
genPlus (iCode * ic)
{
  int size, offset = 0;

  /* special cases :- */

  aopOp (IC_LEFT (ic), ic, FALSE, FALSE);
  aopOp (IC_RIGHT (ic), ic, FALSE, FALSE);
  aopOp (IC_RESULT (ic), ic, TRUE, FALSE);

  /* Swap the left and right operands if:

     if literal, literal on the right or
     if left requires ACC or right is already
     in ACC */

  if ((AOP_TYPE (IC_LEFT (ic)) == AOP_LIT) ||
      (AOP_NEEDSACC (IC_LEFT (ic))) ||
      AOP_TYPE (IC_RIGHT (ic)) == AOP_ACC)
    {
      operand *t = IC_RIGHT (ic);
      IC_RIGHT (ic) = IC_LEFT (ic);
      IC_LEFT (ic) = t;
    }

  /* if both left & right are in bit
     space */
  if (AOP_TYPE (IC_LEFT (ic)) == AOP_CRY &&
      AOP_TYPE (IC_RIGHT (ic)) == AOP_CRY)
    {
      /* Cant happen */
      wassert (0);
    }

  /* if left in bit space & right literal */
  if (AOP_TYPE (IC_LEFT (ic)) == AOP_CRY &&
      AOP_TYPE (IC_RIGHT (ic)) == AOP_LIT)
    {
      /* Can happen I guess */
      wassert (0);
    }

  /* if I can do an increment instead
     of add then GOOD for ME */
  if (genPlusIncr (ic) == TRUE)
    goto release;

  emit2 ("; genPlusIncr failed");

  size = getDataSize (IC_RESULT (ic));

  /* Special case when left and right are constant */
  if (isPair (AOP (IC_RESULT (ic))))
    {
      char *left, *right;

      left = aopGetLitWordLong (AOP (IC_LEFT (ic)), 0, FALSE);
      right = aopGetLitWordLong (AOP (IC_RIGHT (ic)), 0, FALSE);
      if (left && right)
	{
	  /* It's a pair */
	  /* PENDING: fix */
	  char buffer[100];
	  sprintf (buffer, "#(%s + %s)", left, right);
	  emit2 ("ld %s,%s", getPairName (AOP (IC_RESULT (ic))), buffer);
	  goto release;
	}
    }

  if (isPair (AOP (IC_RIGHT (ic))) && getPairId (AOP (IC_RESULT (ic))) == PAIR_HL)
    {
      /* Fetch into HL then do the add */
      spillPair (PAIR_HL);
      fetchPair (PAIR_HL, AOP (IC_LEFT (ic)));
      emit2 ("add hl,%s", getPairName (AOP (IC_RIGHT (ic))));
      goto release;
    }

  /* Special case:
     ld hl,sp+n trashes C so we cant afford to do it during an
     add with stack based varibles.  Worst case is:
     ld  hl,sp+left
     ld  a,(hl)
     ld  hl,sp+right
     add (hl)
     ld  hl,sp+result
     ld  (hl),a
     ld  hl,sp+left+1
     ld  a,(hl)
     ld  hl,sp+right+1
     adc (hl)
     ld  hl,sp+result+1
     ld  (hl),a
     So you cant afford to load up hl if either left, right, or result
     is on the stack (*sigh*)  The alt is:
     ld  hl,sp+left
     ld  de,(hl)
     ld  hl,sp+right
     ld  hl,(hl)
     add hl,de
     ld  hl,sp+result
     ld  (hl),hl
     Combinations in here are:
     * If left or right are in bc then the loss is small - trap later
     * If the result is in bc then the loss is also small
   */
  if (IS_GB)
    {
      if (AOP_TYPE (IC_LEFT (ic)) == AOP_STK ||
	  AOP_TYPE (IC_RIGHT (ic)) == AOP_STK ||
	  AOP_TYPE (IC_RESULT (ic)) == AOP_STK)
	{
	  if ((AOP_SIZE (IC_LEFT (ic)) == 2 ||
	       AOP_SIZE (IC_RIGHT (ic)) == 2) &&
	      (AOP_SIZE (IC_LEFT (ic)) <= 2 &&
	       AOP_SIZE (IC_RIGHT (ic)) <= 2))
	    {
	      if (getPairId (AOP (IC_RIGHT (ic))) == PAIR_BC)
		{
		  /* Swap left and right */
		  operand *t = IC_RIGHT (ic);
		  IC_RIGHT (ic) = IC_LEFT (ic);
		  IC_LEFT (ic) = t;
		}
	      if (getPairId (AOP (IC_LEFT (ic))) == PAIR_BC)
		{
		  fetchPair (PAIR_HL, AOP (IC_RIGHT (ic)));
		  emit2 ("add hl,bc");
		}
	      else
		{
		  fetchPair (PAIR_DE, AOP (IC_LEFT (ic)));
		  fetchPair (PAIR_HL, AOP (IC_RIGHT (ic)));
		  emit2 ("add hl,de");
		}
	      commitPair (AOP (IC_RESULT (ic)), PAIR_HL);
	      goto release;
	    }
	  else if (size == 4)
	    {
	      emit2 ("; WARNING: This add is probably broken.\n");
	    }
	}
    }

  while (size--)
    {
      if (AOP_TYPE (IC_LEFT (ic)) == AOP_ACC)
	{
	  _moveA (aopGet (AOP (IC_LEFT (ic)), offset, FALSE));
	  if (offset == 0)
	    emit2 ("add a,%s",
		   aopGet (AOP (IC_RIGHT (ic)), offset, FALSE));
	  else
	    emit2 ("adc a,%s",
		   aopGet (AOP (IC_RIGHT (ic)), offset, FALSE));
	}
      else
	{
	  _moveA (aopGet (AOP (IC_LEFT (ic)), offset, FALSE));
	  if (offset == 0)
	    emit2 ("add a,%s",
		   aopGet (AOP (IC_RIGHT (ic)), offset, FALSE));
	  else
	    emit2 ("adc a,%s",
		   aopGet (AOP (IC_RIGHT (ic)), offset, FALSE));
	}
      aopPut (AOP (IC_RESULT (ic)), "a", offset++);
    }

release:
  freeAsmop (IC_LEFT (ic), NULL, ic);
  freeAsmop (IC_RIGHT (ic), NULL, ic);
  freeAsmop (IC_RESULT (ic), NULL, ic);

}

/*-----------------------------------------------------------------*/
/* genMinusDec :- does subtraction with deccrement if possible     */
/*-----------------------------------------------------------------*/
static bool
genMinusDec (iCode * ic)
{
  unsigned int icount;
  unsigned int size = getDataSize (IC_RESULT (ic));

  /* will try to generate an increment */
  /* if the right side is not a literal we cannot */
  if (AOP_TYPE (IC_RIGHT (ic)) != AOP_LIT)
    return FALSE;

  /* if the literal value of the right hand side
     is greater than 4 then it is not worth it */
  if ((icount = (unsigned int) floatFromVal (AOP (IC_RIGHT (ic))->aopu.aop_lit)) > 2)
    return FALSE;

  size = getDataSize (IC_RESULT (ic));

#if 0
  /* if increment 16 bits in register */
  if (sameRegs (AOP (IC_LEFT (ic)), AOP (IC_RESULT (ic))) &&
      (size > 1) &&
      (icount == 1))
    {
      symbol *tlbl = newiTempLabel (NULL);
      emit2 ("dec %s", aopGet (AOP (IC_RESULT (ic)), LSB, FALSE));
      emit2 ("jp np," LABEL_STR, tlbl->key + 100);

      emit2 ("dec %s", aopGet (AOP (IC_RESULT (ic)), MSB16, FALSE));
      if (size == 4)
	{
	  wassert (0);
	}
      emitLabel (tlbl->key + 100);
      return TRUE;
    }
#endif

  /* if decrement 16 bits in register */
  if (sameRegs (AOP (IC_LEFT (ic)), AOP (IC_RESULT (ic))) &&
      (size > 1) && isPair (AOP (IC_RESULT (ic))))
    {
      while (icount--)
	emit2 ("dec %s", getPairName (AOP (IC_RESULT (ic))));
      return TRUE;
    }

  /* If result is a pair */
  if (isPair (AOP (IC_RESULT (ic))))
    {
      movLeft2Result (IC_LEFT (ic), 0, IC_RESULT (ic), 0, 0);
      movLeft2Result (IC_LEFT (ic), 1, IC_RESULT (ic), 1, 0);
      while (icount--)
	emit2 ("dec %s", getPairName (AOP (IC_RESULT (ic))));
      return TRUE;
    }

  /* if the sizes are greater than 1 then we cannot */
  if (AOP_SIZE (IC_RESULT (ic)) > 1 ||
      AOP_SIZE (IC_LEFT (ic)) > 1)
    return FALSE;

  /* we can if the aops of the left & result match or if they are in
     registers and the registers are the same */
  if (sameRegs (AOP (IC_LEFT (ic)), AOP (IC_RESULT (ic))))
    {
      while (icount--)
	emit2 ("dec %s", aopGet (AOP (IC_RESULT (ic)), 0, FALSE));
      return TRUE;
    }

  return FALSE;
}

/*-----------------------------------------------------------------*/
/* genMinus - generates code for subtraction                       */
/*-----------------------------------------------------------------*/
static void
genMinus (iCode * ic)
{
  int size, offset = 0;
  unsigned long lit = 0L;

  aopOp (IC_LEFT (ic), ic, FALSE, FALSE);
  aopOp (IC_RIGHT (ic), ic, FALSE, FALSE);
  aopOp (IC_RESULT (ic), ic, TRUE, FALSE);

  /* special cases :- */
  /* if both left & right are in bit space */
  if (AOP_TYPE (IC_LEFT (ic)) == AOP_CRY &&
      AOP_TYPE (IC_RIGHT (ic)) == AOP_CRY)
    {
      wassert (0);
      goto release;
    }

  /* if I can do an decrement instead of subtract then GOOD for ME */
  if (genMinusDec (ic) == TRUE)
    goto release;

  size = getDataSize (IC_RESULT (ic));

  if (AOP_TYPE (IC_RIGHT (ic)) != AOP_LIT)
    {
    }
  else
    {
      lit = (unsigned long) floatFromVal (AOP (IC_RIGHT (ic))->aopu.aop_lit);
      lit = -(long) lit;
    }

  /* Same logic as genPlus */
  if (IS_GB)
    {
      if (AOP_TYPE (IC_LEFT (ic)) == AOP_STK ||
	  AOP_TYPE (IC_RIGHT (ic)) == AOP_STK ||
	  AOP_TYPE (IC_RESULT (ic)) == AOP_STK)
	{
	  if ((AOP_SIZE (IC_LEFT (ic)) == 2 ||
	       AOP_SIZE (IC_RIGHT (ic)) == 2) &&
	      (AOP_SIZE (IC_LEFT (ic)) <= 2 &&
	       AOP_SIZE (IC_RIGHT (ic)) <= 2))
	    {
	      PAIR_ID left = getPairId (AOP (IC_LEFT (ic)));
	      PAIR_ID right = getPairId (AOP (IC_RIGHT (ic)));

	      if (left == PAIR_INVALID && right == PAIR_INVALID)
		{
		  left = PAIR_DE;
		  right = PAIR_HL;
		}
	      else if (right == PAIR_INVALID)
		right = PAIR_DE;
	      else if (left == PAIR_INVALID)
		left = PAIR_DE;

	      fetchPair (left, AOP (IC_LEFT (ic)));
	      /* Order is important.  Right may be HL */
	      fetchPair (right, AOP (IC_RIGHT (ic)));

	      emit2 ("ld a,%s", _pairs[left].l);
	      emit2 ("sub a,%s", _pairs[right].l);
	      emit2 ("ld e,a");
	      emit2 ("ld a,%s", _pairs[left].h);
	      emit2 ("sbc a,%s", _pairs[right].h);

	      aopPut (AOP (IC_RESULT (ic)), "a", 1);
	      aopPut (AOP (IC_RESULT (ic)), "e", 0);
	      goto release;
	    }
	  else if (size == 4)
	    {
	      emit2 ("; WARNING: This sub is probably broken.\n");
	    }
	}
    }

  /* if literal, add a,#-lit, else normal subb */
  while (size--)
    {
      _moveA (aopGet (AOP (IC_LEFT (ic)), offset, FALSE));
      if (AOP_TYPE (IC_RIGHT (ic)) != AOP_LIT)
	{
	  if (!offset)
	    emit2 ("sub a,%s",
		      aopGet (AOP (IC_RIGHT (ic)), offset, FALSE));
	  else
	    emit2 ("sbc a,%s",
		      aopGet (AOP (IC_RIGHT (ic)), offset, FALSE));
	}
      else
	{
	  /* first add without previous c */
	  if (!offset)
	    emit2 ("add a,!immedbyte", (unsigned int) (lit & 0x0FFL));
	  else
	    emit2 ("adc a,!immedbyte", (unsigned int) ((lit >> (offset * 8)) & 0x0FFL));
	}
      aopPut (AOP (IC_RESULT (ic)), "a", offset++);
    }

  if (AOP_SIZE (IC_RESULT (ic)) == 3 &&
      AOP_SIZE (IC_LEFT (ic)) == 3 &&
      !sameRegs (AOP (IC_RESULT (ic)), AOP (IC_LEFT (ic))))
    wassert (0);

release:
  freeAsmop (IC_LEFT (ic), NULL, ic);
  freeAsmop (IC_RIGHT (ic), NULL, ic);
  freeAsmop (IC_RESULT (ic), NULL, ic);
}

/*-----------------------------------------------------------------*/
/* genMult - generates code for multiplication                     */
/*-----------------------------------------------------------------*/
static void
genMult (iCode * ic)
{
  /* Shouldn't occur - all done through function calls */
  wassert (0);
}

/*-----------------------------------------------------------------*/
/* genDiv - generates code for division                            */
/*-----------------------------------------------------------------*/
static void
genDiv (iCode * ic)
{
  /* Shouldn't occur - all done through function calls */
  wassert (0);
}

/*-----------------------------------------------------------------*/
/* genMod - generates code for division                            */
/*-----------------------------------------------------------------*/
static void
genMod (iCode * ic)
{
  /* Shouldn't occur - all done through function calls */
  wassert (0);
}

/*-----------------------------------------------------------------*/
/* genIfxJump :- will create a jump depending on the ifx           */
/*-----------------------------------------------------------------*/
static void
genIfxJump (iCode * ic, char *jval)
{
  symbol *jlbl;
  const char *inst;

  /* if true label then we jump if condition
     supplied is true */
  if (IC_TRUE (ic))
    {
      jlbl = IC_TRUE (ic);
      if (!strcmp (jval, "a"))
	{
	  inst = "nz";
	}
      else if (!strcmp (jval, "c"))
	{
	  inst = "c";
	}
      else if (!strcmp (jval, "nc"))
	{
	  inst = "nc";
	}
      else
	{
	  /* The buffer contains the bit on A that we should test */
	  inst = "nz";
	}
    }
  else
    {
      /* false label is present */
      jlbl = IC_FALSE (ic);
      if (!strcmp (jval, "a"))
	{
	  inst = "z";
	}
      else if (!strcmp (jval, "c"))
	{
	  inst = "nc";
	}
      else if (!strcmp (jval, "nc"))
	{
	  inst = "c";
	}
      else
	{
	  /* The buffer contains the bit on A that we should test */
	  inst = "z";
	}
    }
  /* Z80 can do a conditional long jump */
  if (!strcmp (jval, "a"))
    {
      emit2 ("or a,a");
    }
  else if (!strcmp (jval, "c"))
    {
    }
  else if (!strcmp (jval, "nc"))
    {
    }
  else
    {
      emit2 ("bit %s,a", jval);
    }
  emit2 ("jp %s,!tlabel", inst, jlbl->key + 100);

  /* mark the icode as generated */
  ic->generated = 1;
}

static const char *
_getPairIdName (PAIR_ID id)
{
  return _pairs[id].name;
}

/** Generic compare for > or <
 */
static void
genCmp (operand * left, operand * right,
	operand * result, iCode * ifx, int sign)
{
  int size, offset = 0;
  unsigned long lit = 0L;
  bool swap_sense = FALSE;

  /* if left & right are bit variables */
  if (AOP_TYPE (left) == AOP_CRY &&
      AOP_TYPE (right) == AOP_CRY)
    {
      /* Cant happen on the Z80 */
      wassert (0);
    }
  else
    {
      /* subtract right from left if at the
         end the carry flag is set then we know that
         left is greater than right */
      size = max (AOP_SIZE (left), AOP_SIZE (right));

      /* if unsigned char cmp with lit, just compare */
      if ((size == 1) &&
	  (AOP_TYPE (right) == AOP_LIT && AOP_TYPE (left) != AOP_DIR))
	{
	  emit2 ("ld a,%s", aopGet (AOP (left), offset, FALSE));
	  if (sign)
	    {
	      emit2 ("xor a,!immedbyte", 0x80);
	      emit2 ("cp %s^!constbyte", aopGet (AOP (right), offset, FALSE), 0x80);
	    }
	  else
	    emit2 ("cp %s", aopGet (AOP (right), offset, FALSE));
	}
      else
	{
	  /* Special cases:
	     On the GB:
	     If the left or the right is a lit:
	     Load -lit into HL, add to right via, check sense.
	   */
	  if (size == 2 && (AOP_TYPE (right) == AOP_LIT || AOP_TYPE (left) == AOP_LIT))
	    {
	      PAIR_ID id = PAIR_DE;
	      asmop *lit = AOP (right);
	      asmop *op = AOP (left);
	      swap_sense = TRUE;

	      if (AOP_TYPE (left) == AOP_LIT)
		{
		  swap_sense = FALSE;
		  lit = AOP (left);
		  op = AOP (right);
		}
	      if (sign)
		{
		  emit2 ("ld e,%s", aopGet (op, 0, 0));
		  emit2 ("ld a,%s", aopGet (op, 1, 0));
		  emit2 ("xor a,!immedbyte", 0x80);
		  emit2 ("ld d,a");
		}
	      else
		{
		  id = getPairId (op);
		  if (id == PAIR_INVALID)
		    {
		      fetchPair (PAIR_DE, op);
		      id = PAIR_DE;
		    }
		}
	      spillPair (PAIR_HL);
	      emit2 ("ld hl,%s", fetchLitSpecial (lit, TRUE, sign));
	      emit2 ("add hl,%s", _getPairIdName (id));
	      goto release;
	    }
	  if (AOP_TYPE (right) == AOP_LIT)
	    {
	      lit = (unsigned long) floatFromVal (AOP (right)->aopu.aop_lit);
	      /* optimize if(x < 0) or if(x >= 0) */
	      if (lit == 0L)
		{
		  if (!sign)
		    {
		      /* No sign so it's always false */
		      _clearCarry();
		    }
		  else
		    {
		      /* Just load in the top most bit */
		      _moveA (aopGet (AOP (left), AOP_SIZE (left) - 1, FALSE));
		      if (!(AOP_TYPE (result) == AOP_CRY && AOP_SIZE (result)) && ifx)
			{
			  genIfxJump (ifx, "7");
			  return;
			}
		      else
			emit2 ("rlc a");
		    }
		  goto release;
		}
	    }
	  if (sign)
	    {
	      /* First setup h and l contaning the top most bytes XORed */
	      bool fDidXor = FALSE;
	      if (AOP_TYPE (left) == AOP_LIT)
		{
		  unsigned long lit = (unsigned long)
		  floatFromVal (AOP (left)->aopu.aop_lit);
		  emit2 ("ld %s,!immedbyte", _fTmp[0],
			 0x80 ^ (unsigned int) ((lit >> ((size - 1) * 8)) & 0x0FFL));
		}
	      else
		{
		  emit2 ("ld a,%s", aopGet (AOP (left), size - 1, FALSE));
		  emit2 ("xor a,!immedbyte", 0x80);
		  emit2 ("ld %s,a", _fTmp[0]);
		  fDidXor = TRUE;
		}
	      if (AOP_TYPE (right) == AOP_LIT)
		{
		  unsigned long lit = (unsigned long)
		  floatFromVal (AOP (right)->aopu.aop_lit);
		  emit2 ("ld %s,!immedbyte", _fTmp[1],
			 0x80 ^ (unsigned int) ((lit >> ((size - 1) * 8)) & 0x0FFL));
		}
	      else
		{
		  emit2 ("ld a,%s", aopGet (AOP (right), size - 1, FALSE));
		  emit2 ("xor a,!immedbyte", 0x80);
		  emit2 ("ld %s,a", _fTmp[1]);
		  fDidXor = TRUE;
		}
	      if (!fDidXor)
		_clearCarry();
	    }
	  else
	    {
	      _clearCarry();
	    }
	  while (size--)
	    {
	      /* Do a long subtract */
	      if (!sign || size)
		{
		  _moveA (aopGet (AOP (left), offset, FALSE));
		}
	      if (sign && size == 0)
		{
		  emit2 ("ld a,%s", _fTmp[0]);
		  emit2 ("sbc a,%s", _fTmp[1]);
		}
	      else
		{
		  /* Subtract through, propagating the carry */
		  emit2 ("sbc a,%s ; 2", aopGet (AOP (right), offset++, FALSE));
		}
	    }
	}
    }

release:
  if (AOP_TYPE (result) == AOP_CRY && AOP_SIZE (result))
    {
      outBitCLong (result, swap_sense);
    }
  else
    {
      /* if the result is used in the next
         ifx conditional branch then generate
         code a little differently */
      if (ifx)
	genIfxJump (ifx, swap_sense ? "nc" : "c");
      else
	outBitCLong (result, swap_sense);
      /* leave the result in acc */
    }
}

/*-----------------------------------------------------------------*/
/* genCmpGt :- greater than comparison                             */
/*-----------------------------------------------------------------*/
static void
genCmpGt (iCode * ic, iCode * ifx)
{
  operand *left, *right, *result;
  sym_link *letype, *retype;
  int sign;

  left = IC_LEFT (ic);
  right = IC_RIGHT (ic);
  result = IC_RESULT (ic);

  letype = getSpec (operandType (left));
  retype = getSpec (operandType (right));
  sign = !(SPEC_USIGN (letype) | SPEC_USIGN (retype));
  /* assign the amsops */
  aopOp (left, ic, FALSE, FALSE);
  aopOp (right, ic, FALSE, FALSE);
  aopOp (result, ic, TRUE, FALSE);

  genCmp (right, left, result, ifx, sign);

  freeAsmop (left, NULL, ic);
  freeAsmop (right, NULL, ic);
  freeAsmop (result, NULL, ic);
}

/*-----------------------------------------------------------------*/
/* genCmpLt - less than comparisons                                */
/*-----------------------------------------------------------------*/
static void
genCmpLt (iCode * ic, iCode * ifx)
{
  operand *left, *right, *result;
  sym_link *letype, *retype;
  int sign;

  left = IC_LEFT (ic);
  right = IC_RIGHT (ic);
  result = IC_RESULT (ic);

  letype = getSpec (operandType (left));
  retype = getSpec (operandType (right));
  sign = !(SPEC_USIGN (letype) | SPEC_USIGN (retype));

  /* assign the amsops */
  aopOp (left, ic, FALSE, FALSE);
  aopOp (right, ic, FALSE, FALSE);
  aopOp (result, ic, TRUE, FALSE);

  genCmp (left, right, result, ifx, sign);

  freeAsmop (left, NULL, ic);
  freeAsmop (right, NULL, ic);
  freeAsmop (result, NULL, ic);
}

/*-----------------------------------------------------------------*/
/* gencjneshort - compare and jump if not equal                    */
/*-----------------------------------------------------------------*/
static void
gencjneshort (operand * left, operand * right, symbol * lbl)
{
  int size = max (AOP_SIZE (left), AOP_SIZE (right));
  int offset = 0;
  unsigned long lit = 0L;

  /* Swap the left and right if it makes the computation easier */
  if (AOP_TYPE (left) == AOP_LIT)
    {
      operand *t = right;
      right = left;
      left = t;
    }

  if (AOP_TYPE (right) == AOP_LIT)
    lit = (unsigned long) floatFromVal (AOP (right)->aopu.aop_lit);

  /* if the right side is a literal then anything goes */
  if (AOP_TYPE (right) == AOP_LIT &&
      AOP_TYPE (left) != AOP_DIR)
    {
      if (lit == 0)
	{
	  emit2 ("ld a,%s", aopGet (AOP (left), offset, FALSE));
	  if (size > 1)
	    {
	      size--;
	      offset++;
	      while (size--)
		{
		  emit2 ("or a,%s", aopGet (AOP (left), offset, FALSE));
		}
	    }
	  else
	    {
	      emit2 ("or a,a");
	    }
	  emit2 ("jp nz,!tlabel", lbl->key + 100);
	}
      else
	{
	  while (size--)
	    {
	      emit2 ("ld a,%s ; 2", aopGet (AOP (left), offset, FALSE));
	      if ((AOP_TYPE (right) == AOP_LIT) && lit == 0)
		emit2 ("or a,a");
	      else
		emit2 ("cp a,%s", aopGet (AOP (right), offset, FALSE));
	      emit2 ("jp nz,!tlabel", lbl->key + 100);
	      offset++;
	    }
	}
    }
  /* if the right side is in a register or in direct space or
     if the left is a pointer register & right is not */
  else if (AOP_TYPE (right) == AOP_REG ||
	   AOP_TYPE (right) == AOP_DIR ||
	   (AOP_TYPE (left) == AOP_DIR && AOP_TYPE (right) == AOP_LIT))
    {
      while (size--)
	{
	  _moveA (aopGet (AOP (left), offset, FALSE));
	  if ((AOP_TYPE (left) == AOP_DIR && AOP_TYPE (right) == AOP_LIT) &&
	      ((unsigned int) ((lit >> (offset * 8)) & 0x0FFL) == 0))
	    /* PENDING */
	    emit2 ("jp nz,!tlabel", lbl->key + 100);
	  else
	    {
	      emit2 ("cp %s ; 4", aopGet (AOP (right), offset, FALSE));
	      emit2 ("jp nz,!tlabel", lbl->key + 100);
	    }
	  offset++;
	}
    }
  else
    {
      /* right is a pointer reg need both a & b */
      /* PENDING: is this required? */
      while (size--)
	{
	  _moveA (aopGet (AOP (right), offset, FALSE));
	  emit2 ("cp %s ; 5", aopGet (AOP (left), offset, FALSE));
	  emit2 ("!shortjp nz,!tlabel", lbl->key + 100);
	  offset++;
	}
    }
}

/*-----------------------------------------------------------------*/
/* gencjne - compare and jump if not equal                         */
/*-----------------------------------------------------------------*/
static void
gencjne (operand * left, operand * right, symbol * lbl)
{
  symbol *tlbl = newiTempLabel (NULL);

  gencjneshort (left, right, lbl);

  /* PENDING: ?? */
  emit2 ("ld a,!one");
  emit2 ("!shortjp !tlabel", tlbl->key + 100);
  emitLabel (lbl->key + 100);
  emit2 ("xor a,a");
  emitLabel (tlbl->key + 100);
}

/*-----------------------------------------------------------------*/
/* genCmpEq - generates code for equal to                          */
/*-----------------------------------------------------------------*/
static void
genCmpEq (iCode * ic, iCode * ifx)
{
  operand *left, *right, *result;

  aopOp ((left = IC_LEFT (ic)), ic, FALSE, FALSE);
  aopOp ((right = IC_RIGHT (ic)), ic, FALSE, FALSE);
  aopOp ((result = IC_RESULT (ic)), ic, TRUE, FALSE);

  emit2("; genCmpEq: left %u, right %u, result %u\n", AOP_SIZE(IC_LEFT(ic)), AOP_SIZE(IC_RIGHT(ic)), AOP_SIZE(IC_RESULT(ic)));

  /* Swap operands if it makes the operation easier. ie if:
     1.  Left is a literal.
   */
  if (AOP_TYPE (IC_LEFT (ic)) == AOP_LIT)
    {
      operand *t = IC_RIGHT (ic);
      IC_RIGHT (ic) = IC_LEFT (ic);
      IC_LEFT (ic) = t;
    }

  if (ifx && !AOP_SIZE (result))
    {
      symbol *tlbl;
      /* if they are both bit variables */
      if (AOP_TYPE (left) == AOP_CRY &&
	  ((AOP_TYPE (right) == AOP_CRY) || (AOP_TYPE (right) == AOP_LIT)))
	{
	  wassert (0);
	}
      else
	{
	  tlbl = newiTempLabel (NULL);
	  gencjneshort (left, right, tlbl);
	  if (IC_TRUE (ifx))
	    {
	      emit2 ("jp !tlabel", IC_TRUE (ifx)->key + 100);
	      emitLabel (tlbl->key + 100);
	    }
	  else
	    {
	      /* PENDING: do this better */
	      symbol *lbl = newiTempLabel (NULL);
	      emit2 ("!shortjp !tlabel", lbl->key + 100);
	      emitLabel (tlbl->key + 100);
	      emit2 ("jp !tlabel", IC_FALSE (ifx)->key + 100);
	      emitLabel (lbl->key + 100);
	    }
	}
      /* mark the icode as generated */
      ifx->generated = 1;
      goto release;
    }

  /* if they are both bit variables */
  if (AOP_TYPE (left) == AOP_CRY &&
      ((AOP_TYPE (right) == AOP_CRY) || (AOP_TYPE (right) == AOP_LIT)))
    {
      wassert (0);
    }
  else
    {
      gencjne (left, right, newiTempLabel (NULL));
      if (AOP_TYPE (result) == AOP_CRY && AOP_SIZE (result))
	{
	  wassert (0);
	}
      if (ifx)
	{
	  genIfxJump (ifx, "a");
	  goto release;
	}
      /* if the result is used in an arithmetic operation
         then put the result in place */
      if (AOP_TYPE (result) != AOP_CRY)
	{
	  outAcc (result);
	}
      /* leave the result in acc */
    }

release:
  freeAsmop (left, NULL, ic);
  freeAsmop (right, NULL, ic);
  freeAsmop (result, NULL, ic);
}

/*-----------------------------------------------------------------*/
/* ifxForOp - returns the icode containing the ifx for operand     */
/*-----------------------------------------------------------------*/
static iCode *
ifxForOp (operand * op, iCode * ic)
{
  /* if true symbol then needs to be assigned */
  if (IS_TRUE_SYMOP (op))
    return NULL;

  /* if this has register type condition and
     the next instruction is ifx with the same operand
     and live to of the operand is upto the ifx only then */
  if (ic->next &&
      ic->next->op == IFX &&
      IC_COND (ic->next)->key == op->key &&
      OP_SYMBOL (op)->liveTo <= ic->next->seq)
    return ic->next;

  return NULL;
}

/*-----------------------------------------------------------------*/
/* genAndOp - for && operation                                     */
/*-----------------------------------------------------------------*/
static void
genAndOp (iCode * ic)
{
  operand *left, *right, *result;
  symbol *tlbl;

  /* note here that && operations that are in an if statement are
     taken away by backPatchLabels only those used in arthmetic
     operations remain */
  aopOp ((left = IC_LEFT (ic)), ic, FALSE, TRUE);
  aopOp ((right = IC_RIGHT (ic)), ic, FALSE, TRUE);
  aopOp ((result = IC_RESULT (ic)), ic, FALSE, FALSE);

  /* if both are bit variables */
  if (AOP_TYPE (left) == AOP_CRY &&
      AOP_TYPE (right) == AOP_CRY)
    {
      wassert (0);
    }
  else
    {
      tlbl = newiTempLabel (NULL);
      toBoolean (left);
      emit2 ("!shortjp z,!tlabel", tlbl->key + 100);
      toBoolean (right);
      emitLabel (tlbl->key + 100);
      outBitAcc (result);
    }

  freeAsmop (left, NULL, ic);
  freeAsmop (right, NULL, ic);
  freeAsmop (result, NULL, ic);
}

/*-----------------------------------------------------------------*/
/* genOrOp - for || operation                                      */
/*-----------------------------------------------------------------*/
static void
genOrOp (iCode * ic)
{
  operand *left, *right, *result;
  symbol *tlbl;

  /* note here that || operations that are in an
     if statement are taken away by backPatchLabels
     only those used in arthmetic operations remain */
  aopOp ((left = IC_LEFT (ic)), ic, FALSE, TRUE);
  aopOp ((right = IC_RIGHT (ic)), ic, FALSE, TRUE);
  aopOp ((result = IC_RESULT (ic)), ic, FALSE, FALSE);

  /* if both are bit variables */
  if (AOP_TYPE (left) == AOP_CRY &&
      AOP_TYPE (right) == AOP_CRY)
    {
      wassert (0);
    }
  else
    {
      tlbl = newiTempLabel (NULL);
      toBoolean (left);
      emit2 ("!shortjp nz,!tlabel", tlbl->key + 100);
      toBoolean (right);
      emitLabel (tlbl->key + 100);
      outBitAcc (result);
    }

  freeAsmop (left, NULL, ic);
  freeAsmop (right, NULL, ic);
  freeAsmop (result, NULL, ic);
}

/*-----------------------------------------------------------------*/
/* isLiteralBit - test if lit == 2^n                               */
/*-----------------------------------------------------------------*/
int
isLiteralBit (unsigned long lit)
{
  unsigned long pw[32] =
  {1L, 2L, 4L, 8L, 16L, 32L, 64L, 128L,
   0x100L, 0x200L, 0x400L, 0x800L,
   0x1000L, 0x2000L, 0x4000L, 0x8000L,
   0x10000L, 0x20000L, 0x40000L, 0x80000L,
   0x100000L, 0x200000L, 0x400000L, 0x800000L,
   0x1000000L, 0x2000000L, 0x4000000L, 0x8000000L,
   0x10000000L, 0x20000000L, 0x40000000L, 0x80000000L};
  int idx;

  for (idx = 0; idx < 32; idx++)
    if (lit == pw[idx])
      return idx + 1;
  return 0;
}

/*-----------------------------------------------------------------*/
/* jmpTrueOrFalse -                                                */
/*-----------------------------------------------------------------*/
static void
jmpTrueOrFalse (iCode * ic, symbol * tlbl)
{
  // ugly but optimized by peephole
  if (IC_TRUE (ic))
    {
      symbol *nlbl = newiTempLabel (NULL);
      emit2 ("jp !tlabel", nlbl->key + 100);
      emitLabel (tlbl->key + 100);
      emit2 ("jp !tlabel", IC_TRUE (ic)->key + 100);
      emitLabel (nlbl->key + 100);
    }
  else
    {
      emit2 ("jp !tlabel", IC_FALSE (ic)->key + 100);
      emitLabel (tlbl->key + 100);
    }
  ic->generated = 1;
}

/*-----------------------------------------------------------------*/
/* genAnd  - code for and                                          */
/*-----------------------------------------------------------------*/
static void
genAnd (iCode * ic, iCode * ifx)
{
  operand *left, *right, *result;
  int size, offset = 0;
  unsigned long lit = 0L;
  int bytelit = 0;

  aopOp ((left = IC_LEFT (ic)), ic, FALSE, FALSE);
  aopOp ((right = IC_RIGHT (ic)), ic, FALSE, FALSE);
  aopOp ((result = IC_RESULT (ic)), ic, TRUE, FALSE);

#ifdef DEBUG_TYPE
  emit2 ("; Type res[%d] = l[%d]&r[%d]",
	    AOP_TYPE (result),
	    AOP_TYPE (left), AOP_TYPE (right));
  emit2 ("; Size res[%d] = l[%d]&r[%d]",
	    AOP_SIZE (result),
	    AOP_SIZE (left), AOP_SIZE (right));
#endif

  /* if left is a literal & right is not then exchange them */
  if ((AOP_TYPE (left) == AOP_LIT && AOP_TYPE (right) != AOP_LIT) ||
      AOP_NEEDSACC (left))
    {
      operand *tmp = right;
      right = left;
      left = tmp;
    }

  /* if result = right then exchange them */
  if (sameRegs (AOP (result), AOP (right)))
    {
      operand *tmp = right;
      right = left;
      left = tmp;
    }

  /* if right is bit then exchange them */
  if (AOP_TYPE (right) == AOP_CRY &&
      AOP_TYPE (left) != AOP_CRY)
    {
      operand *tmp = right;
      right = left;
      left = tmp;
    }
  if (AOP_TYPE (right) == AOP_LIT)
    lit = (unsigned long) floatFromVal (AOP (right)->aopu.aop_lit);

  size = AOP_SIZE (result);

  if (AOP_TYPE (left) == AOP_CRY)
    {
      wassert (0);
      goto release;
    }

  // if(val & 0xZZ)       - size = 0, ifx != FALSE  -
  // bit = val & 0xZZ     - size = 1, ifx = FALSE -
  if ((AOP_TYPE (right) == AOP_LIT) &&
      (AOP_TYPE (result) == AOP_CRY) &&
      (AOP_TYPE (left) != AOP_CRY))
    {
      int posbit = isLiteralBit (lit);
      /* left &  2^n */
      if (posbit)
	{
	  posbit--;
	  _moveA (aopGet (AOP (left), posbit >> 3, FALSE));
	  // bit = left & 2^n
	  if (size)
	    {
	      wassert (0);
	      emit2 ("mov c,acc.%d", posbit & 0x07);
	    }
	  // if(left &  2^n)
	  else
	    {
	      if (ifx)
		{
		  sprintf (buffer, "%d", posbit & 0x07);
		  genIfxJump (ifx, buffer);
		}
	      else
		{
		  wassert (0);
		}
	      goto release;
	    }
	}
      else
	{
	  symbol *tlbl = newiTempLabel (NULL);
	  int sizel = AOP_SIZE (left);
	  if (size)
	    {
	      wassert (0);
	      emit2 ("setb c");
	    }
	  while (sizel--)
	    {
	      if ((bytelit = ((lit >> (offset * 8)) & 0x0FFL)) != 0x0L)
		{
		  _moveA (aopGet (AOP (left), offset, FALSE));
		  // byte ==  2^n ?
		  if ((posbit = isLiteralBit (bytelit)) != 0)
		    {
		      wassert (0);
		      emit2 ("jb acc.%d,%05d$", (posbit - 1) & 0x07, tlbl->key + 100);
		    }
		  else
		    {
		      if (bytelit != 0x0FFL)
			emit2 ("and a,%s",
				  aopGet (AOP (right), offset, FALSE));
		      else
			/* For the flags */
			emit2 ("or a,a");
		      emit2 ("!shortjp nz,!tlabel", tlbl->key + 100);
		    }
		}
	      offset++;
	    }
	  // bit = left & literal
	  if (size)
	    {
	      emit2 ("clr c");
	      emit2 ("!tlabeldef", tlbl->key + 100);
	    }
	  // if(left & literal)
	  else
	    {
	      if (ifx)
		jmpTrueOrFalse (ifx, tlbl);
	      goto release;
	    }
	}
      outBitC (result);
      goto release;
    }

  /* if left is same as result */
  if (sameRegs (AOP (result), AOP (left)))
    {
      for (; size--; offset++)
	{
	  if (AOP_TYPE (right) == AOP_LIT)
	    {
	      if ((bytelit = (int) ((lit >> (offset * 8)) & 0x0FFL)) == 0x0FF)
		continue;
	      else
		{
		  if (bytelit == 0)
		    aopPut (AOP (result), "!zero", offset);
		  else
		    {
		      _moveA (aopGet (AOP (left), offset, FALSE));
		      emit2 ("and a,%s",
				aopGet (AOP (right), offset, FALSE));
		      aopPut (AOP (left), "a", offset);
		    }
		}

	    }
	  else
	    {
	      if (AOP_TYPE (left) == AOP_ACC)
		{
		  wassert (0);
		}
	      else
		{
		  _moveA (aopGet (AOP (left), offset, FALSE));
		  emit2 ("and a,%s",
			    aopGet (AOP (right), offset, FALSE));
		  aopPut (AOP (left), "a", offset);
		}
	    }
	}
    }
  else
    {
      // left & result in different registers
      if (AOP_TYPE (result) == AOP_CRY)
	{
	  wassert (0);
	}
      else
	{
	  for (; (size--); offset++)
	    {
	      // normal case
	      // result = left & right
	      if (AOP_TYPE (right) == AOP_LIT)
		{
		  if ((bytelit = (int) ((lit >> (offset * 8)) & 0x0FFL)) == 0x0FF)
		    {
		      aopPut (AOP (result),
			      aopGet (AOP (left), offset, FALSE),
			      offset);
		      continue;
		    }
		  else if (bytelit == 0)
		    {
		      aopPut (AOP (result), "!zero", offset);
		      continue;
		    }
		}
	      // faster than result <- left, anl result,right
	      // and better if result is SFR
	      if (AOP_TYPE (left) == AOP_ACC)
		emit2 ("and a,%s", aopGet (AOP (right), offset, FALSE));
	      else
		{
		  _moveA (aopGet (AOP (left), offset, FALSE));
		  emit2 ("and a,%s",
			    aopGet (AOP (right), offset, FALSE));
		}
	      aopPut (AOP (result), "a", offset);
	    }
	}

    }

release:
  freeAsmop (left, NULL, ic);
  freeAsmop (right, NULL, ic);
  freeAsmop (result, NULL, ic);
}

/*-----------------------------------------------------------------*/
/* genOr  - code for or                                            */
/*-----------------------------------------------------------------*/
static void
genOr (iCode * ic, iCode * ifx)
{
  operand *left, *right, *result;
  int size, offset = 0;
  unsigned long lit = 0L;

  aopOp ((left = IC_LEFT (ic)), ic, FALSE, FALSE);
  aopOp ((right = IC_RIGHT (ic)), ic, FALSE, FALSE);
  aopOp ((result = IC_RESULT (ic)), ic, TRUE, FALSE);

#if 1
  emit2 ("; Type res[%d] = l[%d]&r[%d]",
	    AOP_TYPE (result),
	    AOP_TYPE (left), AOP_TYPE (right));
  emit2 ("; Size res[%d] = l[%d]&r[%d]",
	    AOP_SIZE (result),
	    AOP_SIZE (left), AOP_SIZE (right));
#endif

  /* if left is a literal & right is not then exchange them */
  if ((AOP_TYPE (left) == AOP_LIT && AOP_TYPE (right) != AOP_LIT) ||
      AOP_NEEDSACC (left))
    {
      operand *tmp = right;
      right = left;
      left = tmp;
    }

  /* if result = right then exchange them */
  if (sameRegs (AOP (result), AOP (right)))
    {
      operand *tmp = right;
      right = left;
      left = tmp;
    }

  /* if right is bit then exchange them */
  if (AOP_TYPE (right) == AOP_CRY &&
      AOP_TYPE (left) != AOP_CRY)
    {
      operand *tmp = right;
      right = left;
      left = tmp;
    }
  if (AOP_TYPE (right) == AOP_LIT)
    lit = (unsigned long) floatFromVal (AOP (right)->aopu.aop_lit);

  size = AOP_SIZE (result);

  if (AOP_TYPE (left) == AOP_CRY)
    {
      wassert (0);
      goto release;
    }

  if ((AOP_TYPE (right) == AOP_LIT) &&
      (AOP_TYPE (result) == AOP_CRY) &&
      (AOP_TYPE (left) != AOP_CRY))
    {
      wassert (0);
      goto release;
    }

  /* if left is same as result */
  if (sameRegs (AOP (result), AOP (left)))
    {
      for (; size--; offset++)
	{
	  if (AOP_TYPE (right) == AOP_LIT)
	    {
	      if (((lit >> (offset * 8)) & 0x0FFL) == 0x00L)
		continue;
	      else
		{
		  _moveA (aopGet (AOP (left), offset, FALSE));
		  emit2 ("or a,%s",
			    aopGet (AOP (right), offset, FALSE));
		  aopPut (AOP (result), "a", offset);
		}
	    }
	  else
	    {
	      if (AOP_TYPE (left) == AOP_ACC)
		emit2 ("or a,%s", aopGet (AOP (right), offset, FALSE));
	      else
		{
		  _moveA (aopGet (AOP (left), offset, FALSE));
		  emit2 ("or a,%s",
			    aopGet (AOP (right), offset, FALSE));
		  aopPut (AOP (result), "a", offset);
		}
	    }
	}
    }
  else
    {
      // left & result in different registers
      if (AOP_TYPE (result) == AOP_CRY)
	{
	  wassert (0);
	}
      else
	for (; (size--); offset++)
	  {
	    // normal case
	    // result = left & right
	    if (AOP_TYPE (right) == AOP_LIT)
	      {
		if (((lit >> (offset * 8)) & 0x0FFL) == 0x00L)
		  {
		    aopPut (AOP (result),
			    aopGet (AOP (left), offset, FALSE),
			    offset);
		    continue;
		  }
	      }
	    // faster than result <- left, anl result,right
	    // and better if result is SFR
	    if (AOP_TYPE (left) == AOP_ACC)
	      emit2 ("or a,%s", aopGet (AOP (right), offset, FALSE));
	    else
	      {
		_moveA (aopGet (AOP (left), offset, FALSE));
		emit2 ("or a,%s",
			  aopGet (AOP (right), offset, FALSE));
	      }
	    aopPut (AOP (result), "a", offset);
	    /* PENDING: something weird is going on here.  Add exception. */
	    if (AOP_TYPE (result) == AOP_ACC)
	      break;
	  }
    }

release:
  freeAsmop (left, NULL, ic);
  freeAsmop (right, NULL, ic);
  freeAsmop (result, NULL, ic);
}

/*-----------------------------------------------------------------*/
/* genXor - code for xclusive or                                   */
/*-----------------------------------------------------------------*/
static void
genXor (iCode * ic, iCode * ifx)
{
  operand *left, *right, *result;
  int size, offset = 0;
  unsigned long lit = 0L;

  aopOp ((left = IC_LEFT (ic)), ic, FALSE, FALSE);
  aopOp ((right = IC_RIGHT (ic)), ic, FALSE, FALSE);
  aopOp ((result = IC_RESULT (ic)), ic, TRUE, FALSE);

  /* if left is a literal & right is not then exchange them */
  if ((AOP_TYPE (left) == AOP_LIT && AOP_TYPE (right) != AOP_LIT) ||
      AOP_NEEDSACC (left))
    {
      operand *tmp = right;
      right = left;
      left = tmp;
    }

  /* if result = right then exchange them */
  if (sameRegs (AOP (result), AOP (right)))
    {
      operand *tmp = right;
      right = left;
      left = tmp;
    }

  /* if right is bit then exchange them */
  if (AOP_TYPE (right) == AOP_CRY &&
      AOP_TYPE (left) != AOP_CRY)
    {
      operand *tmp = right;
      right = left;
      left = tmp;
    }
  if (AOP_TYPE (right) == AOP_LIT)
    lit = (unsigned long) floatFromVal (AOP (right)->aopu.aop_lit);

  size = AOP_SIZE (result);

  if (AOP_TYPE (left) == AOP_CRY)
    {
      wassert (0);
      goto release;
    }

  if ((AOP_TYPE (right) == AOP_LIT) &&
      (AOP_TYPE (result) == AOP_CRY) &&
      (AOP_TYPE (left) != AOP_CRY))
    {
      wassert (0);
      goto release;
    }

  /* if left is same as result */
  if (sameRegs (AOP (result), AOP (left)))
    {
      for (; size--; offset++)
	{
	  if (AOP_TYPE (right) == AOP_LIT)
	    {
	      if (((lit >> (offset * 8)) & 0x0FFL) == 0x00L)
		continue;
	      else
		{
		  _moveA (aopGet (AOP (right), offset, FALSE));
		  emit2 ("xor a,%s",
			    aopGet (AOP (left), offset, FALSE));
		  aopPut (AOP (result), "a", 0);
		}
	    }
	  else
	    {
	      if (AOP_TYPE (left) == AOP_ACC)
		emit2 ("xor a,%s", aopGet (AOP (right), offset, FALSE));
	      else
		{
		  _moveA (aopGet (AOP (right), offset, FALSE));
		  emit2 ("xor a,%s",
			    aopGet (AOP (left), offset, FALSE));
		  aopPut (AOP (result), "a", 0);
		}
	    }
	}
    }
  else
    {
      // left & result in different registers
      if (AOP_TYPE (result) == AOP_CRY)
	{
	  wassert (0);
	}
      else
	for (; (size--); offset++)
	  {
	    // normal case
	    // result = left & right
	    if (AOP_TYPE (right) == AOP_LIT)
	      {
		if (((lit >> (offset * 8)) & 0x0FFL) == 0x00L)
		  {
		    aopPut (AOP (result),
			    aopGet (AOP (left), offset, FALSE),
			    offset);
		    continue;
		  }
	      }
	    // faster than result <- left, anl result,right
	    // and better if result is SFR
	    if (AOP_TYPE (left) == AOP_ACC)
	      emit2 ("xor a,%s", aopGet (AOP (right), offset, FALSE));
	    else
	      {
		_moveA (aopGet (AOP (right), offset, FALSE));
		emit2 ("xor a,%s",
			  aopGet (AOP (left), offset, FALSE));
		aopPut (AOP (result), "a", 0);
	      }
	    aopPut (AOP (result), "a", offset);
	  }
    }

release:
  freeAsmop (left, NULL, ic);
  freeAsmop (right, NULL, ic);
  freeAsmop (result, NULL, ic);
}

/*-----------------------------------------------------------------*/
/* genInline - write the inline code out                           */
/*-----------------------------------------------------------------*/
static void
genInline (iCode * ic)
{
  char *buffer, *bp, *bp1;

  _G.lines.isInline += (!options.asmpeep);

  buffer = bp = bp1 = Safe_calloc(1, strlen(IC_INLINE(ic))+1);
  strcpy (buffer, IC_INLINE (ic));

  /* emit each line as a code */
  while (*bp)
    {
      if (*bp == '\n')
	{
	  *bp++ = '\0';
	  emit2 (bp1);
	  bp1 = bp;
	}
      else
	{
	  if (*bp == ':')
	    {
	      bp++;
	      *bp = '\0';
	      bp++;
	      emit2 (bp1);
	      bp1 = bp;
	    }
	  else
	    bp++;
	}
    }
  if (bp1 != bp)
    emit2 (bp1);
  _G.lines.isInline -= (!options.asmpeep);

}

/*-----------------------------------------------------------------*/
/* genRRC - rotate right with carry                                */
/*-----------------------------------------------------------------*/
static void
genRRC (iCode * ic)
{
  wassert (0);
}

/*-----------------------------------------------------------------*/
/* genRLC - generate code for rotate left with carry               */
/*-----------------------------------------------------------------*/
static void
genRLC (iCode * ic)
{
  wassert (0);
}

/*-----------------------------------------------------------------*/
/* shiftR2Left2Result - shift right two bytes from left to result  */
/*-----------------------------------------------------------------*/
static void
shiftR2Left2Result (operand * left, int offl,
		    operand * result, int offr,
		    int shCount, int sign)
{
  movLeft2Result (left, offl, result, offr, 0);
  movLeft2Result (left, offl + 1, result, offr + 1, 0);

  if (sign)
    {
      wassert (0);
    }
  else
    {
      /*  if (AOP(result)->type == AOP_REG) { */
      int size = 2;
      int offset = 0;
      symbol *tlbl, *tlbl1;
      const char *l;

      tlbl = newiTempLabel (NULL);
      tlbl1 = newiTempLabel (NULL);

      /* Left is already in result - so now do the shift */
      if (shCount > 1)
	{
	  emit2 ("ld a,!immedbyte+1", shCount);
	  emit2 ("!shortjp !tlabel", tlbl1->key + 100);
	  emitLabel (tlbl->key + 100);
	}

      emit2 ("or a,a");
      offset = size;
      while (size--)
	{
	  l = aopGet (AOP (result), --offset, FALSE);
	  emit2 ("rr %s", l);
	}
      if (shCount > 1)
	{
	  emitLabel (tlbl1->key + 100);
	  emit2 ("dec a");
	  emit2 ("!shortjp nz,!tlabel", tlbl->key + 100);
	}
    }
}

/*-----------------------------------------------------------------*/
/* shiftL2Left2Result - shift left two bytes from left to result   */
/*-----------------------------------------------------------------*/
static void
shiftL2Left2Result (operand * left, int offl,
		    operand * result, int offr, int shCount)
{
  if (sameRegs (AOP (result), AOP (left)) &&
      ((offl + MSB16) == offr))
    {
      wassert (0);
    }
  else
    {
      /* Copy left into result */
      movLeft2Result (left, offl, result, offr, 0);
      movLeft2Result (left, offl + 1, result, offr + 1, 0);
    }
  /* PENDING: for now just see if it'll work. */
  /*if (AOP(result)->type == AOP_REG) { */
  {
    int size = 2;
    int offset = 0;
    symbol *tlbl, *tlbl1;
    const char *l;

    tlbl = newiTempLabel (NULL);
    tlbl1 = newiTempLabel (NULL);

    /* Left is already in result - so now do the shift */
    if (shCount > 1)
      {
	emit2 ("ld a,!immedbyte+1", shCount);
	emit2 ("!shortjp !tlabel", tlbl1->key + 100);
	emitLabel (tlbl->key + 100);
      }

    emit2 ("or a,a");
    while (size--)
      {
	l = aopGet (AOP (result), offset++, FALSE);
	emit2 ("rl %s", l);
      }
    if (shCount > 1)
      {
	emitLabel (tlbl1->key + 100);
	emit2 ("dec a");
	emit2 ("!shortjp nz,!tlabel", tlbl->key + 100);
      }
  }
}

/*-----------------------------------------------------------------*/
/* AccRol - rotate left accumulator by known count                 */
/*-----------------------------------------------------------------*/
static void
AccRol (int shCount)
{
  shCount &= 0x0007;		// shCount : 0..7

  switch (shCount)
    {
    case 0:
      break;
    case 1:
      emit2 ("rl a");
      break;
    case 2:
      emit2 ("rl a");
      emit2 ("rl a");
      break;
    case 3:
      emit2 ("rl a");
      emit2 ("rl a");
      emit2 ("rl a");
      break;
    case 4:
      emit2 ("rl a");
      emit2 ("rl a");
      emit2 ("rl a");
      emit2 ("rl a");
      break;
    case 5:
      emit2 ("rr a");
      emit2 ("rr a");
      emit2 ("rr a");
      break;
    case 6:
      emit2 ("rr a");
      emit2 ("rr a");
      break;
    case 7:
      emit2 ("rr a");
      break;
    }
}

/*-----------------------------------------------------------------*/
/* AccLsh - left shift accumulator by known count                  */
/*-----------------------------------------------------------------*/
static void
AccLsh (int shCount)
{
  static const unsigned char SLMask[] =
    {
      0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80, 0x00
    };

  if (shCount != 0)
    {
      if (shCount == 1)
	{
	  emit2 ("add a,a");
	}
      else if (shCount == 2)
	{
	  emit2 ("add a,a");
	  emit2 ("add a,a");
	}
      else
	{
	  /* rotate left accumulator */
	  AccRol (shCount);
	  /* and kill the lower order bits */
	  emit2 ("and a,!immedbyte", SLMask[shCount]);
	}
    }
}

/*-----------------------------------------------------------------*/
/* shiftL1Left2Result - shift left one byte from left to result    */
/*-----------------------------------------------------------------*/
static void
shiftL1Left2Result (operand * left, int offl,
		    operand * result, int offr, int shCount)
{
  const char *l;
  l = aopGet (AOP (left), offl, FALSE);
  _moveA (l);
  /* shift left accumulator */
  AccLsh (shCount);
  aopPut (AOP (result), "a", offr);
}


/*-----------------------------------------------------------------*/
/* genlshTwo - left shift two bytes by known amount != 0           */
/*-----------------------------------------------------------------*/
static void
genlshTwo (operand * result, operand * left, int shCount)
{
  int size = AOP_SIZE (result);

  wassert (size == 2);

  /* if shCount >= 8 */
  if (shCount >= 8)
    {
      shCount -= 8;
      if (size > 1)
	{
	  if (shCount)
	    {
	      movLeft2Result (left, LSB, result, MSB16, 0);
	      aopPut (AOP (result), "!zero", 0);
	      shiftL1Left2Result (left, MSB16, result, MSB16, shCount);
	    }
	  else
	    {
	      movLeft2Result (left, LSB, result, MSB16, 0);
	      aopPut (AOP (result), "!zero", 0);
	    }
	}
      else
	{
	  aopPut (AOP (result), "!zero", LSB);
	}
    }
  /*  1 <= shCount <= 7 */
  else
    {
      if (size == 1)
	{
	  wassert (0);
	}
      else
	{
	  shiftL2Left2Result (left, LSB, result, LSB, shCount);
	}
    }
}

/*-----------------------------------------------------------------*/
/* genlshOne - left shift a one byte quantity by known count       */
/*-----------------------------------------------------------------*/
static void
genlshOne (operand * result, operand * left, int shCount)
{
  shiftL1Left2Result (left, LSB, result, LSB, shCount);
}

/*-----------------------------------------------------------------*/
/* genLeftShiftLiteral - left shifting by known count              */
/*-----------------------------------------------------------------*/
static void
genLeftShiftLiteral (operand * left,
		     operand * right,
		     operand * result,
		     iCode * ic)
{
  int shCount = (int) floatFromVal (AOP (right)->aopu.aop_lit);
  int size;

  freeAsmop (right, NULL, ic);

  aopOp (left, ic, FALSE, FALSE);
  aopOp (result, ic, FALSE, FALSE);

  size = getSize (operandType (result));

#if VIEW_SIZE
  emit2 ("; shift left  result %d, left %d", size,
	    AOP_SIZE (left));
#endif

  /* I suppose that the left size >= result size */
  if (shCount == 0)
    {
      wassert (0);
    }

  else if (shCount >= (size * 8))
    while (size--)
      aopPut (AOP (result), "!zero", size);
  else
    {
      switch (size)
	{
	case 1:
	  genlshOne (result, left, shCount);
	  break;
	case 2:
	  genlshTwo (result, left, shCount);
	  break;
	case 4:
	  wassert (0);
	  break;
	default:
	  wassert (0);
	}
    }
  freeAsmop (left, NULL, ic);
  freeAsmop (result, NULL, ic);
}

/*-----------------------------------------------------------------*/
/* genLeftShift - generates code for left shifting                 */
/*-----------------------------------------------------------------*/
static void
genLeftShift (iCode * ic)
{
  int size, offset;
  const char *l;
  symbol *tlbl, *tlbl1;
  operand *left, *right, *result;

  right = IC_RIGHT (ic);
  left = IC_LEFT (ic);
  result = IC_RESULT (ic);

  aopOp (right, ic, FALSE, FALSE);

  /* if the shift count is known then do it
     as efficiently as possible */
  if (AOP_TYPE (right) == AOP_LIT)
    {
      genLeftShiftLiteral (left, right, result, ic);
      return;
    }

  /* shift count is unknown then we have to form a loop get the loop
     count in B : Note: we take only the lower order byte since
     shifting more that 32 bits make no sense anyway, ( the largest
     size of an object can be only 32 bits ) */
  emit2 ("ld a,%s", aopGet (AOP (right), 0, FALSE));
  emit2 ("inc a");
  freeAsmop (right, NULL, ic);
  aopOp (left, ic, FALSE, FALSE);
  aopOp (result, ic, FALSE, FALSE);

  /* now move the left to the result if they are not the
     same */
#if 1
  if (!sameRegs (AOP (left), AOP (result)))
    {

      size = AOP_SIZE (result);
      offset = 0;
      while (size--)
	{
	  l = aopGet (AOP (left), offset, FALSE);
	  aopPut (AOP (result), l, offset);
	  offset++;
	}
    }
#else
  size = AOP_SIZE (result);
  offset = 0;
  while (size--)
    {
      l = aopGet (AOP (left), offset, FALSE);
      aopPut (AOP (result), l, offset);
      offset++;
    }
#endif


  tlbl = newiTempLabel (NULL);
  size = AOP_SIZE (result);
  offset = 0;
  tlbl1 = newiTempLabel (NULL);

  emit2 ("!shortjp !tlabel", tlbl1->key + 100);
  emitLabel (tlbl->key + 100);
  l = aopGet (AOP (result), offset, FALSE);
  emit2 ("or a,a");
  while (size--)
    {
      l = aopGet (AOP (result), offset++, FALSE);
      emit2 ("rl %s", l);
    }
  emitLabel (tlbl1->key + 100);
  emit2 ("dec a");
  emit2 ("!shortjp nz,!tlabel", tlbl->key + 100);

  freeAsmop (left, NULL, ic);
  freeAsmop (result, NULL, ic);
}

/*-----------------------------------------------------------------*/
/* genrshOne - left shift two bytes by known amount != 0           */
/*-----------------------------------------------------------------*/
static void
genrshOne (operand * result, operand * left, int shCount)
{
  /* Errk */
  int size = AOP_SIZE (result);
  const char *l;

  wassert (size == 1);
  wassert (shCount < 8);

  l = aopGet (AOP (left), 0, FALSE);
  if (AOP (result)->type == AOP_REG)
    {
      aopPut (AOP (result), l, 0);
      l = aopGet (AOP (result), 0, FALSE);
      while (shCount--)
	emit2 ("srl %s", l);
    }
  else
    {
      _moveA (l);
      while (shCount--)
	{
	  emit2 ("srl a");
	}
      aopPut (AOP (result), "a", 0);
    }
}

/*-----------------------------------------------------------------*/
/* AccRsh - right shift accumulator by known count                 */
/*-----------------------------------------------------------------*/
static void
AccRsh (int shCount)
{
  static const unsigned char SRMask[] =
    {
      0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01, 0x00
    };

  if (shCount != 0)
    {
      /* rotate right accumulator */
      AccRol (8 - shCount);
      /* and kill the higher order bits */
      emit2 ("and a,!immedbyte", SRMask[shCount]);
    }
}

/*-----------------------------------------------------------------*/
/* shiftR1Left2Result - shift right one byte from left to result   */
/*-----------------------------------------------------------------*/
static void
shiftR1Left2Result (operand * left, int offl,
		    operand * result, int offr,
		    int shCount, int sign)
{
  _moveA (aopGet (AOP (left), offl, FALSE));
  if (sign)
    {
      wassert (0);
    }
  else
    {
      AccRsh (shCount);
    }
  aopPut (AOP (result), "a", offr);
}

/*-----------------------------------------------------------------*/
/* genrshTwo - right shift two bytes by known amount != 0          */
/*-----------------------------------------------------------------*/
static void
genrshTwo (operand * result, operand * left,
	   int shCount, int sign)
{
  /* if shCount >= 8 */
  if (shCount >= 8)
    {
      shCount -= 8;
      if (shCount)
	{
	  shiftR1Left2Result (left, MSB16, result, LSB,
			      shCount, sign);
	}
      else
	{
	  movLeft2Result (left, MSB16, result, LSB, sign);
	}
      aopPut (AOP (result), "!zero", 1);
    }
  /*  1 <= shCount <= 7 */
  else
    {
      shiftR2Left2Result (left, LSB, result, LSB, shCount, sign);
    }
}

/*-----------------------------------------------------------------*/
/* genRightShiftLiteral - left shifting by known count              */
/*-----------------------------------------------------------------*/
static void
genRightShiftLiteral (operand * left,
		      operand * right,
		      operand * result,
		      iCode * ic)
{
  int shCount = (int) floatFromVal (AOP (right)->aopu.aop_lit);
  int size;

  freeAsmop (right, NULL, ic);

  aopOp (left, ic, FALSE, FALSE);
  aopOp (result, ic, FALSE, FALSE);

  size = getSize (operandType (result));

  emit2 ("; shift right  result %d, left %d", size,
	    AOP_SIZE (left));

  /* I suppose that the left size >= result size */
  if (shCount == 0)
    {
      wassert (0);
    }

  else if (shCount >= (size * 8))
    while (size--)
      aopPut (AOP (result), "!zero", size);
  else
    {
      switch (size)
	{
	case 1:
	  genrshOne (result, left, shCount);
	  break;
	case 2:
	  /* PENDING: sign support */
	  genrshTwo (result, left, shCount, FALSE);
	  break;
	case 4:
	  wassert (0);
	  break;
	default:
	  wassert (0);
	}
    }
  freeAsmop (left, NULL, ic);
  freeAsmop (result, NULL, ic);
}

/*-----------------------------------------------------------------*/
/* genRightShift - generate code for right shifting                */
/*-----------------------------------------------------------------*/
static void
genRightShift (iCode * ic)
{
  operand *right, *left, *result;
  sym_link *retype;
  int size, offset, first = 1;
  const char *l;
  bool is_signed;

  symbol *tlbl, *tlbl1;

  /* if signed then we do it the hard way preserve the
     sign bit moving it inwards */
  retype = getSpec (operandType (IC_RESULT (ic)));

  is_signed = !SPEC_USIGN (retype);

  /* signed & unsigned types are treated the same : i.e. the
     signed is NOT propagated inwards : quoting from the
     ANSI - standard : "for E1 >> E2, is equivalent to division
     by 2**E2 if unsigned or if it has a non-negative value,
     otherwise the result is implementation defined ", MY definition
     is that the sign does not get propagated */

  right = IC_RIGHT (ic);
  left = IC_LEFT (ic);
  result = IC_RESULT (ic);

  aopOp (right, ic, FALSE, FALSE);

  /* if the shift count is known then do it
     as efficiently as possible */
  if (AOP_TYPE (right) == AOP_LIT)
    {
      genRightShiftLiteral (left, right, result, ic);
      return;
    }

  aopOp (left, ic, FALSE, FALSE);
  aopOp (result, ic, FALSE, FALSE);

  /* now move the left to the result if they are not the
     same */
  if (!sameRegs (AOP (left), AOP (result)) &&
      AOP_SIZE (result) > 1)
    {

      size = AOP_SIZE (result);
      offset = 0;
      while (size--)
	{
	  l = aopGet (AOP (left), offset, FALSE);
	  aopPut (AOP (result), l, offset);
	  offset++;
	}
    }

  emit2 ("ld a,%s", aopGet (AOP (right), 0, FALSE));
  emit2 ("inc a");
  freeAsmop (right, NULL, ic);

  tlbl = newiTempLabel (NULL);
  tlbl1 = newiTempLabel (NULL);
  size = AOP_SIZE (result);
  offset = size - 1;

  emit2 ("!shortjp !tlabel", tlbl1->key + 100);
  emitLabel (tlbl->key + 100);
  while (size--)
    {
      l = aopGet (AOP (result), offset--, FALSE);
      if (first)
	{
	  if (is_signed)
	    emit2 ("sra %s", l);
	  else
	    emit2 ("srl %s", l);
	  first = 0;
	}
      else
	emit2 ("rr %s", l);
    }
  emitLabel (tlbl1->key + 100);
  emit2 ("dec a");
  emit2 ("!shortjp nz,!tlabel", tlbl->key + 100);

  freeAsmop (left, NULL, ic);
  freeAsmop (result, NULL, ic);
}

/*-----------------------------------------------------------------*/
/* genGenPointerGet -  get value from generic pointer space        */
/*-----------------------------------------------------------------*/
static void
genGenPointerGet (operand * left,
		  operand * result, iCode * ic)
{
  int size, offset;
  sym_link *retype = getSpec (operandType (result));
  int pair = PAIR_HL;

  if (IS_GB)
    pair = PAIR_DE;

  aopOp (left, ic, FALSE, FALSE);
  aopOp (result, ic, FALSE, FALSE);

  if (isPair (AOP (left)) && AOP_SIZE (result) == 1)
    {
      /* Just do it */
      if (isPtrPair (AOP (left)))
	{
	  tsprintf (buffer, "!*pair", getPairName (AOP (left)));
	  aopPut (AOP (result), buffer, 0);
	}
      else
	{
	  emit2 ("ld a,!*pair", getPairName (AOP (left)));
	  aopPut (AOP (result), "a", 0);
	}
      freeAsmop (left, NULL, ic);
      goto release;
    }

  /* For now we always load into IY */
  /* if this is remateriazable */
  fetchPair (pair, AOP (left));

  /* so iy now contains the address */
  freeAsmop (left, NULL, ic);

  /* if bit then unpack */
  if (IS_BITVAR (retype))
    {
      wassert (0);
    }
  else
    {
      size = AOP_SIZE (result);
      offset = 0;

      while (size--)
	{
	  /* PENDING: make this better */
	  if (!IS_GB && AOP (result)->type == AOP_REG)
	    {
	      aopPut (AOP (result), "!*hl", offset++);
	    }
	  else
	    {
	      emit2 ("ld a,!*pair", _pairs[pair].name);
	      aopPut (AOP (result), "a", offset++);
	    }
	  if (size)
	    {
	      emit2 ("inc %s", _pairs[pair].name);
	      _G.pairs[pair].offset++;
	    }
	}
    }

release:
  freeAsmop (result, NULL, ic);
}

/*-----------------------------------------------------------------*/
/* genPointerGet - generate code for pointer get                   */
/*-----------------------------------------------------------------*/
static void
genPointerGet (iCode * ic)
{
  operand *left, *result;
  sym_link *type, *etype;

  left = IC_LEFT (ic);
  result = IC_RESULT (ic);

  /* depending on the type of pointer we need to
     move it to the correct pointer register */
  type = operandType (left);
  etype = getSpec (type);

  genGenPointerGet (left, result, ic);
}

bool
isRegOrLit (asmop * aop)
{
  if (aop->type == AOP_REG || aop->type == AOP_LIT || aop->type == AOP_IMMD)
    return TRUE;
  return FALSE;
}

/*-----------------------------------------------------------------*/
/* genGenPointerSet - stores the value into a pointer location        */
/*-----------------------------------------------------------------*/
static void
genGenPointerSet (operand * right,
		  operand * result, iCode * ic)
{
  int size, offset;
  sym_link *retype = getSpec (operandType (right));
  PAIR_ID pairId = PAIR_HL;

  aopOp (result, ic, FALSE, FALSE);
  aopOp (right, ic, FALSE, FALSE);

  if (IS_GB)
    pairId = PAIR_DE;

  /* Handle the exceptions first */
  if (isPair (AOP (result)) && (AOP_SIZE (right) == 1))
    {
      /* Just do it */
      const char *l = aopGet (AOP (right), 0, FALSE);
      const char *pair = getPairName (AOP (result));
      if (canAssignToPtr (l) && isPtr (pair))
	{
	  emit2 ("ld !*pair,%s", pair, l);
	}
      else
	{
	  _moveA (l);
	  emit2 ("ld !*pair,a", pair);
	}
      goto release;
    }

  /* if the operand is already in dptr
     then we do nothing else we move the value to dptr */
  if (AOP_TYPE (result) != AOP_STR)
    {
      fetchPair (pairId, AOP (result));
    }
  /* so hl know contains the address */
  freeAsmop (result, NULL, ic);

  /* if bit then unpack */
  if (IS_BITVAR (retype))
    {
      wassert (0);
    }
  else
    {
      size = AOP_SIZE (right);
      offset = 0;

      while (size--)
	{
	  const char *l = aopGet (AOP (right), offset, FALSE);
	  if (isRegOrLit (AOP (right)) && !IS_GB)
	    {
	      emit2 ("ld !*pair,%s", _pairs[pairId].name, l);
	    }
	  else
	    {
	      _moveA (l);
	      emit2 ("ld !*pair,a", _pairs[pairId].name);
	    }
	  if (size)
	    {
	      emit2 ("inc %s", _pairs[pairId].name);
	      _G.pairs[pairId].offset++;
	    }
	  offset++;
	}
    }
release:
  freeAsmop (right, NULL, ic);
}

/*-----------------------------------------------------------------*/
/* genPointerSet - stores the value into a pointer location        */
/*-----------------------------------------------------------------*/
static void
genPointerSet (iCode * ic)
{
  operand *right, *result;
  sym_link *type, *etype;

  right = IC_RIGHT (ic);
  result = IC_RESULT (ic);

  /* depending on the type of pointer we need to
     move it to the correct pointer register */
  type = operandType (result);
  etype = getSpec (type);

  genGenPointerSet (right, result, ic);
}

/*-----------------------------------------------------------------*/
/* genIfx - generate code for Ifx statement                        */
/*-----------------------------------------------------------------*/
static void
genIfx (iCode * ic, iCode * popIc)
{
  operand *cond = IC_COND (ic);
  int isbit = 0;

  aopOp (cond, ic, FALSE, TRUE);

  /* get the value into acc */
  if (AOP_TYPE (cond) != AOP_CRY)
    toBoolean (cond);
  else
    isbit = 1;
  /* the result is now in the accumulator */
  freeAsmop (cond, NULL, ic);

  /* if there was something to be popped then do it */
  if (popIc)
    genIpop (popIc);

  /* if the condition is  a bit variable */
  if (isbit && IS_ITEMP (cond) &&
      SPIL_LOC (cond))
    genIfxJump (ic, SPIL_LOC (cond)->rname);
  else if (isbit && !IS_ITEMP (cond))
    genIfxJump (ic, OP_SYMBOL (cond)->rname);
  else
    genIfxJump (ic, "a");

  ic->generated = 1;
}

/*-----------------------------------------------------------------*/
/* genAddrOf - generates code for address of                       */
/*-----------------------------------------------------------------*/
static void
genAddrOf (iCode * ic)
{
  symbol *sym = OP_SYMBOL (IC_LEFT (ic));

  aopOp (IC_RESULT (ic), ic, FALSE, FALSE);

  /* if the operand is on the stack then we
     need to get the stack offset of this
     variable */
  if (IS_GB)
    {
      if (sym->onStack)
	{
	  spillCached ();
	  if (sym->stack <= 0)
	    {
	      emit2 ("!ldahlsp", sym->stack + _G.stack.pushed + _G.stack.offset);
	    }
	  else
	    {
	      emit2 ("!ldahlsp", sym->stack + _G.stack.pushed + _G.stack.offset + _G.stack.param_offset);
	    }
	  emit2 ("ld d,h");
	  emit2 ("ld e,l");
	}
      else
	{
	  emit2 ("ld de,!hashedstr", sym->rname);
	}
      aopPut (AOP (IC_RESULT (ic)), "e", 0);
      aopPut (AOP (IC_RESULT (ic)), "d", 1);
    }
  else
    {
      spillCached ();
      if (sym->onStack)
	{
	  /* if it has an offset  then we need to compute it */
	  if (sym->stack > 0)
	    emit2 ("ld hl,#%d+%d+%d+%d", sym->stack, _G.stack.pushed, _G.stack.offset, _G.stack.param_offset);
	  else
	    emit2 ("ld hl,#%d+%d+%d", sym->stack, _G.stack.pushed, _G.stack.offset);
	  emit2 ("add hl,sp");
	}
      else
	{
	  emit2 ("ld hl,#%s", sym->rname);
	}
      aopPut (AOP (IC_RESULT (ic)), "l", 0);
      aopPut (AOP (IC_RESULT (ic)), "h", 1);
    }
  freeAsmop (IC_RESULT (ic), NULL, ic);
}

/*-----------------------------------------------------------------*/
/* genAssign - generate code for assignment                        */
/*-----------------------------------------------------------------*/
static void
genAssign (iCode * ic)
{
  operand *result, *right;
  int size, offset;
  unsigned long lit = 0L;

  result = IC_RESULT (ic);
  right = IC_RIGHT (ic);

#if 1
  /* Dont bother assigning if they are the same */
  if (operandsEqu (IC_RESULT (ic), IC_RIGHT (ic)))
    {
      emit2 ("; (operands are equal %u)", operandsEqu (IC_RESULT (ic), IC_RIGHT (ic)));
      return;
    }
#endif

  aopOp (right, ic, FALSE, FALSE);
  aopOp (result, ic, TRUE, FALSE);

  /* if they are the same registers */
  if (sameRegs (AOP (right), AOP (result)))
    {
      emit2 ("; (registers are the same)");
      goto release;
    }

  /* if the result is a bit */
  if (AOP_TYPE (result) == AOP_CRY)
    {
      wassert (0);
    }

  /* general case */
  size = AOP_SIZE (result);
  offset = 0;

  if (AOP_TYPE (right) == AOP_LIT)
    lit = (unsigned long) floatFromVal (AOP (right)->aopu.aop_lit);
  if (isPair (AOP (result)))
    {
      fetchPair (getPairId (AOP (result)), AOP (right));
    }
  else if ((size > 1) &&
	   (AOP_TYPE (result) != AOP_REG) &&
	   (AOP_TYPE (right) == AOP_LIT) &&
	   !IS_FLOAT (operandType (right)) &&
	   (lit < 256L))
    {
      bool fXored = FALSE;
      offset = 0;
      /* Work from the top down.
         Done this way so that we can use the cached copy of 0
         in A for a fast clear */
      while (size--)
	{
	  if ((unsigned int) ((lit >> (offset * 8)) & 0x0FFL) == 0)
	    {
	      if (!fXored && size > 1)
		{
		  emit2 ("xor a,a");
		  fXored = TRUE;
		}
	      if (fXored)
		{
		  aopPut (AOP (result), "a", offset);
		}
	      else
		{
		  aopPut (AOP (result), "!zero", offset);
		}
	    }
	  else
	    aopPut (AOP (result),
		    aopGet (AOP (right), offset, FALSE),
		    offset);
	  offset++;
	}
    }
  else if (size == 2 && requiresHL (AOP (right)) && requiresHL (AOP (result)) && IS_GB)
    {
      /* Special case.  Load into a and d, then load out. */
      _moveA (aopGet (AOP (right), 0, FALSE));
      emit2 ("ld e,%s", aopGet (AOP (right), 1, FALSE));
      aopPut (AOP (result), "a", 0);
      aopPut (AOP (result), "e", 1);
    }
  else
    {
      while (size--)
	{
	  /* PENDING: do this check better */
	  if (requiresHL (AOP (right)) && requiresHL (AOP (result)))
	    {
	      _moveA (aopGet (AOP (right), offset, FALSE));
	      aopPut (AOP (result), "a", offset);
	    }
	  else
	    aopPut (AOP (result),
		    aopGet (AOP (right), offset, FALSE),
		    offset);
	  offset++;
	}
    }

release:
  freeAsmop (right, NULL, ic);
  freeAsmop (result, NULL, ic);
}

/*-----------------------------------------------------------------*/
/* genJumpTab - genrates code for jump table                       */
/*-----------------------------------------------------------------*/
static void
genJumpTab (iCode * ic)
{
  symbol *jtab;
  const char *l;

  aopOp (IC_JTCOND (ic), ic, FALSE, FALSE);
  /* get the condition into accumulator */
  l = aopGet (AOP (IC_JTCOND (ic)), 0, FALSE);
  if (!IS_GB)
    emit2 ("push de");
  emit2 ("ld e,%s", l);
  emit2 ("ld d,!zero");
  jtab = newiTempLabel (NULL);
  spillCached ();
  emit2 ("ld hl,!immed!tlabel", jtab->key + 100);
  emit2 ("add hl,de");
  emit2 ("add hl,de");
  emit2 ("add hl,de");
  freeAsmop (IC_JTCOND (ic), NULL, ic);
  if (!IS_GB)
    emit2 ("pop de");
  emit2 ("jp !*hl");
  emitLabel (jtab->key + 100);
  /* now generate the jump labels */
  for (jtab = setFirstItem (IC_JTLABELS (ic)); jtab;
       jtab = setNextItem (IC_JTLABELS (ic)))
    emit2 ("jp !tlabel", jtab->key + 100);
}

/*-----------------------------------------------------------------*/
/* genCast - gen code for casting                                  */
/*-----------------------------------------------------------------*/
static void
genCast (iCode * ic)
{
  operand *result = IC_RESULT (ic);
  sym_link *ctype = operandType (IC_LEFT (ic));
  operand *right = IC_RIGHT (ic);
  int size, offset;

  /* if they are equivalent then do nothing */
  if (operandsEqu (IC_RESULT (ic), IC_RIGHT (ic)))
    return;

  aopOp (right, ic, FALSE, FALSE);
  aopOp (result, ic, FALSE, FALSE);

  /* if the result is a bit */
  if (AOP_TYPE (result) == AOP_CRY)
    {
      wassert (0);
    }

  /* if they are the same size : or less */
  if (AOP_SIZE (result) <= AOP_SIZE (right))
    {

      /* if they are in the same place */
      if (sameRegs (AOP (right), AOP (result)))
	goto release;

      /* if they in different places then copy */
      size = AOP_SIZE (result);
      offset = 0;
      while (size--)
	{
	  aopPut (AOP (result),
		  aopGet (AOP (right), offset, FALSE),
		  offset);
	  offset++;
	}
      goto release;
    }

  /* PENDING: should be OK. */
#if 0
  /* if the result is of type pointer */
  if (IS_PTR (ctype))
    {
      wassert (0);
    }
#endif

  /* so we now know that the size of destination is greater
     than the size of the source */
  /* we move to result for the size of source */
  size = AOP_SIZE (right);
  offset = 0;
  while (size--)
    {
      aopPut (AOP (result),
	      aopGet (AOP (right), offset, FALSE),
	      offset);
      offset++;
    }

  /* now depending on the sign of the destination */
  size = AOP_SIZE (result) - AOP_SIZE (right);
  /* Unsigned or not an integral type - right fill with zeros */
  if (SPEC_USIGN (ctype) || !IS_SPEC (ctype))
    {
      while (size--)
	aopPut (AOP (result), "!zero", offset++);
    }
  else
    {
      /* we need to extend the sign :{ */
        const char *l = aopGet (AOP (right), AOP_SIZE (right) - 1,
			FALSE);
      _moveA (l);
      emit2 ("; genCast: sign extend untested.");
      emit2 ("rla ");
      emit2 ("sbc a,a");
      while (size--)
	aopPut (AOP (result), "a", offset++);
    }

release:
  freeAsmop (right, NULL, ic);
  freeAsmop (result, NULL, ic);
}

/*-----------------------------------------------------------------*/
/* genReceive - generate code for a receive iCode                  */
/*-----------------------------------------------------------------*/
static void
genReceive (iCode * ic)
{
  if (isOperandInFarSpace (IC_RESULT (ic)) &&
      (OP_SYMBOL (IC_RESULT (ic))->isspilt ||
       IS_TRUE_SYMOP (IC_RESULT (ic))))
    {
      wassert (0);
    }
  else
    {
        // PENDING: HACK
        int size;
        int i;

        aopOp (IC_RESULT (ic), ic, FALSE, FALSE);
        size = AOP_SIZE(IC_RESULT(ic));

        for (i = 0; i < size; i++) {
            aopPut(AOP(IC_RESULT(ic)), _fReceive[_G.receiveOffset++], i);
	}
    }

  freeAsmop (IC_RESULT (ic), NULL, ic);
}

/*-----------------------------------------------------------------*/
/* genZ80Code - generate code for Z80 based controllers            */
/*-----------------------------------------------------------------*/
void
genZ80Code (iCode * lic)
{
  iCode *ic;
  int cln = 0;

  /* HACK */
  if (IS_GB)
    {
      _fReturn = _gbz80_return;
      _fTmp = _gbz80_return;
    }
  else
    {
      _fReturn = _z80_return;
      _fTmp = _z80_return;
    }

  _G.lines.head = _G.lines.current = NULL;

  for (ic = lic; ic; ic = ic->next)
    {

      if (cln != ic->lineno)
	{
	  emit2 ("; %s %d", ic->filename, ic->lineno);
	  cln = ic->lineno;
	}
      /* if the result is marked as
         spilt and rematerializable or code for
         this has already been generated then
         do nothing */
      if (resultRemat (ic) || ic->generated)
	continue;

      /* depending on the operation */
      switch (ic->op)
	{
	case '!':
	  emit2 ("; genNot");
	  genNot (ic);
	  break;

	case '~':
	  emit2 ("; genCpl");
	  genCpl (ic);
	  break;

	case UNARYMINUS:
	  emit2 ("; genUminus");
	  genUminus (ic);
	  break;

	case IPUSH:
	  emit2 ("; genIpush");
	  genIpush (ic);
	  break;

	case IPOP:
	  /* IPOP happens only when trying to restore a
	     spilt live range, if there is an ifx statement
	     following this pop then the if statement might
	     be using some of the registers being popped which
	     would destory the contents of the register so
	     we need to check for this condition and handle it */
	  if (ic->next &&
	      ic->next->op == IFX &&
	      regsInCommon (IC_LEFT (ic), IC_COND (ic->next)))
	    {
	      emit2 ("; genIfx");
	      genIfx (ic->next, ic);
	    }
	  else
	    {
	      emit2 ("; genIpop");
	      genIpop (ic);
	    }
	  break;

	case CALL:
	  emit2 ("; genCall");
	  genCall (ic);
	  break;

	case PCALL:
	  emit2 ("; genPcall");
	  genPcall (ic);
	  break;

	case FUNCTION:
	  emit2 ("; genFunction");
	  genFunction (ic);
	  break;

	case ENDFUNCTION:
	  emit2 ("; genEndFunction");
	  genEndFunction (ic);
	  break;

	case RETURN:
	  emit2 ("; genRet");
	  genRet (ic);
	  break;

	case LABEL:
	  emit2 ("; genLabel");
	  genLabel (ic);
	  break;

	case GOTO:
	  emit2 ("; genGoto");
	  genGoto (ic);
	  break;

	case '+':
	  emit2 ("; genPlus");
	  genPlus (ic);
	  break;

	case '-':
	  emit2 ("; genMinus");
	  genMinus (ic);
	  break;

	case '*':
	  emit2 ("; genMult");
	  genMult (ic);
	  break;

	case '/':
	  emit2 ("; genDiv");
	  genDiv (ic);
	  break;

	case '%':
	  emit2 ("; genMod");
	  genMod (ic);
	  break;

	case '>':
	  emit2 ("; genCmpGt");
	  genCmpGt (ic, ifxForOp (IC_RESULT (ic), ic));
	  break;

	case '<':
	  emit2 ("; genCmpLt");
	  genCmpLt (ic, ifxForOp (IC_RESULT (ic), ic));
	  break;

	case LE_OP:
	case GE_OP:
	case NE_OP:

	  /* note these two are xlated by algebraic equivalence
	     during parsing SDCC.y */
	  werror (E_INTERNAL_ERROR, __FILE__, __LINE__,
		  "got '>=' or '<=' shouldn't have come here");
	  break;

	case EQ_OP:
	  emit2 ("; genCmpEq");
	  genCmpEq (ic, ifxForOp (IC_RESULT (ic), ic));
	  break;

	case AND_OP:
	  emit2 ("; genAndOp");
	  genAndOp (ic);
	  break;

	case OR_OP:
	  emit2 ("; genOrOp");
	  genOrOp (ic);
	  break;

	case '^':
	  emit2 ("; genXor");
	  genXor (ic, ifxForOp (IC_RESULT (ic), ic));
	  break;

	case '|':
	  emit2 ("; genOr");
	  genOr (ic, ifxForOp (IC_RESULT (ic), ic));
	  break;

	case BITWISEAND:
	  emit2 ("; genAnd");
	  genAnd (ic, ifxForOp (IC_RESULT (ic), ic));
	  break;

	case INLINEASM:
	  emit2 ("; genInline");
	  genInline (ic);
	  break;

	case RRC:
	  emit2 ("; genRRC");
	  genRRC (ic);
	  break;

	case RLC:
	  emit2 ("; genRLC");
	  genRLC (ic);
	  break;

	case GETHBIT:
	  emit2 ("; genHBIT");
	  wassert (0);

	case LEFT_OP:
	  emit2 ("; genLeftShift");
	  genLeftShift (ic);
	  break;

	case RIGHT_OP:
	  emit2 ("; genRightShift");
	  genRightShift (ic);
	  break;

	case GET_VALUE_AT_ADDRESS:
	  emit2 ("; genPointerGet");
	  genPointerGet (ic);
	  break;

	case '=':

	  if (POINTER_SET (ic))
	    {
	      emit2 ("; genAssign (pointer)");
	      genPointerSet (ic);
	    }
	  else
	    {
	      emit2 ("; genAssign");
	      genAssign (ic);
	    }
	  break;

	case IFX:
	  emit2 ("; genIfx");
	  genIfx (ic, NULL);
	  break;

	case ADDRESS_OF:
	  emit2 ("; genAddrOf");
	  genAddrOf (ic);
	  break;

	case JUMPTABLE:
	  emit2 ("; genJumpTab");
	  genJumpTab (ic);
	  break;

	case CAST:
	  emit2 ("; genCast");
	  genCast (ic);
	  break;

	case RECEIVE:
	  emit2 ("; genReceive");
	  genReceive (ic);
	  break;

	case SEND:
	  emit2 ("; addSet");
	  addSet (&_G.sendSet, ic);
	  break;

	default:
	  ic = ic;
	  /*      piCode(ic,stdout); */

	}
    }


  /* now we are ready to call the
     peep hole optimizer */
  if (!options.nopeep)
    peepHole (&_G.lines.head);

  /* This is unfortunate */
  /* now do the actual printing */
  {
    FILE *fp = codeOutFile;
    if (isInHome () && codeOutFile == code->oFile)
      codeOutFile = home->oFile;
    printLine (_G.lines.head, codeOutFile);
    if (_G.flushStatics)
      {
	flushStatics ();
	_G.flushStatics = 0;
      }
    codeOutFile = fp;
  }
}

/*
  Attic
static int
_isPairUsed (iCode * ic, PAIR_ID pairId)
{
  int ret = 0;
  switch (pairId)
    {
    case PAIR_DE:
      if (bitVectBitValue (ic->rMask, D_IDX))
	ret++;
      if (bitVectBitValue (ic->rMask, E_IDX))
	ret++;
      break;
    default:
      wassert (0);
    }
  return ret;
}

*/
