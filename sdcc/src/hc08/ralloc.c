/*------------------------------------------------------------------------

  SDCCralloc.c - source file for register allocation. 68HC08 specific

                Written By -  Sandeep Dutta . sandeep.dutta@usa.net (1998)

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

#include "common.h"
#include "ralloc.h"
#include "gen.h"

/*-----------------------------------------------------------------*/
/* At this point we start getting processor specific although      */
/* some routines are non-processor specific & can be reused when   */
/* targetting other processors. The decision for this will have    */
/* to be made on a routine by routine basis                        */
/* routines used to pack registers are most definitely not reusable */
/* since the pack the registers depending strictly on the MCU      */
/*-----------------------------------------------------------------*/

extern void genhc08Code (iCode *);
#define D(x)

/* Global data */
static struct
  {
    bitVect *spiltSet;
    set *stackSpil;
    bitVect *regAssigned;
    bitVect *totRegAssigned;    /* final set of LRs that got into registers */
    short blockSpil;
    int slocNum;
    bitVect *funcrUsed;         /* registers used in a function */
    int stackExtend;
    int dataExtend;
  }
_G;

/* Shared with gen.c */
int hc08_ptrRegReq;             /* one byte pointer register required */

/* 8051 registers */
reg_info regshc08[] =
{

  {REG_GPR, A_IDX,   "a",  1, NULL, 0, 1},
  {REG_GPR, X_IDX,   "x",  2, NULL, 0, 1},
  {REG_GPR, H_IDX,   "h",  4, NULL, 0, 1},
  {REG_PTR, HX_IDX,  "hx", 6, NULL, 0, 1},
  {REG_GPR, XA_IDX,  "xa", 3, NULL, 0, 1},

  {REG_CND, CND_IDX, "C",  0, NULL, 0, 1},
  {0,       SP_IDX,  "sp", 0, NULL, 0, 1},
};
int hc08_nRegs = 7;

reg_info *hc08_reg_a;
reg_info *hc08_reg_x;
reg_info *hc08_reg_h;
reg_info *hc08_reg_hx;
reg_info *hc08_reg_xa;
reg_info *hc08_reg_sp;

static void spillThis (symbol *);
static void freeAllRegs ();

/*-----------------------------------------------------------------*/
/* allocReg - allocates register of given type                     */
/*-----------------------------------------------------------------*/
static reg_info *
allocReg (short type)
{
  return NULL;

  if ((type==REG_PTR) && (regshc08[HX_IDX].isFree))
    {
      regshc08[HX_IDX].isFree = 0;
      if (currFunc)
        currFunc->regsUsed =
          bitVectSetBit (currFunc->regsUsed, HX_IDX);
      return &regshc08[HX_IDX];
    }
  return NULL;
}

/*-----------------------------------------------------------------*/
/* hc08_regWithIdx - returns pointer to register with index number */
/*-----------------------------------------------------------------*/
reg_info *
hc08_regWithIdx (int idx)
{
  int i;

  for (i = 0; i < hc08_nRegs; i++)
    if (regshc08[i].rIdx == idx)
      return &regshc08[i];

  werror (E_INTERNAL_ERROR, __FILE__, __LINE__,
          "regWithIdx not found");
  exit (1);
}

/*-----------------------------------------------------------------*/
/* hc08_freeReg - frees a register                                      */
/*-----------------------------------------------------------------*/
void
hc08_freeReg (reg_info * reg)
{
  if (!reg)
    {
      werror (E_INTERNAL_ERROR, __FILE__, __LINE__,
              "hc08_freeReg - Freeing NULL register");
      exit (1);
    }

  reg->isFree = 1;

  switch (reg->rIdx)
    {
      case A_IDX:
        if (hc08_reg_x->isFree)
          hc08_reg_xa->isFree = 1;
        break;
      case X_IDX:
        if (hc08_reg_a->isFree)
          hc08_reg_xa->isFree = 1;
        if (hc08_reg_h->isFree)
          hc08_reg_hx->isFree = 1;
        break;
      case H_IDX:
        if (hc08_reg_x->isFree)
          hc08_reg_hx->isFree = 1;
        break;
      case HX_IDX:
        hc08_reg_h->isFree = 1;
        hc08_reg_x->isFree = 1;
        if (hc08_reg_a->isFree)
          hc08_reg_xa->isFree = 1;
        break;
      case XA_IDX:
        hc08_reg_x->isFree = 1;
        hc08_reg_a->isFree = 1;
        if (hc08_reg_h->isFree)
          hc08_reg_hx->isFree = 1;
        break;
      default:
        break;
    }
}


/*-----------------------------------------------------------------*/
/* nFreeRegs - returns number of free registers                    */
/*-----------------------------------------------------------------*/
static int
nFreeRegs (int type)
{
  int i;
  int nfr = 0;

  return 0;

  for (i = 0; i < hc08_nRegs; i++)
    if (regshc08[i].isFree && regshc08[i].type == type)
      nfr++;
  return nfr;
}

/*-----------------------------------------------------------------*/
/* nfreeRegsType - free registers with type                         */
/*-----------------------------------------------------------------*/
static int
nfreeRegsType (int type)
{
  int nfr;
  if (type == REG_PTR)
    {
      if ((nfr = nFreeRegs (type)) == 0)
        return nFreeRegs (REG_GPR);
    }

  return nFreeRegs (type);
}

/*-----------------------------------------------------------------*/
/* hc08_useReg - marks a register  as used                         */
/*-----------------------------------------------------------------*/
void
hc08_useReg (reg_info * reg)
{
  reg->isFree = 0;

  switch (reg->rIdx)
    {
      case A_IDX:
        hc08_reg_xa->aop = NULL;
        hc08_reg_xa->isFree = 0;
        break;
      case X_IDX:
        hc08_reg_xa->aop = NULL;
        hc08_reg_xa->isFree = 0;
        hc08_reg_hx->aop = NULL;
        hc08_reg_hx->isFree = 0;
        break;
      case H_IDX:
        hc08_reg_hx->aop = NULL;
        hc08_reg_hx->isFree = 0;
        break;
      case HX_IDX:
        hc08_reg_h->aop = NULL;
        hc08_reg_h->isFree = 0;
        hc08_reg_x->aop = NULL;
        hc08_reg_x->isFree = 0;
        break;
      case XA_IDX:
        hc08_reg_x->aop = NULL;
        hc08_reg_x->isFree = 0;
        hc08_reg_a->aop = NULL;
        hc08_reg_a->isFree = 0;
        break;
      default:
        break;
    }
}

/*-----------------------------------------------------------------*/
/* hc08_dirtyReg - marks a register as dirty                       */
/*-----------------------------------------------------------------*/
void
hc08_dirtyReg (reg_info * reg, bool freereg)
{
  reg->aop = NULL;

  switch (reg->rIdx)
    {
      case A_IDX:
        hc08_reg_xa->aop = NULL;
        break;
      case X_IDX:
        hc08_reg_xa->aop = NULL;
        hc08_reg_hx->aop = NULL;
        break;
      case H_IDX:
        hc08_reg_hx->aop = NULL;
        break;
      case HX_IDX:
        hc08_reg_h->aop = NULL;
        hc08_reg_x->aop = NULL;
        break;
      case XA_IDX:
        hc08_reg_x->aop = NULL;
        hc08_reg_a->aop = NULL;
        break;
      default:
        break;
    }
  if (freereg)
    hc08_freeReg(reg);
}

/*-----------------------------------------------------------------*/
/* computeSpillable - given a point find the spillable live ranges */
/*-----------------------------------------------------------------*/
static bitVect *
computeSpillable (iCode * ic)
{
  bitVect *spillable;

  /* spillable live ranges are those that are live at this
     point . the following categories need to be subtracted
     from this set.
     a) - those that are already spilt
     b) - if being used by this one
     c) - defined by this one */

  spillable = bitVectCopy (ic->rlive);
  spillable =
    bitVectCplAnd (spillable, _G.spiltSet);     /* those already spilt */
  spillable =
    bitVectCplAnd (spillable, ic->uses);        /* used in this one */
  bitVectUnSetBit (spillable, ic->defKey);
  spillable = bitVectIntersect (spillable, _G.regAssigned);
  return spillable;

}

/*-----------------------------------------------------------------*/
/* noSpilLoc - return true if a variable has no spil location      */
/*-----------------------------------------------------------------*/
static int
noSpilLoc (symbol * sym, eBBlock * ebp, iCode * ic)
{
  return (sym->usl.spillLoc ? 0 : 1);
}

/*-----------------------------------------------------------------*/
/* hasSpilLoc - will return 1 if the symbol has spil location      */
/*-----------------------------------------------------------------*/
static int
hasSpilLoc (symbol * sym, eBBlock * ebp, iCode * ic)
{
  return (sym->usl.spillLoc ? 1 : 0);
}

/*-----------------------------------------------------------------*/
/* directSpilLoc - will return 1 if the spillocation is in direct  */
/*-----------------------------------------------------------------*/
static int
directSpilLoc (symbol * sym, eBBlock * ebp, iCode * ic)
{
  if (sym->usl.spillLoc &&
      (IN_DIRSPACE (SPEC_OCLS (sym->usl.spillLoc->etype))))
    return 1;
  else
    return 0;
}

/*-----------------------------------------------------------------*/
/* hasSpilLocnoUptr - will return 1 if the symbol has spil location */
/*                    but is not used as a pointer                 */
/*-----------------------------------------------------------------*/
static int
hasSpilLocnoUptr (symbol * sym, eBBlock * ebp, iCode * ic)
{
  return ((sym->usl.spillLoc && !sym->uptr) ? 1 : 0);
}

/*-----------------------------------------------------------------*/
/* rematable - will return 1 if the remat flag is set              */
/*-----------------------------------------------------------------*/
static int
rematable (symbol * sym, eBBlock * ebp, iCode * ic)
{
  return sym->remat;
}

/*-----------------------------------------------------------------*/
/* notUsedInRemaining - not used or defined in remain of the block */
/*-----------------------------------------------------------------*/
static int
notUsedInRemaining (symbol * sym, eBBlock * ebp, iCode * ic)
{
  return ((usedInRemaining (operandFromSymbol (sym), ic) ? 0 : 1) &&
          allDefsOutOfRange (sym->defs, ebp->fSeq, ebp->lSeq));
}

/*-----------------------------------------------------------------*/
/* allLRs - return true for all                                    */
/*-----------------------------------------------------------------*/
static int
allLRs (symbol * sym, eBBlock * ebp, iCode * ic)
{
  return 1;
}

/*-----------------------------------------------------------------*/
/* liveRangesWith - applies function to a given set of live range  */
/*-----------------------------------------------------------------*/
static set *
liveRangesWith (bitVect * lrs, int (func) (symbol *, eBBlock *, iCode *),
                eBBlock * ebp, iCode * ic)
{
  set *rset = NULL;
  int i;

  if (!lrs || !lrs->size)
    return NULL;

  for (i = 1; i < lrs->size; i++)
    {
      symbol *sym;
      if (!bitVectBitValue (lrs, i))
        continue;

      /* if we don't find it in the live range
         hash table we are in serious trouble */
      if (!(sym = hTabItemWithKey (liveRanges, i)))
        {
          werror (E_INTERNAL_ERROR, __FILE__, __LINE__,
                  "liveRangesWith could not find liveRange");
          exit (1);
        }

      if (func (sym, ebp, ic) && bitVectBitValue (_G.regAssigned, sym->key))
        addSetHead (&rset, sym);
    }

  return rset;
}


/*-----------------------------------------------------------------*/
/* leastUsedLR - given a set determines which is the least used    */
/*-----------------------------------------------------------------*/
static symbol *
leastUsedLR (set * sset)
{
  symbol *sym = NULL, *lsym = NULL;

  sym = lsym = setFirstItem (sset);

  if (!lsym)
    return NULL;

  for (; lsym; lsym = setNextItem (sset))
    {
      /* if usage is the same then prefer
         to spill the smaller of the two */
      if (lsym->used == sym->used)
        if (getSize (lsym->type) < getSize (sym->type))
          sym = lsym;

      /* if less usage */
      if (lsym->used < sym->used)
        sym = lsym;
    }

  setToNull ((void *) &sset);
  sym->blockSpil = 0;
  return sym;
}

/*-----------------------------------------------------------------*/
/* noOverLap - will iterate through the list looking for over lap  */
/*-----------------------------------------------------------------*/
static int
noOverLap (set * itmpStack, symbol * fsym)
{
  symbol *sym;

  for (sym = setFirstItem (itmpStack); sym;
       sym = setNextItem (itmpStack))
    {
        if (bitVectBitValue(sym->clashes,fsym->key)) return 0;
    }
  return 1;
}

/*-----------------------------------------------------------------*/
/* isFree - will return 1 if the a free spil location is found     */
/*-----------------------------------------------------------------*/
static
DEFSETFUNC (isFree)
{
  symbol *sym = item;
  V_ARG (symbol **, sloc);
  V_ARG (symbol *, fsym);

  /* if already found */
  if (*sloc)
    return 0;

  /* if it is free && and the itmp assigned to
     this does not have any overlapping live ranges
     with the one currently being assigned and
     the size can be accomodated  */
  if (sym->isFree &&
      noOverLap (sym->usl.itmpStack, fsym) &&
      getSize (sym->type) >= getSize (fsym->type))
    {
      *sloc = sym;
      return 1;
    }

  return 0;
}

/*-----------------------------------------------------------------*/
/* spillLRWithPtrReg :- will spil those live ranges which use PTR  */
/*-----------------------------------------------------------------*/
static void
spillLRWithPtrReg (symbol * forSym)
{
  symbol *lrsym;
  reg_info *hx;
  int k;

  if (!_G.regAssigned || bitVectIsZero (_G.regAssigned))
    return;

  hx = hc08_regWithIdx (HX_IDX);

  /* for all live ranges */
  for (lrsym = hTabFirstItem (liveRanges, &k); lrsym;
       lrsym = hTabNextItem (liveRanges, &k))
    {
      int j;

      /* if no registers assigned to it or spilt */
      /* if it does not overlap this then
         no need to spill it */

      if (lrsym->isspilt || !lrsym->nRegs ||
          (lrsym->liveTo < forSym->liveFrom))
        continue;

      /* go thru the registers : if it is either
         r0 or r1 then spil it */
      for (j = 0; j < lrsym->nRegs; j++)
        if (lrsym->regs[j] == hx)
          {
            spillThis (lrsym);
            break;
          }
    }
}

/*-----------------------------------------------------------------*/
/* createStackSpil - create a location on the stack to spil        */
/*-----------------------------------------------------------------*/
static symbol *
createStackSpil (symbol * sym)
{
  symbol *sloc = NULL;
  int useXstack, model;

  char slocBuffer[30];

  /* first go try and find a free one that is already
     existing on the stack */
  if (applyToSet (_G.stackSpil, isFree, &sloc, sym))
    {
      /* found a free one : just update & return */
      sym->usl.spillLoc = sloc;
      sym->stackSpil = 1;
      sloc->isFree = 0;
      addSetHead (&sloc->usl.itmpStack, sym);
      return sym;
    }

  /* could not then have to create one , this is the hard part
     we need to allocate this on the stack : this is really a
     hack!! but cannot think of anything better at this time */

  if (sprintf (slocBuffer, "sloc%d", _G.slocNum++) >= sizeof (slocBuffer))
    {
      fprintf (stderr, "***Internal error: slocBuffer overflowed: %s:%d\n",
               __FILE__, __LINE__);
      exit (1);
    }

  sloc = newiTemp (slocBuffer);

  /* set the type to the spilling symbol */
  sloc->type = copyLinkChain (sym->type);
  sloc->etype = getSpec (sloc->type);
  SPEC_SCLS (sloc->etype) = S_DATA;
  SPEC_EXTR (sloc->etype) = 0;
  SPEC_STAT (sloc->etype) = 0;
  SPEC_VOLATILE(sloc->etype) = 0;
  SPEC_ABSA(sloc->etype) = 0;

  /* we don't allow it to be allocated
     onto the external stack since : so we
     temporarily turn it off ; we also
     turn off memory model to prevent
     the spil from going to the external storage
   */

  useXstack = options.useXstack;
  model = options.model;
/*     noOverlay = options.noOverlay; */
/*     options.noOverlay = 1; */
  options.model = options.useXstack = 0;

  allocLocal (sloc);

  options.useXstack = useXstack;
  options.model = model;
/*     options.noOverlay = noOverlay; */
  sloc->isref = 1;              /* to prevent compiler warning */

  /* if it is on the stack then update the stack */
  if (IN_STACK (sloc->etype))
    {
      currFunc->stack += getSize (sloc->type);
      _G.stackExtend += getSize (sloc->type);
    }
  else
    _G.dataExtend += getSize (sloc->type);

  /* add it to the _G.stackSpil set */
  addSetHead (&_G.stackSpil, sloc);
  sym->usl.spillLoc = sloc;
  sym->stackSpil = 1;

  /* add it to the set of itempStack set
     of the spill location */
  addSetHead (&sloc->usl.itmpStack, sym);
  return sym;
}

/*-----------------------------------------------------------------*/
/* isSpiltOnStack - returns true if the spil location is on stack  */
/*-----------------------------------------------------------------*/
static bool
isSpiltOnStack (symbol * sym)
{
  sym_link *etype;

  if (!sym)
    return FALSE;

  if (!sym->isspilt)
    return FALSE;

/*     if (sym->_G.stackSpil) */
/*      return TRUE; */

  if (!sym->usl.spillLoc)
    return FALSE;

  etype = getSpec (sym->usl.spillLoc->type);
  if (IN_STACK (etype))
    return TRUE;

  return FALSE;
}

/*-----------------------------------------------------------------*/
/* spillThis - spils a specific operand                            */
/*-----------------------------------------------------------------*/
static void
spillThis (symbol * sym)
{
  int i;
  /* if this is rematerializable or has a spillLocation
     we are okay, else we need to create a spillLocation
     for it */
  if (!(sym->remat || sym->usl.spillLoc))
    createStackSpil (sym);

  /* mark it as spilt & put it in the spilt set */
  sym->isspilt = sym->spillA = 1;
  _G.spiltSet = bitVectSetBit (_G.spiltSet, sym->key);

  bitVectUnSetBit (_G.regAssigned, sym->key);
  bitVectUnSetBit (_G.totRegAssigned, sym->key);

  for (i = 0; i < sym->nRegs; i++)
    {
      if (sym->regs[i])
        {
          hc08_freeReg (sym->regs[i]);
          sym->regs[i] = NULL;
        }
    }

//  /* if spilt on stack then free up r0 & r1
//     if they could have been assigned to some
//     LIVE ranges */
//  if (!hc08_ptrRegReq && isSpiltOnStack (sym))
//    {
//      hc08_ptrRegReq++;
//      spillLRWithPtrReg (sym);
//    }

  if (sym->usl.spillLoc && !sym->remat)
    sym->usl.spillLoc->allocreq++;
  return;
}

/*-----------------------------------------------------------------*/
/* selectSpil - select a iTemp to spil : rather a simple procedure */
/*-----------------------------------------------------------------*/
static symbol *
selectSpil (iCode * ic, eBBlock * ebp, symbol * forSym)
{
  bitVect *lrcs = NULL;
  set *selectS;
  symbol *sym;

  /* get the spillable live ranges */
  lrcs = computeSpillable (ic);

  /* get all live ranges that are rematerializable */
  if ((selectS = liveRangesWith (lrcs, rematable, ebp, ic)))
    {
      /* return the least used of these */
      return leastUsedLR (selectS);
    }

  /* get live ranges with spillLocations in direct space */
  if ((selectS = liveRangesWith (lrcs, directSpilLoc, ebp, ic)))
    {
      sym = leastUsedLR (selectS);
      strncpyz (sym->rname,
                sym->usl.spillLoc->rname[0] ?
                   sym->usl.spillLoc->rname : sym->usl.spillLoc->name,
                sizeof(sym->rname));
      sym->spildir = 1;
      /* mark it as allocation required */
      sym->usl.spillLoc->allocreq++;
      return sym;
    }

  /* if the symbol is local to the block then */
  if (forSym->liveTo < ebp->lSeq)
    {
      /* check if there are any live ranges allocated
         to registers that are not used in this block */
      if (!_G.blockSpil && (selectS = liveRangesWith (lrcs, notUsedInBlock, ebp, ic)))
        {
          sym = leastUsedLR (selectS);
          /* if this is not rematerializable */
          if (!sym->remat)
            {
              _G.blockSpil++;
              sym->blockSpil = 1;
            }
          return sym;
        }

      /* check if there are any live ranges that are
         not used in the remainder of the block */
      if (!_G.blockSpil &&
          !isiCodeInFunctionCall (ic) &&
          (selectS = liveRangesWith (lrcs, notUsedInRemaining, ebp, ic)))
        {
          sym = leastUsedLR (selectS);
          if (sym != forSym)
            {
              if (!sym->remat)
                {
                  sym->remainSpil = 1;
                  _G.blockSpil++;
                }
              return sym;
            }
        }
    }

  /* find live ranges with spillocation && not used as pointers */
  if ((selectS = liveRangesWith (lrcs, hasSpilLocnoUptr, ebp, ic)))
    {
      sym = leastUsedLR (selectS);
      /* mark this as allocation required */
      sym->usl.spillLoc->allocreq++;
      return sym;
    }

  /* find live ranges with spillocation */
  if ((selectS = liveRangesWith (lrcs, hasSpilLoc, ebp, ic)))
    {
      sym = leastUsedLR (selectS);
      sym->usl.spillLoc->allocreq++;
      return sym;
    }

  /* couldn't find then we need to create a spil
     location on the stack, for which one?
     the least used ofcourse */
  if ((selectS = liveRangesWith (lrcs, noSpilLoc, ebp, ic)))
    {
      /* return a created spil location */
      sym = createStackSpil (leastUsedLR (selectS));
      sym->usl.spillLoc->allocreq++;
      return sym;
    }

  /* this is an extreme situation we will spill
     this one : happens very rarely but it does happen */
  spillThis (forSym);
  return forSym;
}

/*-----------------------------------------------------------------*/
/* spilSomething - spil some variable & mark registers as free     */
/*-----------------------------------------------------------------*/
static bool
spilSomething (iCode * ic, eBBlock * ebp, symbol * forSym)
{
  symbol *ssym;
  int i;

  /* get something we can spil */
  ssym = selectSpil (ic, ebp, forSym);

  /* mark it as spilt */
  ssym->isspilt = ssym->spillA = 1;
  _G.spiltSet = bitVectSetBit (_G.spiltSet, ssym->key);

  /* mark it as not register assigned &
     take it away from the set */
  bitVectUnSetBit (_G.regAssigned, ssym->key);
  bitVectUnSetBit (_G.totRegAssigned, ssym->key);

  /* mark the registers as free */
  for (i = 0; i < ssym->nRegs; i++)
    if (ssym->regs[i])
      hc08_freeReg (ssym->regs[i]);

  /* if spilt on stack then free up hx
     if it could have been assigned to as gprs */
  if (!hc08_ptrRegReq && isSpiltOnStack (ssym))
    {
      hc08_ptrRegReq++;
      spillLRWithPtrReg (ssym);
    }

  /* if this was a block level spil then insert push & pop
     at the start & end of block respectively */
  if (ssym->blockSpil)
    {
      iCode *nic = newiCode (IPUSH, operandFromSymbol (ssym), NULL);
      /* add push to the start of the block */
      addiCodeToeBBlock (ebp, nic, (ebp->sch->op == LABEL ?
                                    ebp->sch->next : ebp->sch));
      nic = newiCode (IPOP, operandFromSymbol (ssym), NULL);
      /* add pop to the end of the block */
      addiCodeToeBBlock (ebp, nic, NULL);
    }

  /* if spilt because not used in the remainder of the
     block then add a push before this instruction and
     a pop at the end of the block */
  if (ssym->remainSpil)
    {
      iCode *nic = newiCode (IPUSH, operandFromSymbol (ssym), NULL);
      /* add push just before this instruction */
      addiCodeToeBBlock (ebp, nic, ic);

      nic = newiCode (IPOP, operandFromSymbol (ssym), NULL);
      /* add pop to the end of the block */
      addiCodeToeBBlock (ebp, nic, NULL);
    }

  if (ssym == forSym)
    return FALSE;
  else
    return TRUE;
}

/*-----------------------------------------------------------------*/
/* getRegPtr - will try for PTR if not a GPR type if not spil      */
/*-----------------------------------------------------------------*/
static reg_info *
getRegPtr (iCode * ic, eBBlock * ebp, symbol * sym)
{
  reg_info *reg;

tryAgain:
  /* try for a ptr type */
  if ((reg = allocReg (REG_PTR)))
    return reg;

  /* try for gpr type */
  if ((reg = allocReg (REG_GPR)))
    return reg;

  /* we have to spil */
  if (!spilSomething (ic, ebp, sym))
    return NULL;

  /* this looks like an infinite loop but
     in really selectSpil will abort  */
  goto tryAgain;
}

/*-----------------------------------------------------------------*/
/* getRegGpr - will try for GPR if not spil                        */
/*-----------------------------------------------------------------*/
static reg_info *
getRegGpr (iCode * ic, eBBlock * ebp, symbol * sym)
{
  reg_info *reg;

tryAgain:
  /* try for gpr type */
  if ((reg = allocReg (REG_GPR)))
    return reg;

  if (!hc08_ptrRegReq)
    if ((reg = allocReg (REG_PTR)))
      return reg;

  /* we have to spil */
  if (!spilSomething (ic, ebp, sym))
    return NULL;

  /* this looks like an infinite loop but
     in really selectSpil will abort  */
  goto tryAgain;
}

/*-----------------------------------------------------------------*/
/* getRegPtrNoSpil - get it cannot be spilt                        */
/*-----------------------------------------------------------------*/
static reg_info *getRegPtrNoSpil()
{
  reg_info *reg;

  /* try for a ptr type */
  if ((reg = allocReg (REG_PTR)))
    return reg;

  /* try for gpr type */
  if ((reg = allocReg (REG_GPR)))
    return reg;

  assert(0);

  /* just to make the compiler happy */
  return 0;
}

/*-----------------------------------------------------------------*/
/* getRegGprNoSpil - get it cannot be spilt                        */
/*-----------------------------------------------------------------*/
static reg_info *getRegGprNoSpil()
{
  reg_info *reg;
  if ((reg = allocReg (REG_GPR)))
    return reg;

  if (!hc08_ptrRegReq)
    if ((reg = allocReg (REG_PTR)))
      return reg;

  assert(0);

  /* just to make the compiler happy */
  return 0;
}

/*-----------------------------------------------------------------*/
/* symHasReg - symbol has a given register                         */
/*-----------------------------------------------------------------*/
static bool
symHasReg (symbol * sym, reg_info * reg)
{
  int i;

  for (i = 0; i < sym->nRegs; i++)
    if (sym->regs[i] == reg)
      return TRUE;

  return FALSE;
}

/*-----------------------------------------------------------------*/
/* updateRegUsage -  update the registers in use at the start of   */
/*                   this icode                                    */
/*-----------------------------------------------------------------*/
static void
updateRegUsage (iCode * ic)
{
  int reg;

  // update the registers in use at the start of this icode
  for (reg=0; reg<hc08_nRegs; reg++)
    {
      if (regshc08[reg].isFree)
        {
          ic->riu &= ~(regshc08[reg].mask);
        }
      else
        {
          ic->riu |= (regshc08[reg].mask);
        }
    }
}

/*-----------------------------------------------------------------*/
/* deassignLRs - check the live to and if they have registers & are */
/*               not spilt then free up the registers              */
/*-----------------------------------------------------------------*/
static void
deassignLRs (iCode * ic, eBBlock * ebp)
{
  symbol *sym;
  int k;
  symbol *result;

  for (sym = hTabFirstItem (liveRanges, &k); sym;
       sym = hTabNextItem (liveRanges, &k))
    {
      symbol *psym = NULL;
      /* if it does not end here */
      if (sym->liveTo > ic->seq)
        continue;

      /* if it was spilt on stack then we can
         mark the stack spil location as free */
      if (sym->isspilt)
        {
          if (sym->stackSpil)
            {
              sym->usl.spillLoc->isFree = 1;
              sym->stackSpil = 0;
            }
          continue;
        }

      if (!bitVectBitValue (_G.regAssigned, sym->key))
        continue;

      /* special case check if this is an IFX &
         the privious one was a pop and the
         previous one was not spilt then keep track
         of the symbol */
      if (ic->op == IFX && ic->prev &&
          ic->prev->op == IPOP &&
          !ic->prev->parmPush &&
          !OP_SYMBOL (IC_LEFT (ic->prev))->isspilt)
        psym = OP_SYMBOL (IC_LEFT (ic->prev));

      if (sym->nRegs)
        {
          int i = 0;

          bitVectUnSetBit (_G.regAssigned, sym->key);

          /* if the result of this one needs registers
             and does not have it then assign it right
             away */
          if (IC_RESULT (ic) &&
              !(SKIP_IC2 (ic) ||        /* not a special icode */
                ic->op == JUMPTABLE ||
                ic->op == IFX ||
                ic->op == IPUSH ||
                ic->op == IPOP ||
                ic->op == RETURN ||
                POINTER_SET (ic)) &&
              (result = OP_SYMBOL (IC_RESULT (ic))) &&  /* has a result */
              result->liveTo > ic->seq &&       /* and will live beyond this */
              result->liveTo <= ebp->lSeq &&    /* does not go beyond this block */
              result->liveFrom == ic->seq &&    /* does not start before here */
              result->regType == sym->regType &&        /* same register types */
              result->nRegs &&  /* which needs registers */
              !result->isspilt &&       /* and does not already have them */
              !result->remat &&
              !bitVectBitValue (_G.regAssigned, result->key) &&
          /* the number of free regs + number of regs in this LR
             can accomodate the what result Needs */
              ((nfreeRegsType (result->regType) +
                sym->nRegs) >= result->nRegs)
             )
            {
              for (i = 0; i < result->nRegs; i++)
                if (i < sym->nRegs)
                  result->regs[i] = sym->regs[i];
                else
                  result->regs[i] = getRegGpr (ic, ebp, result);

              _G.regAssigned = bitVectSetBit (_G.regAssigned, result->key);
              _G.totRegAssigned = bitVectSetBit (_G.totRegAssigned, result->key);

            }

          /* free the remaining */
          for (; i < sym->nRegs; i++)
            {
              if (psym)
                {
                  if (!symHasReg (psym, sym->regs[i]))
                    hc08_freeReg (sym->regs[i]);
                }
              else
                hc08_freeReg (sym->regs[i]);
            }
        }
    }
}


/*-----------------------------------------------------------------*/
/* reassignLR - reassign this to registers                         */
/*-----------------------------------------------------------------*/
static void
reassignLR (operand * op)
{
  symbol *sym = OP_SYMBOL (op);
  int i;

  /* not spilt any more */
  sym->isspilt = sym->spillA = sym->blockSpil = sym->remainSpil = 0;
  bitVectUnSetBit (_G.spiltSet, sym->key);

  _G.regAssigned = bitVectSetBit (_G.regAssigned, sym->key);
  _G.totRegAssigned = bitVectSetBit (_G.totRegAssigned, sym->key);

  _G.blockSpil--;

  for (i = 0; i < sym->nRegs; i++)
    sym->regs[i]->isFree = 0;
}

/*-----------------------------------------------------------------*/
/* willCauseSpill - determines if allocating will cause a spill    */
/*-----------------------------------------------------------------*/
static int
willCauseSpill (int nr, int rt)
{
  /* first check if there are any available registers
     of the type required */
  if (rt == REG_PTR)
    {
      /* special case for pointer type
         if pointer type not avlb then
         check for type gpr */
      if (nFreeRegs (rt) >= nr)
        return 0;
      if (nFreeRegs (REG_GPR) >= nr)
        return 0;
    }
  else
    {
      if (hc08_ptrRegReq)
        {
          if (nFreeRegs (rt) >= nr)
            return 0;
        }
      else
        {
          if (nFreeRegs (REG_PTR) +
              nFreeRegs (REG_GPR) >= nr)
            return 0;
        }
    }

  /* it will cause a spil */
  return 1;
}

/*-----------------------------------------------------------------*/
/* positionRegs - the allocator can allocate same registers to res- */
/* ult and operand, if this happens make sure they are in the same */
/* position as the operand otherwise chaos results                 */
/*-----------------------------------------------------------------*/
static int
positionRegs (symbol * result, symbol * opsym)
{
  int count = min (result->nRegs, opsym->nRegs);
  int i, j = 0, shared = 0;
  int change = 0;

  /* if the result has been spilt then cannot share */
  if (opsym->isspilt)
    return 0;
again:
  shared = 0;
  /* first make sure that they actually share */
  for (i = 0; i < count; i++)
    {
      for (j = 0; j < count; j++)
        {
          if (result->regs[i] == opsym->regs[j] && i != j)
            {
              shared = 1;
              goto xchgPositions;
            }
        }
    }
xchgPositions:
  if (shared)
    {
      reg_info *tmp = result->regs[i];
      result->regs[i] = result->regs[j];
      result->regs[j] = tmp;
      change ++;
      goto again;
    }
  return change;
}

/*------------------------------------------------------------------*/
/* verifyRegsAssigned - make sure an iTemp is properly initialized; */
/* it should either have registers or have beed spilled. Otherwise, */
/* there was an uninitialized variable, so just spill this to get   */
/* the operand in a valid state.                                    */
/*------------------------------------------------------------------*/
static void
verifyRegsAssigned (operand *op, iCode * ic)
{
  symbol * sym;

  if (!op) return;
  if (!IS_ITEMP (op)) return;

  sym = OP_SYMBOL (op);
  if (sym->isspilt) return;
  if (!sym->nRegs) return;
  if (sym->regs[0]) return;

  werrorfl (ic->filename, ic->lineno, W_LOCAL_NOINIT,
            sym->prereqv ? sym->prereqv->name : sym->name);
  spillThis (sym);
}


/*-----------------------------------------------------------------*/
/* serialRegAssign - serially allocate registers to the variables  */
/*-----------------------------------------------------------------*/
static void
serialRegAssign (eBBlock ** ebbs, int count)
{
  int i;

  /* for all blocks */
  for (i = 0; i < count; i++)
    {
      iCode *ic;

      if (ebbs[i]->noPath &&
          (ebbs[i]->entryLabel != entryLabel &&
           ebbs[i]->entryLabel != returnLabel))
        continue;

      /* for all instructions do */
      for (ic = ebbs[i]->sch; ic; ic = ic->next)
        {
          updateRegUsage(ic);

          /* if this is an ipop that means some live
             range will have to be assigned again */
          if (ic->op == IPOP)
              reassignLR (IC_LEFT (ic));

          /* if result is present && is a true symbol */
          if (IC_RESULT (ic) && ic->op != IFX &&
              IS_TRUE_SYMOP (IC_RESULT (ic)))
            {
              OP_SYMBOL (IC_RESULT (ic))->allocreq++;
            }

          /* take away registers from live
             ranges that end at this instruction */
          deassignLRs (ic, ebbs[i]);

          /* some don't need registers */
          if (SKIP_IC2 (ic) ||
              ic->op == JUMPTABLE ||
              ic->op == IFX ||
              ic->op == IPUSH ||
              ic->op == IPOP ||
              (IC_RESULT (ic) && POINTER_SET (ic)))
            {
              continue;
            }

          /* now we need to allocate registers only for the result */
          if (IC_RESULT (ic))
            {
              symbol *sym = OP_SYMBOL (IC_RESULT (ic));
              bitVect *spillable;
              int willCS;
              int j;
              int ptrRegSet = 0;

              /* Make sure any spill location is definitely allocated */
              if (sym->isspilt && !sym->remat && sym->usl.spillLoc &&
                  !sym->usl.spillLoc->allocreq)
                {
                  sym->usl.spillLoc->allocreq++;
                }

              /* if it does not need or is spilt
                 or is already assigned to registers
                 or will not live beyond this instructions */
              if (!sym->nRegs ||
                  sym->isspilt ||
                  bitVectBitValue (_G.regAssigned, sym->key) ||
                  sym->liveTo <= ic->seq)
                {
                  continue;
                }

              /* if some liverange has been spilt at the block level
                 and this one live beyond this block then spil this
                 to be safe */
              if (_G.blockSpil && sym->liveTo > ebbs[i]->lSeq)
                {
                  spillThis (sym);
                  continue;
                }
              /* if trying to allocate this will cause
                 a spill and there is nothing to spill
                 or this one is rematerializable then
                 spill this one */
              willCS = willCauseSpill (sym->nRegs, sym->regType);
              spillable = computeSpillable (ic);
              if (sym->remat || (willCS && bitVectIsZero (spillable)))
                {
                  spillThis (sym);
                  continue;
                }

              /* If the live range preceeds the point of definition
                 then ideally we must take into account registers that
                 have been allocated after sym->liveFrom but freed
                 before ic->seq. This is complicated, so spill this
                 symbol instead and let fillGaps handle the allocation. */
              if (sym->liveFrom < ic->seq)
                {
                  spillThis (sym);
                  continue;
                }

              /* if it has a spillocation & is used less than
                 all other live ranges then spill this */
              if (willCS)
                {
                  if (sym->usl.spillLoc)
                    {
                      symbol *leastUsed = leastUsedLR (liveRangesWith (spillable,
                                                                       allLRs, ebbs[i], ic));
                      if (leastUsed && leastUsed->used > sym->used)
                        {
                          spillThis (sym);
                          continue;
                        }
                    }
                  else
                    {
                      /* if none of the liveRanges have a spillLocation then better
                         to spill this one than anything else already assigned to registers */
                      if (liveRangesWith(spillable,noSpilLoc,ebbs[i],ic))
                        {
                          /* if this is local to this block then we might find a block spil */
                          if (!(sym->liveFrom >= ebbs[i]->fSeq && sym->liveTo <= ebbs[i]->lSeq))
                            {
                              spillThis (sym);
                              continue;
                            }
                        }
                    }
                }
              /* if we need ptr regs for the right side
                 then mark it */
              if (POINTER_GET (ic) && IS_SYMOP (IC_LEFT (ic))
                  && getSize (OP_SYMBOL (IC_LEFT (ic))->type) <= (unsigned int) PTRSIZE)
                {
                  hc08_ptrRegReq++;
                  ptrRegSet = 1;
                }

              /* else we assign registers to it */
              _G.regAssigned = bitVectSetBit (_G.regAssigned, sym->key);
              _G.totRegAssigned = bitVectSetBit (_G.totRegAssigned, sym->key);

              for (j = 0; j < sym->nRegs; j++)
                {
                  if (sym->regType == REG_PTR)
                      sym->regs[j] = getRegPtr (ic, ebbs[i], sym);
                  else
                      sym->regs[j] = getRegGpr (ic, ebbs[i], sym);

                  /* if the allocation failed which means
                     this was spilt then break */
                  if (!sym->regs[j])
                    {
                      break;
                    }
                }

              /* if it shares registers with operands make sure
                 that they are in the same position */
              if (IC_LEFT (ic) && IS_SYMOP (IC_LEFT (ic)) &&
                  OP_SYMBOL (IC_LEFT (ic))->nRegs && ic->op != '=')
                {
                  positionRegs (OP_SYMBOL (IC_RESULT (ic)),
                                OP_SYMBOL (IC_LEFT (ic)));
                }
              /* do the same for the right operand */
              if (IC_RIGHT (ic) && IS_SYMOP (IC_RIGHT (ic)) &&
                  OP_SYMBOL (IC_RIGHT (ic))->nRegs)
                {
                  positionRegs (OP_SYMBOL (IC_RESULT (ic)),
                                OP_SYMBOL (IC_RIGHT (ic)));
                }

              if (ptrRegSet)
                {
                  hc08_ptrRegReq--;
                  ptrRegSet = 0;
                }
            }
        }
    }

  /* Check for and fix any problems with uninitialized operands */
  for (i = 0; i < count; i++)
    {
      iCode *ic;

      if (ebbs[i]->noPath &&
          (ebbs[i]->entryLabel != entryLabel &&
           ebbs[i]->entryLabel != returnLabel))
        {
          continue;
        }

      for (ic = ebbs[i]->sch; ic; ic = ic->next)
        {
          if (SKIP_IC2 (ic))
            continue;

          if (ic->op == IFX)
            {
              verifyRegsAssigned (IC_COND (ic), ic);
              continue;
            }

          if (ic->op == JUMPTABLE)
            {
              verifyRegsAssigned (IC_JTCOND (ic), ic);
              continue;
            }

          verifyRegsAssigned (IC_RESULT (ic), ic);
          verifyRegsAssigned (IC_LEFT (ic), ic);
          verifyRegsAssigned (IC_RIGHT (ic), ic);
        }
    }
}

/*-----------------------------------------------------------------*/
/* fillGaps - Try to fill in the Gaps left by Pass1                */
/*-----------------------------------------------------------------*/
static void fillGaps()
{
  symbol *sym =NULL;
  int key =0;

  if (getenv("DISABLE_FILL_GAPS"))
    return;

  /* look for liveranges that were spilt by the allocator */
  for (sym = hTabFirstItem(liveRanges, &key) ; sym ;
       sym = hTabNextItem(liveRanges, &key))
    {
      int i;
      int pdone = 0;

      if (!sym->spillA || !sym->clashes || sym->remat)
        continue;

      /* find the liveRanges this one clashes with, that are
         still assigned to registers & mark the registers as used*/
      for ( i = 0 ; i < sym->clashes->size ; i ++)
        {
          int k;
          symbol *clr;

          if (bitVectBitValue(sym->clashes,i) == 0 ||    /* those that clash with this */
              bitVectBitValue(_G.totRegAssigned,i) == 0) /* and are still assigned to registers */
            continue ;

          clr = hTabItemWithKey(liveRanges, i);
          assert(clr);

          /* mark these registers as used */
          for (k = 0 ; k < clr->nRegs ; k++ )
            hc08_useReg(clr->regs[k]);
        }

      if (willCauseSpill(sym->nRegs, sym->regType))
        {
          /* NOPE :( clear all registers & and continue */
          freeAllRegs();
          continue ;
        }

      /* THERE IS HOPE !!!! */
      for (i=0; i < sym->nRegs ; i++ )
        {
          if (sym->regType == REG_PTR)
            sym->regs[i] = getRegPtrNoSpil ();
          else
            sym->regs[i] = getRegGprNoSpil ();
        }

      /* For all its definitions check if the registers
         allocated needs positioning NOTE: we can position
         only ONCE if more than One positioning required
         then give up.
        */
      sym->isspilt = 0;
      for (i = 0 ; i < sym->defs->size ; i++ )
        {
          if (bitVectBitValue(sym->defs,i))
            {
              iCode *ic;
              if (!(ic = hTabItemWithKey(iCodehTab,i)))
                continue;
              if (SKIP_IC(ic))
                continue;
              assert(isSymbolEqual(sym,OP_SYMBOL(IC_RESULT(ic)))); /* just making sure */
              /* if left is assigned to registers */
              if (IS_SYMOP(IC_LEFT(ic)) &&
                  bitVectBitValue(_G.totRegAssigned,OP_SYMBOL(IC_LEFT(ic))->key))
                {
                  pdone += positionRegs(sym,OP_SYMBOL(IC_LEFT(ic)));
                }
              if (IS_SYMOP(IC_RIGHT(ic)) &&
                  bitVectBitValue(_G.totRegAssigned,OP_SYMBOL(IC_RIGHT(ic))->key))
                {
                  pdone += positionRegs(sym,OP_SYMBOL(IC_RIGHT(ic)));
                }
              if (pdone > 1)
                break;
            }
        }
      for (i = 0 ; i < sym->uses->size ; i++ )
        {
          if (bitVectBitValue(sym->uses,i))
            {
              iCode *ic;
              if (!(ic = hTabItemWithKey(iCodehTab,i)))
                continue;
              if (SKIP_IC(ic))
                continue;
              if (!IS_ASSIGN_ICODE(ic))
                continue;

              /* if result is assigned to registers */
              if (IS_SYMOP(IC_RESULT(ic)) &&
                  bitVectBitValue(_G.totRegAssigned,OP_SYMBOL(IC_RESULT(ic))->key))
                {
                  pdone += positionRegs(sym,OP_SYMBOL(IC_RESULT(ic)));
                }
              if (pdone > 1)
                break;
            }
        }
      /* had to position more than once GIVE UP */
      if (pdone > 1)
        {
          /* UNDO all the changes we made to try this */
          sym->isspilt = 1;
          for (i=0; i < sym->nRegs ; i++ )
            {
              sym->regs[i] = NULL;
            }
          freeAllRegs();
          D(printf ("Fill Gap gave up due to positioning for %s in function %s\n",sym->name, currFunc ? currFunc->name : "UNKNOWN"));
          continue ;
        }
      D(printf ("FILLED GAP for %s in function %s\n",sym->name, currFunc ? currFunc->name : "UNKNOWN"));

      _G.totRegAssigned = bitVectSetBit(_G.totRegAssigned,sym->key);
      sym->isspilt = sym->spillA = 0 ;
      sym->usl.spillLoc->allocreq--;
      freeAllRegs();
    }
}

/*-----------------------------------------------------------------*/
/* rUmaskForOp :- returns register mask for an operand             */
/*-----------------------------------------------------------------*/
bitVect *
hc08_rUmaskForOp (operand * op)
{
  bitVect *rumask;
  symbol *sym;
  int j;

  /* only temporaries are assigned registers */
  if (!IS_ITEMP (op))
    return NULL;

  sym = OP_SYMBOL (op);

  /* if spilt or no registers assigned to it
     then nothing */
  if (sym->isspilt || !sym->nRegs)
    return NULL;

  rumask = newBitVect (hc08_nRegs);

  for (j = 0; j < sym->nRegs; j++)
    {
      rumask = bitVectSetBit (rumask, sym->regs[j]->rIdx);
    }

  return rumask;
}

/*-----------------------------------------------------------------*/
/* regsUsedIniCode :- returns bit vector of registers used in iCode */
/*-----------------------------------------------------------------*/
static bitVect *
regsUsedIniCode (iCode * ic)
{
  bitVect *rmask = newBitVect (hc08_nRegs);

  /* do the special cases first */
  if (ic->op == IFX)
    {
      rmask = bitVectUnion (rmask, hc08_rUmaskForOp (IC_COND (ic)));
      goto ret;
    }

  /* for the jumptable */
  if (ic->op == JUMPTABLE)
    {
      rmask = bitVectUnion (rmask, hc08_rUmaskForOp (IC_JTCOND (ic)));
      goto ret;
    }

  /* of all other cases */
  if (IC_LEFT (ic))
    rmask = bitVectUnion (rmask, hc08_rUmaskForOp (IC_LEFT (ic)));

  if (IC_RIGHT (ic))
    rmask = bitVectUnion (rmask, hc08_rUmaskForOp (IC_RIGHT (ic)));

  if (IC_RESULT (ic))
    rmask = bitVectUnion (rmask, hc08_rUmaskForOp (IC_RESULT (ic)));

ret:
  return rmask;
}

/*-----------------------------------------------------------------*/
/* createRegMask - for each instruction will determine the regsUsed */
/*-----------------------------------------------------------------*/
static void
createRegMask (eBBlock ** ebbs, int count)
{
  int i;

  /* for all blocks */
  for (i = 0; i < count; i++)
    {
      iCode *ic;

      if (ebbs[i]->noPath &&
          (ebbs[i]->entryLabel != entryLabel &&
           ebbs[i]->entryLabel != returnLabel))
        continue;

      /* for all instructions */
      for (ic = ebbs[i]->sch; ic; ic = ic->next)
        {
          int j;

          if (SKIP_IC2 (ic) || !ic->rlive)
            continue;

          /* first mark the registers used in this
             instruction */
          ic->rUsed = regsUsedIniCode (ic);
          _G.funcrUsed = bitVectUnion (_G.funcrUsed, ic->rUsed);

          /* now create the register mask for those
             registers that are in use : this is a
             super set of ic->rUsed */
          ic->rMask = newBitVect (hc08_nRegs + 1);

          /* for all live Ranges alive at this point */
          for (j = 1; j < ic->rlive->size; j++)
            {
              symbol *sym;
              int k;

              /* if not alive then continue */
              if (!bitVectBitValue (ic->rlive, j))
                continue;

              /* find the live range we are interested in */
              if (!(sym = hTabItemWithKey (liveRanges, j)))
                {
                  werror (E_INTERNAL_ERROR, __FILE__, __LINE__,
                          "createRegMask cannot find live range");
                  fprintf(stderr, "\tmissing live range: key=%d\n", j);
                  exit (0);
                }

              /* if no register assigned to it */
              if (!sym->nRegs || sym->isspilt)
                continue;

              /* for all the registers allocated to it */
              for (k = 0; k < sym->nRegs; k++)
                if (sym->regs[k])
                  ic->rMask = bitVectSetBit (ic->rMask, sym->regs[k]->rIdx);
            }
        }
    }
}

/*-----------------------------------------------------------------*/
/* rematStr - returns the rematerialized string for a remat var    */
/*-----------------------------------------------------------------*/
static char *
rematStr (symbol * sym)
{
  iCode *ic = sym->rematiCode;
  int offset = 0;

  while (1)
    {
      /* if plus adjust offset to right hand side */
      if (ic->op == '+')
        {
          offset += (int) operandLitValue (IC_RIGHT (ic));
          ic = OP_SYMBOL (IC_LEFT (ic))->rematiCode;
          continue;
        }

      /* if minus adjust offset to right hand side */
      if (ic->op == '-')
        {
          offset -= (int) operandLitValue (IC_RIGHT (ic));
          ic = OP_SYMBOL (IC_LEFT (ic))->rematiCode;
          continue;
        }

      /* cast then continue */
      if (IS_CAST_ICODE(ic))
        {
          ic = OP_SYMBOL (IC_RIGHT (ic))->rematiCode;
          continue;
        }
      /* we reached the end */
      break;
    }

  if (ic->op == ADDRESS_OF)
    {
      if (offset)
        {
          SNPRINTF (buffer, sizeof(buffer),
                    "(%s %c 0x%04x)",
                    OP_SYMBOL (IC_LEFT (ic))->rname,
                    offset >= 0 ? '+' : '-',
                    abs (offset) & 0xffff);
        }
      else
        {
          strncpyz (buffer, OP_SYMBOL (IC_LEFT (ic))->rname, sizeof(buffer));
        }
    }
  else if (ic->op == '=')
    {
      offset += (int) operandLitValue (IC_RIGHT (ic));
      SNPRINTF (buffer, sizeof(buffer),
                "0x%04x",
                offset & 0xffff);
    }
  return buffer;
}

/*-----------------------------------------------------------------*/
/* regTypeNum - computes the type & number of registers required   */
/*-----------------------------------------------------------------*/
static void
regTypeNum (eBBlock *ebbs)
{
  symbol *sym;
  int k;
  iCode *ic;

  /* for each live range do */
  for (sym = hTabFirstItem (liveRanges, &k); sym;
       sym = hTabNextItem (liveRanges, &k))
    {
      /* if used zero times then no registers needed */
      if ((sym->liveTo - sym->liveFrom) == 0)
        continue;

      /* if the live range is a temporary */
      if (sym->isitmp)
        {
          /* if the type is marked as a conditional */
          if (sym->regType == REG_CND)
            continue;

          /* if used in return only then we don't
             need registers */
          if (sym->ruonly || sym->accuse)
            {
              if (IS_AGGREGATE (sym->type) || sym->isptr)
                sym->type = aggrToPtr (sym->type, FALSE);
              continue;
            }

          /* if the symbol has only one definition &
             that definition is a get_pointer */
          if (bitVectnBitsOn (sym->defs) == 1 &&
              (ic = hTabItemWithKey (iCodehTab, bitVectFirstBit (sym->defs))) &&
              POINTER_GET (ic) &&
              !IS_BITVAR (sym->etype) &&
              (aggrToPtrDclType (operandType (IC_LEFT (ic)), FALSE) == POINTER))
            {
              if (ptrPseudoSymSafe (sym, ic))
                {
                  ptrPseudoSymConvert (sym, ic, rematStr (OP_SYMBOL (IC_LEFT (ic))));
                  continue;
                }

              /* if in data space or idata space then try to
                 allocate pointer register */
            }

          /* if not then we require registers */
          sym->nRegs = ((IS_AGGREGATE (sym->type) || sym->isptr) ?
                        getSize (sym->type = aggrToPtr (sym->type, FALSE)) :
                        getSize (sym->type));

          if (sym->nRegs > 4)
            {
              fprintf (stderr, "allocated more than 4 or 0 registers for type ");
              printTypeChain (sym->type, stderr);
              fprintf (stderr, "\n");
            }

          /* determine the type of register required */
          if (sym->nRegs == 1 && IS_PTR (sym->type) && sym->uptr)
            sym->regType = REG_PTR;
          else
            sym->regType = REG_GPR;
        }
      else
        /* for the first run we don't provide */
        /* registers for true symbols we will */
        /* see how things go                  */
        sym->nRegs = 0;
    }
}

/*-----------------------------------------------------------------*/
/* freeAllRegs - mark all registers as free                        */
/*-----------------------------------------------------------------*/
static void
freeAllRegs ()
{
  int i;

  for (i = 0; i < hc08_nRegs; i++)
    {
      regshc08[i].isFree = 1;
      regshc08[i].aop = NULL;
    }
}

/*-----------------------------------------------------------------*/
/* deallocStackSpil - this will set the stack pointer back         */
/*-----------------------------------------------------------------*/
static
DEFSETFUNC (deallocStackSpil)
{
  symbol *sym = item;

  deallocLocal (sym);
  return 0;
}

#if 0
/*-----------------------------------------------------------------*/
/* farSpacePackable - returns the packable icode for far variables */
/*-----------------------------------------------------------------*/
static iCode *
farSpacePackable (iCode * ic)
{
  iCode *dic;

  /* go thru till we find a definition for the
     symbol on the right */
  for (dic = ic->prev; dic; dic = dic->prev)
    {
      /* if the definition is a call then no */
      if ((dic->op == CALL || dic->op == PCALL) &&
          IC_RESULT (dic)->key == IC_RIGHT (ic)->key)
        {
          return NULL;
        }

      /* if shift by unknown amount then not */
      if ((dic->op == LEFT_OP || dic->op == RIGHT_OP) &&
          IC_RESULT (dic)->key == IC_RIGHT (ic)->key)
        return NULL;

#if 0
      /* if pointer get and size > 1 */
      if (POINTER_GET (dic) &&
          getSize (aggrToPtr (operandType (IC_LEFT (dic)), FALSE)) > 1)
        return NULL;

      if (POINTER_SET (dic) &&
          getSize (aggrToPtr (operandType (IC_RESULT (dic)), FALSE)) > 1)
        return NULL;
#endif

      /* if any three is a true symbol in far space */
      if (IC_RESULT (dic) &&
          IS_TRUE_SYMOP (IC_RESULT (dic)) /* &&
          isOperandInFarSpace (IC_RESULT (dic)) */)
        return NULL;

      if (IC_RIGHT (dic) &&
          IS_TRUE_SYMOP (IC_RIGHT (dic)) /* &&
          isOperandInFarSpace (IC_RIGHT (dic)) */ &&
          !isOperandEqual (IC_RIGHT (dic), IC_RESULT (ic)))
        return NULL;

      if (IC_LEFT (dic) &&
          IS_TRUE_SYMOP (IC_LEFT (dic)) /* &&
          isOperandInFarSpace (IC_LEFT (dic)) */ &&
          !isOperandEqual (IC_LEFT (dic), IC_RESULT (ic)))
        return NULL;

      if (isOperandEqual (IC_RIGHT (ic), IC_RESULT (dic)))
        {
          if ((dic->op == LEFT_OP ||
               dic->op == RIGHT_OP ||
               dic->op == '-') &&
              IS_OP_LITERAL (IC_RIGHT (dic)))
            return NULL;
          else
            return dic;
        }
    }

  return NULL;
}
#endif

#if 0
static void
packRegsForLiteral (iCode * ic)
{
  int k;
  iCode *uic;

  if (ic->op != '=')
    return;
  if (POINTER_SET (ic))
    return;
  if (!IS_LITERAL (getSpec (operandType (IC_RIGHT (ic)))))
    return;
  if (bitVectnBitsOn (OP_DEFS (IC_RESULT (ic))) > 1)
    return;

  for (k=0; k< OP_USES (IC_RESULT (ic))->size; k++)
    if (bitVectBitValue (OP_USES (IC_RESULT (ic)), k))
      {
        uic = hTabItemWithKey (iCodehTab, k);
        if (!uic) continue;

        if (uic->op != IFX && uic->op != JUMPTABLE)
          {
            if (IC_LEFT (uic) && IC_LEFT (uic)->key == IC_RESULT (ic)->key)
              ReplaceOpWithCheaperOp(&IC_LEFT(uic), IC_RIGHT(ic));
            if (IC_RIGHT (uic) && IC_RIGHT (uic)->key == IC_RESULT (ic)->key)
              ReplaceOpWithCheaperOp(&IC_RIGHT(uic), IC_RIGHT(ic));
            if (IC_RESULT (uic) && IC_RESULT (uic)->key == IC_RESULT (ic)->key)
              ReplaceOpWithCheaperOp(&IC_RESULT(uic), IC_RIGHT(ic));
          }
      }

}
#endif


/*-----------------------------------------------------------------*/
/* packRegsForAssign - register reduction for assignment           */
/*-----------------------------------------------------------------*/
static int
packRegsForAssign (iCode * ic, eBBlock * ebp)
{
  iCode *dic, *sic;

  if (!IS_ITEMP (IC_RIGHT (ic)) ||
      OP_SYMBOL (IC_RIGHT (ic))->isind ||
      OP_LIVETO (IC_RIGHT (ic)) > ic->seq)
    {
      return 0;
    }

  /* if the true symbol is defined in far space or on stack
     then we should not since this will increase register pressure */
#if 0
  if (isOperandInFarSpace(IC_RESULT(ic)) && !farSpacePackable(ic))
    {
      return 0;
    }
#endif

  /* find the definition of iTempNN scanning backwards if we find
     a use of the true symbol before we find the definition then
     we cannot */
  for (dic = ic->prev; dic; dic = dic->prev)
    {
      int crossedCall = 0;

      /* We can pack across a function call only if it's a local */
      /* variable or our parameter. Never pack global variables */
      /* or parameters to a function we call. */
      if ((dic->op == CALL || dic->op == PCALL))
        {
          if (!OP_SYMBOL (IC_RESULT (ic))->ismyparm
              && !OP_SYMBOL (IC_RESULT (ic))->islocal)
            {
              crossedCall = 1;
            }
        }

      /* Don't move an assignment out of a critical block */
      if (dic->op == CRITICAL)
        {
          dic = NULL;
          break;
        }

      if (SKIP_IC2 (dic))
        continue;

      if (dic->op == IFX)
        {
          if (IS_SYMOP (IC_COND (dic)) &&
              (IC_COND (dic)->key == IC_RESULT (ic)->key ||
               IC_COND (dic)->key == IC_RIGHT (ic)->key))
            {
              dic = NULL;
              break;
            }
        }
      else
        {
          if (IS_TRUE_SYMOP (IC_RESULT (dic)) &&
              IS_OP_VOLATILE (IC_RESULT (dic)))
            {
              dic = NULL;
              break;
            }

          if (IS_SYMOP (IC_RESULT (dic)) &&
              IC_RESULT (dic)->key == IC_RIGHT (ic)->key)
            {
              if (POINTER_SET (dic))
                dic = NULL;
              break;
            }

          if (IS_SYMOP (IC_RIGHT (dic)) &&
              (IC_RIGHT (dic)->key == IC_RESULT (ic)->key ||
               IC_RIGHT (dic)->key == IC_RIGHT (ic)->key))
            {
              dic = NULL;
              break;
            }

          if (IS_SYMOP (IC_LEFT (dic)) &&
              (IC_LEFT (dic)->key == IC_RESULT (ic)->key ||
               IC_LEFT (dic)->key == IC_RIGHT (ic)->key))
            {
              dic = NULL;
              break;
            }

          if (POINTER_SET (dic) &&
              IC_RESULT (dic)->key == IC_RESULT (ic)->key)
            {
              dic = NULL;
              break;
            }

          if (crossedCall)
            {
              dic = NULL;
              break;
            }
        }
    }

  if (!dic)
    return 0;                   /* did not find */

  /* if assignment then check that right is not a bit */
  if (ASSIGNMENT (ic) && !POINTER_SET (ic))
    {
      sym_link *etype = operandType (IC_RESULT (dic));
      if (IS_BITFIELD (etype))
        {
          /* if result is a bit too then it's ok */
          etype = operandType (IC_RESULT (ic));
          if (!IS_BITFIELD (etype))
            {
              return 0;
            }
        }
    }

  /* if the result is on stack or iaccess then it must be
     the same atleast one of the operands */
  if (OP_SYMBOL (IC_RESULT (ic))->onStack ||
      OP_SYMBOL (IC_RESULT (ic))->iaccess)
    {
      /* the operation has only one symbol
         operator then we can pack */
      if ((IC_LEFT (dic) && !IS_SYMOP (IC_LEFT (dic))) ||
          (IC_RIGHT (dic) && !IS_SYMOP (IC_RIGHT (dic))))
        goto pack;

      if (!((IC_LEFT (dic) &&
             IC_RESULT (ic)->key == IC_LEFT (dic)->key) ||
            (IC_RIGHT (dic) &&
             IC_RESULT (ic)->key == IC_RIGHT (dic)->key)))
        return 0;
    }
pack:
  /* found the definition */
  /* replace the result with the result of */
  /* this assignment and remove this assignment */
  bitVectUnSetBit (OP_SYMBOL (IC_RESULT (dic))->defs, dic->key);
  ReplaceOpWithCheaperOp (&IC_RESULT (dic), IC_RESULT (ic));

  if (IS_ITEMP (IC_RESULT (dic)) && OP_SYMBOL (IC_RESULT (dic))->liveFrom > dic->seq)
    {
      OP_SYMBOL (IC_RESULT (dic))->liveFrom = dic->seq;
    }
  // TODO: and the otherway around?

  /* delete from liverange table also
     delete from all the points inbetween and the new
     one */
  for (sic = dic; sic != ic; sic = sic->next)
    {
      bitVectUnSetBit (sic->rlive, IC_RESULT (ic)->key);
      if (IS_ITEMP (IC_RESULT (dic)))
        bitVectSetBit (sic->rlive, IC_RESULT (dic)->key);
    }

  remiCodeFromeBBlock (ebp, ic);
  bitVectUnSetBit (OP_DEFS (IC_RESULT (ic)), ic->key);
  hTabDeleteItem (&iCodehTab, ic->key, ic, DELETE_ITEM, NULL);
  OP_DEFS (IC_RESULT (dic)) = bitVectSetBit (OP_DEFS (IC_RESULT (dic)), dic->key);
  return 1;
}

/*------------------------------------------------------------------*/
/* findAssignToSym : scanning backwards looks for first assig found */
/*------------------------------------------------------------------*/
static iCode *
findAssignToSym (operand * op, iCode * ic)
{
  iCode *dic;

  /* This routine is used to find sequences like
     iTempAA = FOO;
     ...;  (intervening ops don't use iTempAA or modify FOO)
     blah = blah + iTempAA;

     and eliminate the use of iTempAA, freeing up its register for
     other uses.
  */

  for (dic = ic->prev; dic; dic = dic->prev)
    {
      /* if definition by assignment */
      if (dic->op == '=' &&
          !POINTER_SET (dic) &&
          IC_RESULT (dic)->key == op->key
          &&  IS_TRUE_SYMOP(IC_RIGHT(dic))
        )
        break;  /* found where this temp was defined */

      /* if we find an usage then we cannot delete it */
      if (IC_LEFT (dic) && IC_LEFT (dic)->key == op->key)
        return NULL;

      if (IC_RIGHT (dic) && IC_RIGHT (dic)->key == op->key)
        return NULL;

      if (POINTER_SET (dic) && IC_RESULT (dic)->key == op->key)
        return NULL;
    }

  if (!dic)
    return NULL;   /* didn't find any assignment to op */

  /* we are interested only if defined in far space */
  /* or in stack space in case of + & - */

  /* if assigned to a non-symbol then don't repack regs */
  if (!IS_SYMOP (IC_RIGHT (dic)))
    return NULL;

  /* if the symbol is volatile then we should not */
  if (isOperandVolatile (IC_RIGHT (dic), TRUE))
    return NULL;
  /* XXX TODO --- should we be passing FALSE to isOperandVolatile()?
     What does it mean for an iTemp to be volatile, anyway? Passing
     TRUE is more cautious but may prevent possible optimizations */

  /* if the symbol is in far space then we should not */
  /* if (isOperandInFarSpace (IC_RIGHT (dic)))
    return NULL; */

  /* for + & - operations make sure that
     if it is on the stack it is the same
     as one of the three operands */
#if 0
  if ((ic->op == '+' || ic->op == '-') &&
      OP_SYMBOL (IC_RIGHT (dic))->onStack)
    {
      if (IC_RESULT (ic)->key != IC_RIGHT (dic)->key &&
          IC_LEFT (ic)->key != IC_RIGHT (dic)->key &&
          IC_RIGHT (ic)->key != IC_RIGHT (dic)->key)
        return NULL;
    }
#endif

  /* now make sure that the right side of dic
     is not defined between ic & dic */
  if (dic)
    {
      iCode *sic = dic->next;

      for (; sic != ic; sic = sic->next)
        if (IC_RESULT (sic) &&
            IC_RESULT (sic)->key == IC_RIGHT (dic)->key)
          return NULL;
    }

  return dic;
}

/*-----------------------------------------------------------------*/
/* reassignAliasedSym - used by packRegsForSupport to replace      */
/*                      redundant iTemp with equivalent symbol     */
/*-----------------------------------------------------------------*/
static void
reassignAliasedSym (eBBlock *ebp, iCode *assignment, iCode *use, operand *op)
{
  iCode *ic;
  unsigned oldSymKey, newSymKey;

  oldSymKey = op->key;
  newSymKey = IC_RIGHT(assignment)->key;

  /* only track live ranges of compiler-generated temporaries */
  if (!IS_ITEMP(IC_RIGHT(assignment)))
    newSymKey = 0;

  /* update the live-value bitmaps */
  for (ic = assignment; ic != use; ic = ic->next) {
    bitVectUnSetBit (ic->rlive, oldSymKey);
    if (newSymKey != 0)
      ic->rlive = bitVectSetBit (ic->rlive, newSymKey);
  }

  /* update the sym of the used operand */
  OP_SYMBOL(op) = OP_SYMBOL(IC_RIGHT(assignment));
  op->key = OP_SYMBOL(op)->key;

  /* update the sym's liverange */
  if ( OP_LIVETO(op) < ic->seq )
    setToRange(op, ic->seq, FALSE);

  /* remove the assignment iCode now that its result is unused */
  remiCodeFromeBBlock (ebp, assignment);
  bitVectUnSetBit(OP_SYMBOL(IC_RESULT(assignment))->defs, assignment->key);
  hTabDeleteItem (&iCodehTab, assignment->key, assignment, DELETE_ITEM, NULL);
}


/*-----------------------------------------------------------------*/
/* packRegsForSupport :- reduce some registers for support calls   */
/*-----------------------------------------------------------------*/
static int
packRegsForSupport (iCode * ic, eBBlock * ebp)
{
  iCode *dic;
  int changes = 0;

  /* for the left & right operand :- look to see if the
     left was assigned a true symbol in far space in that
     case replace them */

  if (IS_ITEMP (IC_LEFT (ic)) &&
      OP_SYMBOL (IC_LEFT (ic))->liveTo <= ic->seq)
    {
      dic = findAssignToSym (IC_LEFT (ic), ic);

      if (dic)
        {
          /* found it we need to remove it from the block */
          reassignAliasedSym (ebp, dic, ic, IC_LEFT(ic));
          changes++;
        }
    }

  /* do the same for the right operand */
  if (IS_ITEMP (IC_RIGHT (ic)) &&
      OP_SYMBOL (IC_RIGHT (ic))->liveTo <= ic->seq)
    {
      iCode *dic = findAssignToSym (IC_RIGHT (ic), ic);

      if (dic)
        {
          /* found it we need to remove it from the block */
          reassignAliasedSym (ebp, dic, ic, IC_RIGHT(ic));
          changes++;
        }
    }

  return changes;
}


#if 0
/*-----------------------------------------------------------------*/
/* packRegsForOneuse : - will reduce some registers for single Use */
/*-----------------------------------------------------------------*/
static iCode *
packRegsForOneuse (iCode * ic, operand * op, eBBlock * ebp)
{
  bitVect *uses;
  iCode *dic, *sic;

  /* if returning a literal then do nothing */
  if (!IS_SYMOP (op))
    return NULL;

  /* only up to 2 bytes */
  if (getSize (operandType (op)) > (fReturnSizeHC08 - 2))
    return NULL;

  return NULL;

  if (ic->op != SEND && //RETURN
      ic->op != SEND &&
      !POINTER_SET (ic) &&
      !POINTER_GET (ic))
    return NULL;

  if (ic->op == SEND && ic->argreg != 1)
    return NULL;

  /* this routine will mark the symbol as used in one
     instruction use only && if the definition is local
     (ie. within the basic block) && has only one definition &&
     that definition is either a return value from a
     function or does not contain any variables in
     far space */
  uses = bitVectCopy (OP_USES (op));
  bitVectUnSetBit (uses, ic->key);      /* take away this iCode */
  if (!bitVectIsZero (uses))    /* has other uses */
    return NULL;

  /* if it has only one definition */
  if (bitVectnBitsOn (OP_DEFS (op)) > 1)
    return NULL;                /* has more than one definition */

  /* get that definition */
  if (!(dic = hTabItemWithKey (iCodehTab, bitVectFirstBit (OP_DEFS (op)))))
    return NULL;

  /* if that only usage is a cast */
  if (dic->op == CAST)
    {
      /* to a bigger type */
      if (getSize(OP_SYM_TYPE(IC_RESULT(dic))) > getSize(OP_SYM_TYPE(IC_RIGHT(dic))))
        {
          /* then we can not, since we cannot predict the usage of b & acc */
          return NULL;
        }
    }

  /* found the definition now check if it is local */
  if (dic->seq < ebp->fSeq || dic->seq > ebp->lSeq)
    return NULL;                /* non-local */

  /* now check if it is the return from a function call */
  if (dic->op == CALL || dic->op == PCALL)
    {
      if (ic->op != SEND && ic->op != RETURN &&
          !POINTER_SET(ic) && !POINTER_GET(ic))
        {
          OP_SYMBOL (op)->ruonly = 1;
          return dic;
        }
      dic = dic->next;
    }


//  /* otherwise check that the definition does
//     not contain any symbols in far space */
//  if (isOperandInFarSpace (IC_LEFT (dic)) ||
//      isOperandInFarSpace (IC_RIGHT (dic)) ||
//      IS_OP_RUONLY (IC_LEFT (ic)) ||
//      IS_OP_RUONLY (IC_RIGHT (ic)))
//    {
//      return NULL;
//    }

  /* if pointer set then make sure the pointer
     is one byte */
#if 0
  if (POINTER_SET (dic) &&
      !IS_DATA_PTR (aggrToPtr (operandType (IC_RESULT (dic)), FALSE)))
    return NULL;

  if (POINTER_GET (dic) &&
      !IS_DATA_PTR (aggrToPtr (operandType (IC_LEFT (dic)), FALSE)))
    return NULL;
#endif

  sic = dic;

  /* make sure the intervening instructions
     don't have anything in far space */
  for (dic = dic->next; dic && dic != ic && sic != ic; dic = dic->next)
    {
      /* if there is an intervening function call then no */
      if (dic->op == CALL || dic->op == PCALL)
        return NULL;
      /* if pointer set then make sure the pointer
         is one byte */
#if 0
      if (POINTER_SET (dic) &&
          !IS_DATA_PTR (aggrToPtr (operandType (IC_RESULT (dic)), FALSE)))
        return NULL;

      if (POINTER_GET (dic) &&
          !IS_DATA_PTR (aggrToPtr (operandType (IC_LEFT (dic)), FALSE)))
        return NULL;
#endif
      /* if address of & the result is remat then okay */
      if (dic->op == ADDRESS_OF &&
          OP_SYMBOL (IC_RESULT (dic))->remat)
        continue;

      /* if operand has size of three or more & this
         operation is a '*','/' or '%' then 'b' may
         cause a problem */
#if 0
      if ((dic->op == '%' || dic->op == '/' || dic->op == '*') &&
          getSize (operandType (op)) >= 3)
        return NULL;
#endif

      /* if left or right or result is in far space */
//      if (isOperandInFarSpace (IC_LEFT (dic)) ||
//        isOperandInFarSpace (IC_RIGHT (dic)) ||
//        isOperandInFarSpace (IC_RESULT (dic)) ||
//        IS_OP_RUONLY (IC_LEFT (dic)) ||
//        IS_OP_RUONLY (IC_RIGHT (dic)) ||
//        IS_OP_RUONLY (IC_RESULT (dic)))
//      {
//        return NULL;
//      }
//      /* if left or right or result is on stack */
//     if (isOperandOnStack(IC_LEFT(dic)) ||
//        isOperandOnStack(IC_RIGHT(dic)) ||
//        isOperandOnStack(IC_RESULT(dic))) {
//      return NULL;
//     }
    }

  OP_SYMBOL (op)->ruonly = 1;
  return sic;
}
#endif

/*-----------------------------------------------------------------*/
/* isBitwiseOptimizable - requirements of JEAN LOUIS VERN          */
/*-----------------------------------------------------------------*/
static bool
isBitwiseOptimizable (iCode * ic)
{
  sym_link *ltype = getSpec (operandType (IC_LEFT (ic)));
  sym_link *rtype = getSpec (operandType (IC_RIGHT (ic)));

  /* bitwise operations are considered optimizable
     under the following conditions (Jean-Louis VERN)

     x & lit
     bit & bit
     bit & x
     bit ^ bit
     bit ^ x
     x   ^ lit
     x   | lit
     bit | bit
     bit | x
  */
  if (IS_LITERAL(rtype) ||
      (IS_BITVAR (ltype) && IN_BITSPACE (SPEC_OCLS (ltype))))
    return TRUE;
  else
    return FALSE;
}

/*-----------------------------------------------------------------*/
/* isCommutativeOp - tests whether this op cares what order its    */
/*                   operands are in                               */
/*-----------------------------------------------------------------*/
bool isCommutativeOp2(unsigned int op)
{
  if (op == '+' || op == '*' || op == EQ_OP ||
      op == '^' || op == '|' || op == BITWISEAND)
    return TRUE;
  else
    return FALSE;
}

/*-----------------------------------------------------------------*/
/* operandUsesAcc2 - determines whether the code generated for this */
/*                  operand will have to use the accumulator       */
/*-----------------------------------------------------------------*/
bool operandUsesAcc2(operand *op)
{
  if (!op)
    return FALSE;

  if (IS_SYMOP(op)) {
    symbol *sym = OP_SYMBOL(op);
    memmap *symspace;

    if (sym->accuse)
      return TRUE;  /* duh! */

//    if (IN_STACK(sym->etype) || sym->onStack ||
//      (SPIL_LOC(op) && SPIL_LOC(op)->onStack))
//      return TRUE;  /* acc is used to calc stack offset */

    if (IS_ITEMP(op))
      {
        if (SPIL_LOC(op)) {
          sym = SPIL_LOC(op);  /* if spilled, look at spill location */
        } else {
          return FALSE;  /* more checks? */
        }
      }

    symspace = SPEC_OCLS(sym->etype);

//    if (sym->iaccess && symspace->paged)
//      return TRUE;  /* must fetch paged indirect sym via accumulator */

    if (IN_BITSPACE(symspace))
      return TRUE;  /* fetching bit vars uses the accumulator */

    if (IN_FARSPACE(symspace) || IN_CODESPACE(symspace))
      return TRUE;  /* fetched via accumulator and dptr */
  }

  return FALSE;
}

/*-----------------------------------------------------------------*/
/* canDefAccResult - return 1 if the iCode can generate a result   */
/*                   in A or XA                                    */
/*-----------------------------------------------------------------*/
static int
canDefAccResult (iCode * ic)
{
  int size;

  if (ic->op == IFX || ic->op == JUMPTABLE)     /* these iCodes have no result */
    return 0;

  if (POINTER_SET (ic))
    return 0;

  if (!IC_RESULT (ic))
    return 0;

  if (!IS_ITEMP (IC_RESULT (ic)))
    return 0;

  /* I don't think an iTemp can be an aggregate, but just in case */
  if (IS_AGGREGATE(operandType(IC_RESULT(ic))))
    return 0;

  size = getSize (operandType (IC_RESULT (ic)));

  if (size == 1)
    {
      /* All 1 byte operations should safely generate an accumulator result */
      return 1;
    }
  else if (size == 2)
    {
      switch (ic->op)
        {
        case LEFT_OP:
        case RIGHT_OP:
          return isOperandLiteral (IC_RIGHT (ic))
                  && SPEC_USIGN (operandType (IC_RESULT (ic)));
        case CALL:
        case PCALL:
        case '*':
        case RECEIVE:
        case '=': /* assignment, since POINTER_SET is already ruled out */
          return 1;

        default:
          return 0;
        }
    }

  return 0;
}

/*-----------------------------------------------------------------*/
/* canUseAccOperand - return 1 if the iCode can use the operand    */
/*                    when passed in A or XA                       */
/*-----------------------------------------------------------------*/
static int
canUseAccOperand (iCode * ic, operand * op)
{
  int size;
  operand * otherOp;

  if (ic->op == IFX)
    {
      if (isOperandEqual (op, IC_COND (ic)))
        return 1;
      else
        return 0;
    }

  if (ic->op == JUMPTABLE)
    {
      if (isOperandEqual (op, IC_JTCOND (ic)))
        return 1;
      else
        return 0;
    }

  if (POINTER_SET (ic) && isOperandEqual (op, IC_RESULT (ic)))
    return 1;

  if (isOperandEqual (op, IC_LEFT (ic)))
    otherOp = IC_RIGHT (ic);
  else if (isOperandEqual (op, IC_RIGHT (ic)))
    otherOp = IC_LEFT (ic);
  else
    return 0;

  /* Generation of SEND is deferred until CALL; not safe */
  /* if there are intermediate iCodes */
  if (ic->op == SEND && ic->next && ic->next->op != CALL)
    return 0;

  size = getSize (operandType (op));
  if (size == 1)
    {
      /* All 1 byte operations should safely use an accumulator operand */
      return 1;
    }
  else if (size == 2)
    {
      switch (ic->op)
        {
        case LEFT_OP:
        case RIGHT_OP:
          return isOperandLiteral (IC_RIGHT (ic));
        case SEND:
          return 1;
        default:
          return 0;
        }
    }

  return 0;
}


/*-----------------------------------------------------------------*/
/* packRegsForAccUse - pack registers for acc use                  */
/*-----------------------------------------------------------------*/
static int
packRegsForAccUse (iCode * ic)
{
  iCode * uic;
  operand * op;

  if (!canDefAccResult (ic))
    return 0;

  op = IC_RESULT (ic);

  /* has only one definition */
  if (bitVectnBitsOn (OP_DEFS (op)) > 1)
    return 0;

  /* has only one use */
  if (bitVectnBitsOn (OP_USES (op)) > 1)
    return 0;

  uic = ic->next;
  if (!uic)
    return 0;

  if (!canUseAccOperand (uic, op))
    return 0;

  #if 0
  if ((POINTER_GET(uic))
      || (ic->op == ADDRESS_OF && uic->op == '+' && IS_OP_LITERAL (IC_RIGHT (uic))))
    {
      OP_SYMBOL (IC_RESULT (ic))->accuse = ACCUSE_HX;
      return;
    }
  #endif

  OP_SYMBOL (IC_RESULT (ic))->accuse = ACCUSE_XA;
  return 1;
}

/*-----------------------------------------------------------------*/
/* packForPush - heuristics to reduce iCode for pushing            */
/*-----------------------------------------------------------------*/
static void
packForPush (iCode * ic, eBBlock ** ebpp, int blockno)
{
  iCode *dic, *lic;
  bitVect *dbv;
  struct eBBlock * ebp=ebpp[blockno];

  if (ic->op != IPUSH || !IS_ITEMP (IC_LEFT (ic)))
    return;

  /* must have only definition & one usage */
  if (bitVectnBitsOn (OP_DEFS (IC_LEFT (ic))) != 1 ||
      bitVectnBitsOn (OP_USES (IC_LEFT (ic))) != 1)
    return;

  /* find the definition */
  if (!(dic = hTabItemWithKey (iCodehTab,
                               bitVectFirstBit (OP_DEFS (IC_LEFT (ic))))))
    return;

  if (dic->op != '=' || POINTER_SET (dic))
    return;

  if (dic->seq < ebp->fSeq) // Evelyn did this
    {
      int i;
      for (i=0; i<blockno; i++)
        {
          if (dic->seq >= ebpp[i]->fSeq && dic->seq <= ebpp[i]->lSeq)
            {
              ebp=ebpp[i];
              break;
            }
        }
      wassert (i!=blockno); // no way to recover from here
    }

  if (IS_SYMOP(IC_RIGHT(dic)))
    {
      /* make sure the right side does not have any definitions
         inbetween */
      dbv = OP_DEFS(IC_RIGHT(dic));
      for (lic = ic; lic && lic != dic ; lic = lic->prev)
        {
          if (bitVectBitValue(dbv,lic->key))
            return ;
        }
      /* make sure they have the same type */
      if (IS_SPEC(operandType(IC_LEFT(ic))))
        {
          sym_link *itype=operandType(IC_LEFT(ic));
          sym_link *ditype=operandType(IC_RIGHT(dic));

          if (SPEC_USIGN(itype)!=SPEC_USIGN(ditype) ||
              SPEC_LONG(itype)!=SPEC_LONG(ditype))
            return;
        }
      /* extend the live range of replaced operand if needed */
      if (OP_SYMBOL(IC_RIGHT(dic))->liveTo < ic->seq)
        {
          OP_SYMBOL(IC_RIGHT(dic))->liveTo = ic->seq;
        }
      bitVectUnSetBit(OP_SYMBOL(IC_RESULT(dic))->defs,dic->key);
    }

  /* we now we know that it has one & only one def & use
     and the that the definition is an assignment */
  ReplaceOpWithCheaperOp(&IC_LEFT (ic), IC_RIGHT (dic));
  remiCodeFromeBBlock (ebp, dic);
  hTabDeleteItem (&iCodehTab, dic->key, dic, DELETE_ITEM, NULL);
}

/*-----------------------------------------------------------------*/
/* packRegisters - does some transformations to reduce register    */
/*                   pressure                                      */
/*-----------------------------------------------------------------*/
static void
packRegisters (eBBlock ** ebpp, int blockno)
{
  iCode *ic;
  int change = 0;
  eBBlock *ebp = ebpp[blockno];

  do
    {
      change = 0;

      /* look for assignments of the form */
      /* iTempNN = TrueSym (someoperation) SomeOperand */
      /*       ....                       */
      /* TrueSym := iTempNN:1             */
      for (ic = ebp->sch; ic; ic = ic->next)
        {
          /* find assignment of the form TrueSym := iTempNN:1 */
          if (ic->op == '=' && !POINTER_SET (ic))
            change += packRegsForAssign (ic, ebp);
        }
    }
  while (change);

  for (ic = ebp->sch; ic; ic = ic->next)
    {
      //packRegsForLiteral (ic);

      /* if this is an itemp & result of an address of a true sym
         then mark this as rematerialisable   */
      if (ic->op == ADDRESS_OF &&
          IS_ITEMP (IC_RESULT (ic)) &&
          IS_TRUE_SYMOP (IC_LEFT (ic)) &&
          bitVectnBitsOn (OP_DEFS (IC_RESULT (ic))) == 1 &&
          !OP_SYMBOL (IC_LEFT (ic))->onStack)
        {
          OP_SYMBOL (IC_RESULT (ic))->remat = 1;
          OP_SYMBOL (IC_RESULT (ic))->rematiCode = ic;
          OP_SYMBOL (IC_RESULT (ic))->usl.spillLoc = NULL;
        }
#if 1
      if (ic->op == '=' &&
          !POINTER_SET (ic) &&
          IS_ITEMP (IC_RESULT (ic)) &&
          IS_VALOP (IC_RIGHT (ic)) &&
          bitVectnBitsOn (OP_DEFS (IC_RESULT (ic))) == 1)
        {
          OP_SYMBOL (IC_RESULT (ic))->remat = 1;
          OP_SYMBOL (IC_RESULT (ic))->rematiCode = ic;
          OP_SYMBOL (IC_RESULT (ic))->usl.spillLoc = NULL;
        }
#endif
      /* if straight assignment then carry remat flag if
         this is the only definition */
      if (ic->op == '=' &&
          !POINTER_SET (ic) &&
          IS_SYMOP (IC_RIGHT (ic)) &&
          OP_SYMBOL (IC_RIGHT (ic))->remat &&
          !IS_CAST_ICODE(OP_SYMBOL (IC_RIGHT (ic))->rematiCode) &&
          bitVectnBitsOn (OP_SYMBOL (IC_RESULT (ic))->defs) <= 1)
        {
          OP_SYMBOL (IC_RESULT (ic))->remat = OP_SYMBOL (IC_RIGHT (ic))->remat;
          OP_SYMBOL (IC_RESULT (ic))->rematiCode = OP_SYMBOL (IC_RIGHT (ic))->rematiCode;
        }

      /* if cast to a generic pointer & the pointer being
         cast is remat, then we can remat this cast as well */
      if (ic->op == CAST &&
          IS_SYMOP(IC_RIGHT(ic)) &&
          OP_SYMBOL(IC_RIGHT(ic))->remat &&
          bitVectnBitsOn (OP_DEFS (IC_RESULT (ic))) == 1)
        {
          sym_link *to_type = operandType(IC_LEFT(ic));
          sym_link *from_type = operandType(IC_RIGHT(ic));
          if (IS_GENPTR(to_type) && IS_PTR(from_type))
            {
              OP_SYMBOL (IC_RESULT (ic))->remat = 1;
              OP_SYMBOL (IC_RESULT (ic))->rematiCode = ic;
              OP_SYMBOL (IC_RESULT (ic))->usl.spillLoc = NULL;
            }
        }

      /* if this is a +/- operation with a rematerizable
         then mark this as rematerializable as well */
      if ((ic->op == '+' || ic->op == '-') &&
          (IS_SYMOP (IC_LEFT (ic)) &&
           IS_ITEMP (IC_RESULT (ic)) &&
           IS_OP_LITERAL (IC_RIGHT (ic))) &&
           OP_SYMBOL (IC_LEFT (ic))->remat &&
          (!IS_SYMOP (IC_RIGHT (ic)) || !IS_CAST_ICODE(OP_SYMBOL (IC_RIGHT (ic))->rematiCode)) &&
           bitVectnBitsOn (OP_DEFS (IC_RESULT (ic))) == 1)
        {
          OP_SYMBOL (IC_RESULT (ic))->remat = 1;
          OP_SYMBOL (IC_RESULT (ic))->rematiCode = ic;
          OP_SYMBOL (IC_RESULT (ic))->usl.spillLoc = NULL;
        }

      /* mark the pointer usages */
      if (POINTER_SET (ic) && IS_SYMOP (IC_RESULT (ic)))
        OP_SYMBOL (IC_RESULT (ic))->uptr = 1;

      if (POINTER_GET (ic) && IS_SYMOP (IC_LEFT (ic)))
        OP_SYMBOL (IC_LEFT (ic))->uptr = 1;

      if (!SKIP_IC2 (ic))
        {
#if 0
          /* if we are using a symbol on the stack
             then we should say hc08_ptrRegReq */
          if (ic->op == IFX && IS_SYMOP (IC_COND (ic)))
            hc08_ptrRegReq += ((OP_SYMBOL (IC_COND (ic))->onStack ||
                                 OP_SYMBOL (IC_COND (ic))->iaccess) ? 1 : 0);
          else if (ic->op == JUMPTABLE && IS_SYMOP (IC_JTCOND (ic)))
            hc08_ptrRegReq += ((OP_SYMBOL (IC_JTCOND (ic))->onStack ||
                              OP_SYMBOL (IC_JTCOND (ic))->iaccess) ? 1 : 0);
          else
            {
              if (IS_SYMOP (IC_LEFT (ic)))
                hc08_ptrRegReq += ((OP_SYMBOL (IC_LEFT (ic))->onStack ||
                                OP_SYMBOL (IC_LEFT (ic))->iaccess) ? 1 : 0);
              if (IS_SYMOP (IC_RIGHT (ic)))
                hc08_ptrRegReq += ((OP_SYMBOL (IC_RIGHT (ic))->onStack ||
                               OP_SYMBOL (IC_RIGHT (ic))->iaccess) ? 1 : 0);
              if (IS_SYMOP (IC_RESULT (ic)))
                hc08_ptrRegReq += ((OP_SYMBOL (IC_RESULT (ic))->onStack ||
                              OP_SYMBOL (IC_RESULT (ic))->iaccess) ? 1 : 0);
            }
#endif
        }

      /* if the condition of an if instruction
         is defined in the previous instruction and
         this is the only usage then
         mark the itemp as a conditional */
      if ((IS_CONDITIONAL (ic) ||
           (IS_BITWISE_OP(ic) && isBitwiseOptimizable (ic))) &&
          ic->next && ic->next->op == IFX &&
          bitVectnBitsOn (OP_USES(IC_RESULT(ic)))==1 &&
          isOperandEqual (IC_RESULT (ic), IC_COND (ic->next)) &&
          OP_SYMBOL (IC_RESULT (ic))->liveTo <= ic->next->seq)
        {
          OP_SYMBOL (IC_RESULT (ic))->regType = REG_CND;
          continue;
        }

      #if 0
      /* if the condition of an if instruction
         is defined in the previous GET_POINTER instruction and
         this is the only usage then
         mark the itemp as accumulator use */
      if ((POINTER_GET (ic) && getSize (operandType (IC_RESULT (ic))) <=1) &&
          ic->next && ic->next->op == IFX &&
          bitVectnBitsOn (OP_USES(IC_RESULT(ic)))==1 &&
          isOperandEqual (IC_RESULT (ic), IC_COND (ic->next)) &&
          OP_SYMBOL (IC_RESULT (ic))->liveTo <= ic->next->seq)
        {
          OP_SYMBOL (IC_RESULT (ic))->accuse = 1;
          continue;
        }

      if (ic->op != IFX && ic->op !=JUMPTABLE && !POINTER_SET (ic)
          && IC_RESULT (ic) && IS_ITEMP (IC_RESULT (ic))
          && getSize (operandType (IC_RESULT (ic))) == 1
          && bitVectnBitsOn (OP_USES (IC_RESULT (ic))) == 1
          && ic->next
          && OP_SYMBOL (IC_RESULT (ic))->liveTo <= ic->next->seq)
        {
          int accuse = 0;

          if (ic->next->op == IFX)
            {
              if (isOperandEqual (IC_RESULT (ic), IC_COND (ic->next)))
                accuse = 1;
            }
          else if (ic->next->op == JUMPTABLE)
            {
               if (isOperandEqual (IC_RESULT (ic), IC_JTCOND (ic->next)))
                 accuse = 1;
            }
          else
            {
               if (isOperandEqual (IC_RESULT (ic), IC_LEFT (ic->next)))
                 accuse = 1;
               if (isOperandEqual (IC_RESULT (ic), IC_RIGHT (ic->next)))
                 accuse = 1;
            }

          if (accuse)
            {
              OP_SYMBOL (IC_RESULT (ic))->accuse = 1;
              continue;
            }

        }
      #endif

      /* reduce for support function calls */
      if (ic->supportRtn || (ic->op != IFX && ic->op != JUMPTABLE))
        packRegsForSupport (ic, ebp);

      #if 0
      /* some cases the redundant moves can
         can be eliminated for return statements */
      if ((ic->op == RETURN || (ic->op == SEND && ic->argreg == 1)) &&
          /* !isOperandInFarSpace (IC_LEFT (ic)) && */
          options.model == MODEL_SMALL)
        {
          packRegsForOneuse (ic, IC_LEFT (ic), ebp);
        }

      /* if pointer set & left has a size more than
         one and right is not in far space */
      if (POINTER_SET (ic) &&
          /* !isOperandInFarSpace (IC_RIGHT (ic)) && */
          !OP_SYMBOL (IC_RESULT (ic))->remat &&
          !IS_OP_RUONLY (IC_RIGHT (ic))
          /* && getSize (aggrToPtr (operandType (IC_RESULT (ic)), FALSE)) > 1 */ )
        {
          packRegsForOneuse (ic, IC_RESULT (ic), ebp);
        }

      /* if pointer get */
      if (POINTER_GET (ic) &&
          IS_SYMOP (IC_LEFT (ic)) &&
          /* !isOperandInFarSpace (IC_RESULT (ic)) && */
          !OP_SYMBOL (IC_LEFT (ic))->remat &&
          !IS_OP_RUONLY (IC_RESULT (ic))
           /* && getSize (aggrToPtr (operandType (IC_LEFT (ic)), FALSE)) > 1 */)
        {
          packRegsForOneuse (ic, IC_LEFT (ic), ebp);
        }

      /* if this is a cast for integral promotion then
         check if it's the only use of the definition of the
         operand being casted/ if yes then replace
         the result of that arithmetic operation with
         this result and get rid of the cast */
      if (ic->op == CAST)
        {
          sym_link *fromType = operandType (IC_RIGHT (ic));
          sym_link *toType = operandType (IC_LEFT (ic));

          if (IS_INTEGRAL (fromType) && IS_INTEGRAL (toType) &&
              getSize (fromType) != getSize (toType) &&
              SPEC_USIGN (fromType) == SPEC_USIGN (toType))
            {
              iCode *dic = packRegsForOneuse (ic, IC_RIGHT (ic), ebp);
              if (dic)
                {
                  if (IS_ARITHMETIC_OP (dic))
                    {
                      bitVectUnSetBit(OP_SYMBOL(IC_RESULT(dic))->defs,dic->key);
                      ReplaceOpWithCheaperOp(&IC_RESULT (dic), IC_RESULT (ic));
                      remiCodeFromeBBlock (ebp, ic);
                      bitVectUnSetBit(OP_SYMBOL(IC_RESULT(ic))->defs,ic->key);
                      hTabDeleteItem (&iCodehTab, ic->key, ic, DELETE_ITEM, NULL);
                      OP_DEFS(IC_RESULT (dic))=bitVectSetBit (OP_DEFS (IC_RESULT (dic)), dic->key);
                      ic = ic->prev;
                    }
                  else
                    {
                      OP_SYMBOL (IC_RIGHT (ic))->ruonly = 0;
                    }
                }
            }
          else
            {
              /* if the type from and type to are the same
                 then if this is the only use then pack it */
              if (compareType (operandType (IC_RIGHT (ic)),
                             operandType (IC_LEFT (ic))) == 1)
                {
                  iCode *dic = packRegsForOneuse (ic, IC_RIGHT (ic), ebp);
                  if (dic)
                    {
                      bitVectUnSetBit(OP_SYMBOL(IC_RESULT(dic))->defs,dic->key);
                      ReplaceOpWithCheaperOp(&IC_RESULT (dic), IC_RESULT (ic));
                      remiCodeFromeBBlock (ebp, ic);
                      bitVectUnSetBit(OP_SYMBOL(IC_RESULT(ic))->defs,ic->key);
                      hTabDeleteItem (&iCodehTab, ic->key, ic, DELETE_ITEM, NULL);
                      OP_DEFS(IC_RESULT (dic))=bitVectSetBit (OP_DEFS (IC_RESULT (dic)), dic->key);
                      ic = ic->prev;
                    }
                }
            }
        }
      #endif

      /* pack for PUSH
         iTempNN := (some variable in farspace) V1
         push iTempNN ;
         -------------
         push V1
       */
      if (ic->op == IPUSH)
        {
          packForPush (ic, ebpp, blockno);
        }

      packRegsForAccUse (ic);
    }
}

/*-----------------------------------------------------------------*/
/* assignRegisters - assigns registers to each live range as need  */
/*-----------------------------------------------------------------*/
void
hc08_assignRegisters (ebbIndex * ebbi)
{
  eBBlock ** ebbs = ebbi->bbOrder;
  int count = ebbi->count;
  iCode *ic;
  int i;

  setToNull ((void *) &_G.funcrUsed);
  setToNull ((void *) &_G.regAssigned);
  setToNull ((void *) &_G.totRegAssigned);
  hc08_ptrRegReq = _G.stackExtend = _G.dataExtend = 0;
  hc08_nRegs = 7;
  hc08_reg_a = hc08_regWithIdx(A_IDX);
  hc08_reg_x = hc08_regWithIdx(X_IDX);
  hc08_reg_h = hc08_regWithIdx(H_IDX);
  hc08_reg_hx = hc08_regWithIdx(HX_IDX);
  hc08_reg_xa = hc08_regWithIdx(XA_IDX);
  hc08_reg_sp = hc08_regWithIdx(SP_IDX);
  hc08_nRegs = 5;

  /* change assignments this will remove some
     live ranges reducing some register pressure */

  for (i = 0; i < count; i++)
    packRegisters (ebbs, i);

  /* liveranges probably changed by register packing
     so we compute them again */
  recomputeLiveRanges (ebbs, count);

  if (options.dump_pack)
    dumpEbbsToFileExt (DUMP_PACK, ebbi);

  /* first determine for each live range the number of
     registers & the type of registers required for each */
  regTypeNum (*ebbs);

  /* and serially allocate registers */
  serialRegAssign (ebbs, count);

  freeAllRegs ();
  //setToNull ((void *) &_G.regAssigned);
  //setToNull ((void *) &_G.totRegAssigned);
  fillGaps();

  /* if stack was extended then tell the user */
  if (_G.stackExtend)
    {
/*      werror(W_TOOMANY_SPILS,"stack", */
/*             _G.stackExtend,currFunc->name,""); */
      _G.stackExtend = 0;
    }

  if (_G.dataExtend)
    {
/*      werror(W_TOOMANY_SPILS,"data space", */
/*             _G.dataExtend,currFunc->name,""); */
      _G.dataExtend = 0;
    }

  /* after that create the register mask
     for each of the instruction */
  createRegMask (ebbs, count);

  /* redo that offsets for stacked automatic variables */
  if (currFunc)
    {
      redoStackOffsets ();
    }

  if (options.dump_rassgn)
    {
      dumpEbbsToFileExt (DUMP_RASSGN, ebbi);
      dumpLiveRanges (DUMP_LRANGE, liveRanges);
    }

  /* do the overlaysegment stuff SDCCmem.c */
  doOverlays (ebbs, count);

  /* now get back the chain */
  ic = iCodeLabelOptimize (iCodeFromeBBlock (ebbs, count));

  genhc08Code (ic);

  /* free up any _G.stackSpil locations allocated */
  applyToSet (_G.stackSpil, deallocStackSpil);
  _G.slocNum = 0;
  setToNull ((void *) &_G.stackSpil);
  setToNull ((void *) &_G.spiltSet);
  /* mark all registers as free */
  freeAllRegs ();

  return;
}
