/*-------------------------------------------------------------------------
  gen.c - source file for code generation for pic
  
  Written By -  Sandeep Dutta . sandeep.dutta@usa.net (1998)
         and -  Jean-Louis VERN.jlvern@writeme.com (1999)
  Bug Fixes  -  Wojciech Stryjewski  wstryj1@tiger.lsu.edu (1999 v2.1.9a)
  PIC port   -  Scott Dattalo scott@dattalo.com (2000)
  
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
  
  Notes:
  000123 mlh	Moved aopLiteral to SDCCglue.c to help the split
  		Made everything static
-------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "SDCCglobl.h"
#include "newalloc.h"

#include "common.h"
#include "SDCCpeeph.h"
#include "ralloc.h"
#include "pcode.h"
#include "gen.h"


extern void genUMult8X8_16 (operand *, operand *,operand *,pCodeOpReg *);
extern void genSMult8X8_16 (operand *, operand *,operand *,pCodeOpReg *);
void genMult8X8_8 (operand *, operand *,operand *);
pCode *AssembleLine(char *line);
extern void printpBlock(FILE *of, pBlock *pb);

static int labelOffset=0;
extern int debug_verbose;
static int optimized_for_speed = 0;

/* max_key keeps track of the largest label number used in 
   a function. This is then used to adjust the label offset
   for the next function.
*/
static int max_key=0;
static int GpsuedoStkPtr=0;

pCodeOp *popGetImmd(char *name, unsigned int offset, int index,int is_func);
unsigned int pic14aopLiteral (value *val, int offset);
const char *AopType(short type);
static iCode *ifxForOp ( operand *op, iCode *ic );

#define BYTEofLONG(l,b) ( (l>> (b<<3)) & 0xff)

/* this is the down and dirty file with all kinds of 
   kludgy & hacky stuff. This is what it is all about
   CODE GENERATION for a specific MCU . some of the
   routines may be reusable, will have to see */

static char *zero = "#0x00";
static char *one  = "#0x01";
static char *spname = "sp";

char *fReturnpic14[] = {"temp1","temp2","temp3","temp4" };
//char *fReturn390[] = {"dpl","dph","dpx", "b","a" };
unsigned fReturnSizePic = 4; /* shared with ralloc.c */
static char **fReturn = fReturnpic14;

static char *accUse[] = {"a","b"};

//static short rbank = -1;

static struct {
    short r0Pushed;
    short r1Pushed;
    short accInUse;
    short inLine;
    short debugLine;
    short nRegsSaved;
    set *sendSet;
} _G;

/* Resolved ifx structure. This structure stores information
   about an iCode ifx that makes it easier to generate code.
*/
typedef struct resolvedIfx {
  symbol *lbl;     /* pointer to a label */
  int condition;   /* true or false ifx */
  int generated;   /* set true when the code associated with the ifx
		    * is generated */
} resolvedIfx;

extern int pic14_ptrRegReq ;
extern int pic14_nRegs;
extern FILE *codeOutFile;
static void saverbank (int, iCode *,bool);

static lineNode *lineHead = NULL;
static lineNode *lineCurr = NULL;

static unsigned char   SLMask[] = {0xFF ,0xFE, 0xFC, 0xF8, 0xF0,
0xE0, 0xC0, 0x80, 0x00};
static unsigned char   SRMask[] = {0xFF, 0x7F, 0x3F, 0x1F, 0x0F,
0x07, 0x03, 0x01, 0x00};

static  pBlock *pb;

/*-----------------------------------------------------------------*/
/*  my_powof2(n) - If `n' is an integaer power of 2, then the      */
/*                 exponent of 2 is returned, otherwise -1 is      */
/*                 returned.                                       */
/* note that this is similar to the function `powof2' in SDCCsymt  */
/* if(n == 2^y)                                                    */
/*   return y;                                                     */
/* return -1;                                                      */
/*-----------------------------------------------------------------*/
static int my_powof2 (unsigned long num)
{
  if(num) {
    if( (num & (num-1)) == 0) {
      int nshifts = -1;
      while(num) {
	num>>=1;
	nshifts++;
      }
      return nshifts;
    }
  }

  return -1;
}

void DEBUGpic14_AopType(int line_no, operand *left, operand *right, operand *result)
{

  DEBUGpic14_emitcode ("; ","line = %d result %s=%s, left %s=%s, right %s=%s, size = %d",
		       line_no,
		       ((result) ? AopType(AOP_TYPE(result)) : "-"),
		       ((result) ? aopGet(AOP(result),0,TRUE,FALSE) : "-"),
		       ((left)   ? AopType(AOP_TYPE(left)) : "-"),
		       ((left)   ? aopGet(AOP(left),0,TRUE,FALSE) : "-"),
		       ((right)  ? AopType(AOP_TYPE(right)) : "-"),
		       ((right)  ? aopGet(AOP(right),0,FALSE,FALSE) : "-"),
		       ((result) ? AOP_SIZE(result) : 0));

}

void DEBUGpic14_AopTypeSign(int line_no, operand *left, operand *right, operand *result)
{

  DEBUGpic14_emitcode ("; ","line = %d, signs: result %s=%c, left %s=%c, right %s=%c",
		       line_no,
		       ((result) ? AopType(AOP_TYPE(result)) : "-"),
		       ((result) ? (SPEC_USIGN(operandType(result)) ? 'u' : 's') : '-'),
		       ((left)   ? AopType(AOP_TYPE(left)) : "-"),
		       ((left)   ? (SPEC_USIGN(operandType(left))   ? 'u' : 's') : '-'),
		       ((right)  ? AopType(AOP_TYPE(right)) : "-"),
		       ((right)  ? (SPEC_USIGN(operandType(right))  ? 'u' : 's') : '-'));

}

void DEBUGpic14_emitcode (char *inst,char *fmt, ...)
{
    va_list ap;
    char lb[INITIAL_INLINEASM];  
    char *lbp = lb;

    if(!debug_verbose)
      return;

    va_start(ap,fmt);   

    if (inst && *inst) {
	if (fmt && *fmt)
	    sprintf(lb,"%s\t",inst);
	else
	    sprintf(lb,"%s",inst);
        vsprintf(lb+(strlen(lb)),fmt,ap);
    }  else
        vsprintf(lb,fmt,ap);

    while (isspace(*lbp)) lbp++;

    if (lbp && *lbp) 
        lineCurr = (lineCurr ?
                    connectLine(lineCurr,newLineNode(lb)) :
                    (lineHead = newLineNode(lb)));
    lineCurr->isInline = _G.inLine;
    lineCurr->isDebug  = _G.debugLine;

    addpCode2pBlock(pb,newpCodeCharP(lb));

    va_end(ap);
}


void emitpLabel(int key)
{
  addpCode2pBlock(pb,newpCodeLabel(NULL,key+100+labelOffset));
}

void emitpcode(PIC_OPCODE poc, pCodeOp *pcop)
{
  if(pcop)
    addpCode2pBlock(pb,newpCode(poc,pcop));
  else
    DEBUGpic14_emitcode(";","%s  ignoring NULL pcop",__FUNCTION__);
}

void emitpcodeNULLop(PIC_OPCODE poc)
{

  addpCode2pBlock(pb,newpCode(poc,NULL));

}

void emitpcodePagesel(const char *label)
{

  char code[81];
  strcpy(code,"\tpagesel ");
  strcat(code,label);
  addpCode2pBlock(pb,newpCodeInlineP(code));

}

/*-----------------------------------------------------------------*/
/* pic14_emitcode - writes the code into a file : for now it is simple    */
/*-----------------------------------------------------------------*/
void pic14_emitcode (char *inst,char *fmt, ...)
{
    va_list ap;
    char lb[INITIAL_INLINEASM];  
    char *lbp = lb;

    va_start(ap,fmt);   

    if (inst && *inst) {
	if (fmt && *fmt)
	    sprintf(lb,"%s\t",inst);
	else
	    sprintf(lb,"%s",inst);
        vsprintf(lb+(strlen(lb)),fmt,ap);
    }  else
        vsprintf(lb,fmt,ap);

    while (isspace(*lbp)) lbp++;

    if (lbp && *lbp) 
        lineCurr = (lineCurr ?
                    connectLine(lineCurr,newLineNode(lb)) :
                    (lineHead = newLineNode(lb)));
    lineCurr->isInline = _G.inLine;
    lineCurr->isDebug  = _G.debugLine;

    if(debug_verbose)
      addpCode2pBlock(pb,newpCodeCharP(lb));

    va_end(ap);
}


/*-----------------------------------------------------------------*/
/* getFreePtr - returns r0 or r1 whichever is free or can be pushed*/
/*-----------------------------------------------------------------*/
static regs *getFreePtr (iCode *ic, asmop **aopp, bool result)
{
    bool r0iu = FALSE , r1iu = FALSE;
    bool r0ou = FALSE , r1ou = FALSE;

    /* the logic: if r0 & r1 used in the instruction
    then we are in trouble otherwise */

    /* first check if r0 & r1 are used by this
    instruction, in which case we are in trouble */
    if ((r0iu = bitVectBitValue(ic->rUsed,R0_IDX)) &&
        (r1iu = bitVectBitValue(ic->rUsed,R1_IDX))) 
    {
        goto endOfWorld;      
    }

    r0ou = bitVectBitValue(ic->rMask,R0_IDX);
    r1ou = bitVectBitValue(ic->rMask,R1_IDX);

    /* if no usage of r0 then return it */
    if (!r0iu && !r0ou) {
        ic->rUsed = bitVectSetBit(ic->rUsed,R0_IDX);
        (*aopp)->type = AOP_R0;
        
        return (*aopp)->aopu.aop_ptr = pic14_regWithIdx(R0_IDX);
    }

    /* if no usage of r1 then return it */
    if (!r1iu && !r1ou) {
        ic->rUsed = bitVectSetBit(ic->rUsed,R1_IDX);
        (*aopp)->type = AOP_R1;

        return (*aopp)->aopu.aop_ptr = pic14_regWithIdx(R1_IDX);
    }    

    /* now we know they both have usage */
    /* if r0 not used in this instruction */
    if (!r0iu) {
        /* push it if not already pushed */
        if (!_G.r0Pushed) {
	  //pic14_emitcode ("push","%s",
	  //          pic14_regWithIdx(R0_IDX)->dname);
            _G.r0Pushed++ ;
        }
        
        ic->rUsed = bitVectSetBit(ic->rUsed,R0_IDX);
        (*aopp)->type = AOP_R0;

        return (*aopp)->aopu.aop_ptr = pic14_regWithIdx(R0_IDX);
    }

    /* if r1 not used then */

    if (!r1iu) {
        /* push it if not already pushed */
        if (!_G.r1Pushed) {
	  //pic14_emitcode ("push","%s",
	  //          pic14_regWithIdx(R1_IDX)->dname);
            _G.r1Pushed++ ;
        }
        
        ic->rUsed = bitVectSetBit(ic->rUsed,R1_IDX);
        (*aopp)->type = AOP_R1;
        return pic14_regWithIdx(R1_IDX);
    }

endOfWorld :
    /* I said end of world but not quite end of world yet */
    /* if this is a result then we can push it on the stack*/
    if (result) {
        (*aopp)->type = AOP_STK;    
        return NULL;
    }

    /* other wise this is true end of the world */
    werror(E_INTERNAL_ERROR,__FILE__,__LINE__,
           "getFreePtr should never reach here");
    exit(0);
}

/*-----------------------------------------------------------------*/
/* newAsmop - creates a new asmOp                                  */
/*-----------------------------------------------------------------*/
asmop *newAsmop (short type)
{
    asmop *aop;

    aop = Safe_calloc(1,sizeof(asmop));
    aop->type = type;
    return aop;
}

static void genSetDPTR(int n)
{
    if (!n)
    {
        pic14_emitcode(";", "Select standard DPTR");
        pic14_emitcode("mov", "dps, #0x00");
    }
    else
    {
        pic14_emitcode(";", "Select alternate DPTR");
        pic14_emitcode("mov", "dps, #0x01");
    }
}

/*-----------------------------------------------------------------*/
/* resolveIfx - converts an iCode ifx into a form more useful for  */
/*              generating code                                    */
/*-----------------------------------------------------------------*/
static void resolveIfx(resolvedIfx *resIfx, iCode *ifx)
{
  if(!resIfx) 
    return;

  //  DEBUGpic14_emitcode("; ***","%s %d",__FUNCTION__,__LINE__);

  resIfx->condition = 1;    /* assume that the ifx is true */
  resIfx->generated = 0;    /* indicate that the ifx has not been used */

  if(!ifx) {
    resIfx->lbl = newiTempLabel(NULL);  /* oops, there is no ifx. so create a label */
/*
    DEBUGpic14_emitcode("; ***","%s %d null ifx creating new label key =%d",
			__FUNCTION__,__LINE__,resIfx->lbl->key);
*/
  } else {
    if(IC_TRUE(ifx)) {
      resIfx->lbl = IC_TRUE(ifx);
    } else {
      resIfx->lbl = IC_FALSE(ifx);
      resIfx->condition = 0;
    }
/*
    if(IC_TRUE(ifx)) 
      DEBUGpic14_emitcode("; ***","ifx true is non-null");
    if(IC_FALSE(ifx)) 
      DEBUGpic14_emitcode("; ***","ifx false is non-null");
*/
  }

  //  DEBUGpic14_emitcode("; ***","%s lbl->key=%d, (lab offset=%d)",__FUNCTION__,resIfx->lbl->key,labelOffset);

}
/*-----------------------------------------------------------------*/
/* pointerCode - returns the code for a pointer type               */
/*-----------------------------------------------------------------*/
static int pointerCode (sym_link *etype)
{

    return PTR_TYPE(SPEC_OCLS(etype));

}

/*-----------------------------------------------------------------*/
/* aopForSym - for a true symbol                                   */
/*-----------------------------------------------------------------*/
static asmop *aopForSym (iCode *ic,symbol *sym,bool result)
{
    asmop *aop;
    memmap *space= SPEC_OCLS(sym->etype);

    DEBUGpic14_emitcode("; ***","%s %d",__FUNCTION__,__LINE__);
    /* if already has one */
    if (sym->aop)
        return sym->aop;

    /* assign depending on the storage class */
    /* if it is on the stack or indirectly addressable */
    /* space we need to assign either r0 or r1 to it   */    
    if ((sym->onStack && !options.stack10bit) || sym->iaccess) {
        sym->aop = aop = newAsmop(0);
        aop->aopu.aop_ptr = getFreePtr(ic,&aop,result);
        aop->size = getSize(sym->type);

        /* now assign the address of the variable to 
        the pointer register */
        if (aop->type != AOP_STK) {

            if (sym->onStack) {
                    if ( _G.accInUse )
                        pic14_emitcode("push","acc");

                    pic14_emitcode("mov","a,_bp");
                    pic14_emitcode("add","a,#0x%02x",
                             ((sym->stack < 0) ?
			      ((char)(sym->stack - _G.nRegsSaved )) :
			      ((char)sym->stack)) & 0xff);
                    pic14_emitcode("mov","%s,a",
                             aop->aopu.aop_ptr->name);

                    if ( _G.accInUse )
                        pic14_emitcode("pop","acc");
            } else
                pic14_emitcode("mov","%s,#%s",
                         aop->aopu.aop_ptr->name,
                         sym->rname);
            aop->paged = space->paged;
        } else
            aop->aopu.aop_stk = sym->stack;
        return aop;
    }
    
    if (sym->onStack && options.stack10bit)
    {
        /* It's on the 10 bit stack, which is located in
         * far data space.
         */
         
      //DEBUGpic14_emitcode(";","%d",__LINE__);

        if ( _G.accInUse )
        	pic14_emitcode("push","acc");

        pic14_emitcode("mov","a,_bp");
        pic14_emitcode("add","a,#0x%02x",
                 ((sym->stack < 0) ?
                   ((char)(sym->stack - _G.nRegsSaved )) :
                   ((char)sym->stack)) & 0xff);
        
        genSetDPTR(1);
        pic14_emitcode ("mov","dpx1,#0x40");
        pic14_emitcode ("mov","dph1,#0x00");
        pic14_emitcode ("mov","dpl1, a");
        genSetDPTR(0);
    	
        if ( _G.accInUse )
            pic14_emitcode("pop","acc");
            
        sym->aop = aop = newAsmop(AOP_DPTR2);
    	aop->size = getSize(sym->type); 
    	return aop;
    }

    //DEBUGpic14_emitcode(";","%d",__LINE__);
    /* if in bit space */
    if (IN_BITSPACE(space)) {
        sym->aop = aop = newAsmop (AOP_CRY);
        aop->aopu.aop_dir = sym->rname ;
        aop->size = getSize(sym->type);
	//DEBUGpic14_emitcode(";","%d sym->rname = %s, size = %d",__LINE__,sym->rname,aop->size);
        return aop;
    }
    /* if it is in direct space */
    if (IN_DIRSPACE(space)) {
        sym->aop = aop = newAsmop (AOP_DIR);
        aop->aopu.aop_dir = sym->rname ;
        aop->size = getSize(sym->type);
	DEBUGpic14_emitcode(";","%d sym->rname = %s, size = %d",__LINE__,sym->rname,aop->size);
        return aop;
    }

    /* special case for a function */
    if (IS_FUNC(sym->type)) {   

      sym->aop = aop = newAsmop(AOP_PCODE);
      aop->aopu.pcop = popGetImmd(sym->rname,0,0,1);
      PCOI(aop->aopu.pcop)->_const = IN_CODESPACE(space);
      PCOI(aop->aopu.pcop)->_function = 1;
      PCOI(aop->aopu.pcop)->index = 0;
      aop->size = FPTRSIZE; 
      /*
        sym->aop = aop = newAsmop(AOP_IMMD);    
	aop->aopu.aop_immd = Safe_calloc(1,strlen(sym->rname)+1);
        strcpy(aop->aopu.aop_immd,sym->rname);
        aop->size = FPTRSIZE; 
      */
	DEBUGpic14_emitcode(";","%d size = %d, name =%s",__LINE__,aop->size,sym->rname);
        return aop;
    }


    /* only remaining is far space */
    /* in which case DPTR gets the address */
    sym->aop = aop = newAsmop(AOP_PCODE);

    aop->aopu.pcop = popGetImmd(sym->rname,0,0,0);
    PCOI(aop->aopu.pcop)->_const = IN_CODESPACE(space);
    PCOI(aop->aopu.pcop)->index = 0;

    DEBUGpic14_emitcode(";","%d: rname %s, val %d, const = %d",
			__LINE__,sym->rname, 0, PCOI(aop->aopu.pcop)->_const);

    allocDirReg (IC_LEFT(ic));

    aop->size = FPTRSIZE; 
/*
    DEBUGpic14_emitcode(";","%d size = %d, name =%s",__LINE__,aop->size,sym->rname);
    sym->aop = aop = newAsmop(AOP_DPTR);
    pic14_emitcode ("mov","dptr,#%s", sym->rname);
    aop->size = getSize(sym->type);

    DEBUGpic14_emitcode(";","%d size = %d",__LINE__,aop->size);
*/

    /* if it is in code space */
    if (IN_CODESPACE(space))
        aop->code = 1;

    return aop;     
}

/*-----------------------------------------------------------------*/
/* aopForRemat - rematerialzes an object                           */
/*-----------------------------------------------------------------*/
static asmop *aopForRemat (operand *op) // x symbol *sym)
{
  symbol *sym = OP_SYMBOL(op);
  iCode *ic = NULL;
  asmop *aop = newAsmop(AOP_PCODE);
  int val = 0;
  int offset = 0;

  ic = sym->rematiCode;

  DEBUGpic14_emitcode(";","%s %d",__FUNCTION__,__LINE__);
  if(IS_OP_POINTER(op)) {
    DEBUGpic14_emitcode(";","%s %d IS_OP_POINTER",__FUNCTION__,__LINE__);
  }
  for (;;) {
    if (ic->op == '+') {
      val += (int) operandLitValue(IC_RIGHT(ic));
    } else if (ic->op == '-') {
      val -= (int) operandLitValue(IC_RIGHT(ic));
    } else
      break;
	
    ic = OP_SYMBOL(IC_LEFT(ic))->rematiCode;
  }

  offset = OP_SYMBOL(IC_LEFT(ic))->offset;
  aop->aopu.pcop = popGetImmd(OP_SYMBOL(IC_LEFT(ic))->rname,0,val,0);
  PCOI(aop->aopu.pcop)->_const = IS_PTR_CONST(operandType(op));
  PCOI(aop->aopu.pcop)->index = val;

  DEBUGpic14_emitcode(";","%d: rname %s, val %d, const = %d",
		      __LINE__,OP_SYMBOL(IC_LEFT(ic))->rname,
		      val, IS_PTR_CONST(operandType(op)));

  //    DEBUGpic14_emitcode(";","aop type  %s",AopType(AOP_TYPE(IC_LEFT(ic))));

  allocDirReg (IC_LEFT(ic));

  return aop;        
}

int aopIdx (asmop *aop, int offset)
{
  if(!aop)
    return -1;

  if(aop->type !=  AOP_REG)
    return -2;
	
  return aop->aopu.aop_reg[offset]->rIdx;

}
/*-----------------------------------------------------------------*/
/* regsInCommon - two operands have some registers in common       */
/*-----------------------------------------------------------------*/
static bool regsInCommon (operand *op1, operand *op2)
{
    symbol *sym1, *sym2;
    int i;

    /* if they have registers in common */
    if (!IS_SYMOP(op1) || !IS_SYMOP(op2))
        return FALSE ;

    sym1 = OP_SYMBOL(op1);
    sym2 = OP_SYMBOL(op2);

    if (sym1->nRegs == 0 || sym2->nRegs == 0)
        return FALSE ;

    for (i = 0 ; i < sym1->nRegs ; i++) {
        int j;
        if (!sym1->regs[i])
            continue ;

        for (j = 0 ; j < sym2->nRegs ;j++ ) {
            if (!sym2->regs[j])
                continue ;

            if (sym2->regs[j] == sym1->regs[i])
                return TRUE ;
        }
    }

    return FALSE ;
}

/*-----------------------------------------------------------------*/
/* operandsEqu - equivalent                                        */
/*-----------------------------------------------------------------*/
static bool operandsEqu ( operand *op1, operand *op2)
{
    symbol *sym1, *sym2;

    /* if they not symbols */
    if (!IS_SYMOP(op1) || !IS_SYMOP(op2))
        return FALSE;

    sym1 = OP_SYMBOL(op1);
    sym2 = OP_SYMBOL(op2);

    /* if both are itemps & one is spilt
       and the other is not then false */
    if (IS_ITEMP(op1) && IS_ITEMP(op2) &&
	sym1->isspilt != sym2->isspilt )
	return FALSE ;

    /* if they are the same */
    if (sym1 == sym2)
        return TRUE ;

    if (strcmp(sym1->rname,sym2->rname) == 0)
        return TRUE;


    /* if left is a tmp & right is not */
    if (IS_ITEMP(op1)  && 
        !IS_ITEMP(op2) &&
        sym1->isspilt  &&
        (sym1->usl.spillLoc == sym2))
        return TRUE;

    if (IS_ITEMP(op2)  && 
        !IS_ITEMP(op1) &&
        sym2->isspilt  &&
	sym1->level > 0 &&
        (sym2->usl.spillLoc == sym1))
        return TRUE ;

    return FALSE ;
}

/*-----------------------------------------------------------------*/
/* pic14_sameRegs - two asmops have the same registers                   */
/*-----------------------------------------------------------------*/
bool pic14_sameRegs (asmop *aop1, asmop *aop2 )
{
    int i;

    if (aop1 == aop2)
        return TRUE ;

    if (aop1->type != AOP_REG ||
        aop2->type != AOP_REG )
        return FALSE ;

    if (aop1->size != aop2->size )
        return FALSE ;

    for (i = 0 ; i < aop1->size ; i++ )
        if (aop1->aopu.aop_reg[i] !=
            aop2->aopu.aop_reg[i] )
            return FALSE ;

    return TRUE ;
}

/*-----------------------------------------------------------------*/
/* aopOp - allocates an asmop for an operand  :                    */
/*-----------------------------------------------------------------*/
void aopOp (operand *op, iCode *ic, bool result)
{
    asmop *aop;
    symbol *sym;
    int i;

    if (!op)
        return ;

    //    DEBUGpic14_emitcode(";","%d",__LINE__);
    /* if this a literal */
    if (IS_OP_LITERAL(op)) {
        op->aop = aop = newAsmop(AOP_LIT);
        aop->aopu.aop_lit = op->operand.valOperand;
        aop->size = getSize(operandType(op));
        return;
    }

    {
      sym_link *type = operandType(op);
      if(IS_PTR_CONST(type))
	DEBUGpic14_emitcode(";","%d aop type is const pointer",__LINE__);
    }

    /* if already has a asmop then continue */
    if (op->aop)
        return ;

    /* if the underlying symbol has a aop */
    if (IS_SYMOP(op) && OP_SYMBOL(op)->aop) {
      DEBUGpic14_emitcode(";","%d",__LINE__);
        op->aop = OP_SYMBOL(op)->aop;
        return;
    }

    /* if this is a true symbol */
    if (IS_TRUE_SYMOP(op)) {    
      //DEBUGpic14_emitcode(";","%d - true symop",__LINE__);
      op->aop = aopForSym(ic,OP_SYMBOL(op),result);
      return ;
    }

    /* this is a temporary : this has
    only four choices :
    a) register
    b) spillocation
    c) rematerialize 
    d) conditional   
    e) can be a return use only */

    sym = OP_SYMBOL(op);


    /* if the type is a conditional */
    if (sym->regType == REG_CND) {
        aop = op->aop = sym->aop = newAsmop(AOP_CRY);
        aop->size = 0;
        return;
    }

    /* if it is spilt then two situations
    a) is rematerialize 
    b) has a spill location */
    if (sym->isspilt || sym->nRegs == 0) {

      DEBUGpic14_emitcode(";","%d",__LINE__);
        /* rematerialize it NOW */
        if (sym->remat) {

            sym->aop = op->aop = aop =
                                      aopForRemat (op);
            aop->size = getSize(sym->type);
	    //DEBUGpic14_emitcode(";"," %d: size %d, %s\n",__LINE__,aop->size,aop->aopu.aop_immd);
            return;
        }

	if (sym->accuse) {
	    int i;
            aop = op->aop = sym->aop = newAsmop(AOP_ACC);
            aop->size = getSize(sym->type);
            for ( i = 0 ; i < 2 ; i++ )
                aop->aopu.aop_str[i] = accUse[i];
	    DEBUGpic14_emitcode(";","%d size=%d",__LINE__,aop->size);
            return;  
	}

        if (sym->ruonly ) {
	  if(sym->isptr) {  // && sym->uptr 
	    aop = op->aop = sym->aop = newAsmop(AOP_PCODE);
	    aop->aopu.pcop = newpCodeOp(NULL,PO_GPR_POINTER); //popCopyReg(&pc_fsr);

	    //PCOI(aop->aopu.pcop)->_const = 0;
	    //PCOI(aop->aopu.pcop)->index = 0;
	    /*
	      DEBUGpic14_emitcode(";","%d: rname %s, val %d, const = %d",
	      __LINE__,sym->rname, 0, PCOI(aop->aopu.pcop)->_const);
	    */
	    //allocDirReg (IC_LEFT(ic));

	    aop->size = getSize(sym->type);
	    DEBUGpic14_emitcode(";","%d",__LINE__);
	    return;

	  } else {

	    unsigned i;

	    aop = op->aop = sym->aop = newAsmop(AOP_STR);
	    aop->size = getSize(sym->type);
	    for ( i = 0 ; i < fReturnSizePic ; i++ )
	      aop->aopu.aop_str[i] = fReturn[i];

	    DEBUGpic14_emitcode(";","%d",__LINE__);
	    return;
	  }
        }

        /* else spill location  */
	if (sym->usl.spillLoc && getSize(sym->type) != getSize(sym->usl.spillLoc->type)) {
	    /* force a new aop if sizes differ */
	    sym->usl.spillLoc->aop = NULL;
	}
	DEBUGpic14_emitcode(";","%s %d %s sym->rname = %s, offset %d",
			    __FUNCTION__,__LINE__,
			    sym->usl.spillLoc->rname,
			    sym->rname, sym->usl.spillLoc->offset);

	sym->aop = op->aop = aop = newAsmop(AOP_PCODE);
	//aop->aopu.pcop = popGetImmd(sym->usl.spillLoc->rname,0,sym->usl.spillLoc->offset);
	aop->aopu.pcop = popRegFromString(sym->usl.spillLoc->rname, 
					  getSize(sym->type), 
					  sym->usl.spillLoc->offset);
        aop->size = getSize(sym->type);

        return;
    }

    {
      sym_link *type = operandType(op);
      if(IS_PTR_CONST(type)) 
	DEBUGpic14_emitcode(";","%d aop type is const pointer",__LINE__);
    }

    /* must be in a register */
    DEBUGpic14_emitcode(";","%d register type nRegs=%d",__LINE__,sym->nRegs);
    sym->aop = op->aop = aop = newAsmop(AOP_REG);
    aop->size = sym->nRegs;
    for ( i = 0 ; i < sym->nRegs ;i++)
        aop->aopu.aop_reg[i] = sym->regs[i];
}

/*-----------------------------------------------------------------*/
/* freeAsmop - free up the asmop given to an operand               */
/*----------------------------------------------------------------*/
void freeAsmop (operand *op, asmop *aaop, iCode *ic, bool pop)
{   
    asmop *aop ;

    if (!op)
        aop = aaop;
    else 
        aop = op->aop;

    if (!aop)
        return ;

    if (aop->freed)
        goto dealloc; 

    aop->freed = 1;

    /* depending on the asmop type only three cases need work AOP_RO
       , AOP_R1 && AOP_STK */
#if 0
    switch (aop->type) {
        case AOP_R0 :
            if (_G.r0Pushed ) {
                if (pop) {
                    pic14_emitcode ("pop","ar0");     
                    _G.r0Pushed--;
                }
            }
            bitVectUnSetBit(ic->rUsed,R0_IDX);
            break;

        case AOP_R1 :
            if (_G.r1Pushed ) {
                if (pop) {
                    pic14_emitcode ("pop","ar1");
                    _G.r1Pushed--;
                }
            }
            bitVectUnSetBit(ic->rUsed,R1_IDX);          
            break;

        case AOP_STK :
        {
            int sz = aop->size;    
            int stk = aop->aopu.aop_stk + aop->size;
            bitVectUnSetBit(ic->rUsed,R0_IDX);
            bitVectUnSetBit(ic->rUsed,R1_IDX);          

            getFreePtr(ic,&aop,FALSE);
            
            if (options.stack10bit)
            {
                /* I'm not sure what to do here yet... */
                /* #STUB */
            	fprintf(stderr, 
            		"*** Warning: probably generating bad code for "
            		"10 bit stack mode.\n");
            }
            
            if (stk) {
                pic14_emitcode ("mov","a,_bp");
                pic14_emitcode ("add","a,#0x%02x",((char)stk) & 0xff);
                pic14_emitcode ("mov","%s,a",aop->aopu.aop_ptr->name);
            } else {
                pic14_emitcode ("mov","%s,_bp",aop->aopu.aop_ptr->name);
            }

            while (sz--) {
                pic14_emitcode("pop","acc");
                pic14_emitcode("mov","@%s,a",aop->aopu.aop_ptr->name);
                if (!sz) break;
                pic14_emitcode("dec","%s",aop->aopu.aop_ptr->name);
            }
            op->aop = aop;
            freeAsmop(op,NULL,ic,TRUE);
            if (_G.r0Pushed) {
                pic14_emitcode("pop","ar0");
                _G.r0Pushed--;
            }

            if (_G.r1Pushed) {
                pic14_emitcode("pop","ar1");
                _G.r1Pushed--;
            }       
        }
    }
#endif

dealloc:
    /* all other cases just dealloc */
    if (op ) {
        op->aop = NULL;
        if (IS_SYMOP(op)) {
            OP_SYMBOL(op)->aop = NULL;    
            /* if the symbol has a spill */
	    if (SPIL_LOC(op))
                SPIL_LOC(op)->aop = NULL;
        }
    }
}

/*-----------------------------------------------------------------*/
/* aopGet - for fetching value of the aop                          */
/*-----------------------------------------------------------------*/
char *aopGet (asmop *aop, int offset, bool bit16, bool dname)
{
    char *s = buffer ;
    char *rs;

    //DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    /* offset is greater than
    size then zero */
    if (offset > (aop->size - 1) &&
        aop->type != AOP_LIT)
        return zero;

    /* depending on type */
    switch (aop->type) {
	
    case AOP_R0:
    case AOP_R1:
        DEBUGpic14_emitcode(";","%d",__LINE__);
	/* if we need to increment it */       
	while (offset > aop->coff) {        
	    pic14_emitcode ("inc","%s",aop->aopu.aop_ptr->name);  
	    aop->coff++;
	}
	
	while (offset < aop->coff) {
	    pic14_emitcode("dec","%s",aop->aopu.aop_ptr->name);
	    aop->coff--;
	}
	
	aop->coff = offset ;
	if (aop->paged) {
	    pic14_emitcode("movx","a,@%s",aop->aopu.aop_ptr->name);
	    return (dname ? "acc" : "a");
	}       
	sprintf(s,"@%s",aop->aopu.aop_ptr->name);
	rs = Safe_calloc(1,strlen(s)+1);
	strcpy(rs,s);   
	return rs;
	
    case AOP_DPTR:
    case AOP_DPTR2:
        DEBUGpic14_emitcode(";","%d",__LINE__);
    if (aop->type == AOP_DPTR2)
    {
        genSetDPTR(1);
    }
    
	while (offset > aop->coff) {
	    pic14_emitcode ("inc","dptr");
	    aop->coff++;
	}
	
	while (offset < aop->coff) {        
	    pic14_emitcode("lcall","__decdptr");
	    aop->coff--;
	}
	
	aop->coff = offset;
	if (aop->code) {
	    pic14_emitcode("clr","a");
	    pic14_emitcode("movc","a,@a+dptr");
        }
    else {
	    pic14_emitcode("movx","a,@dptr");
    }
	    
    if (aop->type == AOP_DPTR2)
    {
        genSetDPTR(0);
    }
	    
    return (dname ? "acc" : "a");
	
	
    case AOP_IMMD:
	if (bit16) 
	    sprintf (s,"%s",aop->aopu.aop_immd);
	else
	    if (offset) 
		sprintf(s,"(%s >> %d)",
			aop->aopu.aop_immd,
			offset*8);
	    else
		sprintf(s,"%s",
			aop->aopu.aop_immd);
	DEBUGpic14_emitcode(";","%d immd %s",__LINE__,s);
	rs = Safe_calloc(1,strlen(s)+1);
	strcpy(rs,s);   
	return rs;
	
    case AOP_DIR:
      if (offset) {
	sprintf(s,"(%s + %d)",
		aop->aopu.aop_dir,
		offset);
	DEBUGpic14_emitcode(";","oops AOP_DIR did this %s\n",s);
      } else
	    sprintf(s,"%s",aop->aopu.aop_dir);
	rs = Safe_calloc(1,strlen(s)+1);
	strcpy(rs,s);   
	return rs;
	
    case AOP_REG:
      //if (dname) 
      //    return aop->aopu.aop_reg[offset]->dname;
      //else
	    return aop->aopu.aop_reg[offset]->name;
	
    case AOP_CRY:
      //pic14_emitcode(";","%d",__LINE__);
      return aop->aopu.aop_dir;
	
    case AOP_ACC:
        DEBUGpic14_emitcode(";Warning -pic port ignoring get(AOP_ACC)","%d",__LINE__);
	return "AOP_accumulator_bug";

    case AOP_LIT:
	sprintf(s,"0x%02x", pic14aopLiteral (aop->aopu.aop_lit,offset));
	rs = Safe_calloc(1,strlen(s)+1);
	strcpy(rs,s);   
	return rs;
	
    case AOP_STR:
	aop->coff = offset ;
	if (strcmp(aop->aopu.aop_str[offset],"a") == 0 &&
	    dname)
	    return "acc";
        DEBUGpic14_emitcode(";","%d - %s",__LINE__, aop->aopu.aop_str[offset]);
	
	return aop->aopu.aop_str[offset];
	
    case AOP_PCODE:
      {
	pCodeOp *pcop = aop->aopu.pcop;
	DEBUGpic14_emitcode(";","%d: aopGet AOP_PCODE type %s",__LINE__,pCodeOpType(pcop));
	if(pcop->name) {
	  DEBUGpic14_emitcode(";","%s offset %d",pcop->name,PCOI(pcop)->offset);
	  //sprintf(s,"(%s+0x%02x)", pcop->name,PCOI(aop->aopu.pcop)->offset);
	  sprintf(s,"%s", pcop->name);
	} else
	  sprintf(s,"0x%02x", PCOI(aop->aopu.pcop)->offset);

      }
      rs = Safe_calloc(1,strlen(s)+1);
      strcpy(rs,s);   
      return rs;

    }

    werror(E_INTERNAL_ERROR,__FILE__,__LINE__,
           "aopget got unsupported aop->type");
    exit(0);
}


/*-----------------------------------------------------------------*/
/* popGetTempReg - create a new temporary pCodeOp                  */
/*-----------------------------------------------------------------*/
pCodeOp *popGetTempReg(void)
{

  pCodeOp *pcop;

  pcop = newpCodeOp(NULL, PO_GPR_TEMP);
  if(pcop && pcop->type == PO_GPR_TEMP && PCOR(pcop)->r) {
    PCOR(pcop)->r->wasUsed=1;
    PCOR(pcop)->r->isFree=0;
  }

  return pcop;
}

/*-----------------------------------------------------------------*/
/* popGetTempReg - create a new temporary pCodeOp                  */
/*-----------------------------------------------------------------*/
void popReleaseTempReg(pCodeOp *pcop)
{

  if(pcop && pcop->type == PO_GPR_TEMP && PCOR(pcop)->r)
    PCOR(pcop)->r->isFree = 1;

}
/*-----------------------------------------------------------------*/
/* popGetLabel - create a new pCodeOp of type PO_LABEL             */
/*-----------------------------------------------------------------*/
pCodeOp *popGetLabel(unsigned int key)
{

  DEBUGpic14_emitcode ("; ***","%s  key=%d, label offset %d",__FUNCTION__,key, labelOffset);

  if(key>(unsigned int)max_key)
    max_key = key;

  return newpCodeOpLabel(NULL,key+100+labelOffset);
}

/*-------------------------------------------------------------------*/
/* popGetLabel - create a new pCodeOp of type PO_LABEL with offset=1 */
/*-------------------------------------------------------------------*/
pCodeOp *popGetHighLabel(unsigned int key)
{
  pCodeOp *pcop;
  DEBUGpic14_emitcode ("; ***","%s  key=%d, label offset %d",__FUNCTION__,key, labelOffset);

  if(key>(unsigned int)max_key)
    max_key = key;

  pcop = newpCodeOpLabel(NULL,key+100+labelOffset);
  PCOLAB(pcop)->offset = 1;
  return pcop;
}

/*-----------------------------------------------------------------*/
/* popCopyReg - copy a pcode operator                              */
/*-----------------------------------------------------------------*/
pCodeOp *popCopyReg(pCodeOpReg *pc)
{
  pCodeOpReg *pcor;

  pcor = Safe_calloc(1,sizeof(pCodeOpReg) );
  pcor->pcop.type = pc->pcop.type;
  if(pc->pcop.name) {
    if(!(pcor->pcop.name = Safe_strdup(pc->pcop.name)))
      fprintf(stderr,"oops %s %d",__FILE__,__LINE__);
  } else
    pcor->pcop.name = NULL;

  pcor->r = pc->r;
  pcor->rIdx = pc->rIdx;
  pcor->r->wasUsed=1;

  //DEBUGpic14_emitcode ("; ***","%s  , copying %s, rIdx=%d",__FUNCTION__,pc->pcop.name,pc->rIdx);

  return PCOP(pcor);
}
/*-----------------------------------------------------------------*/
/* popGet - asm operator to pcode operator conversion              */
/*-----------------------------------------------------------------*/
pCodeOp *popGetLit(unsigned int lit)
{

  return newpCodeOpLit(lit);
}


/*-----------------------------------------------------------------*/
/* popGetImmd - asm operator to pcode immediate conversion         */
/*-----------------------------------------------------------------*/
pCodeOp *popGetImmd(char *name, unsigned int offset, int index,int is_func)
{

  return newpCodeOpImmd(name, offset,index, 0, is_func);
}


/*-----------------------------------------------------------------*/
/* popGet - asm operator to pcode operator conversion              */
/*-----------------------------------------------------------------*/
pCodeOp *popGetWithString(char *str)
{
  pCodeOp *pcop;


  if(!str) {
    fprintf(stderr,"NULL string %s %d\n",__FILE__,__LINE__);
    exit (1);
  }

  pcop = newpCodeOp(str,PO_STR);

  return pcop;
}

/*-----------------------------------------------------------------*/
/* popRegFromString -                                              */
/*-----------------------------------------------------------------*/
pCodeOp *popRegFromString(char *str, int size, int offset)
{

  pCodeOp *pcop = Safe_calloc(1,sizeof(pCodeOpReg) );
  pcop->type = PO_DIR;

  DEBUGpic14_emitcode(";","%d",__LINE__);

  if(!str)
    str = "BAD_STRING";

  pcop->name = Safe_calloc(1,strlen(str)+1);
  strcpy(pcop->name,str);

  //pcop->name = Safe_strdup( ( (str) ? str : "BAD STRING"));

  PCOR(pcop)->r = dirregWithName(pcop->name);
  if(PCOR(pcop)->r == NULL) {
    //fprintf(stderr,"%d - couldn't find %s in allocated registers, size =%d\n",__LINE__,aop->aopu.aop_dir,aop->size);
    PCOR(pcop)->r = allocRegByName (pcop->name,size);
    DEBUGpic14_emitcode(";","%d  %s   offset=%d - had to alloc by reg name",__LINE__,pcop->name,offset);
  } else {
    DEBUGpic14_emitcode(";","%d  %s   offset=%d",__LINE__,pcop->name,offset);
  }
  PCOR(pcop)->instance = offset;

  return pcop;
}

pCodeOp *popRegFromIdx(int rIdx)
{
  pCodeOp *pcop;

  DEBUGpic14_emitcode ("; ***","%s,%d  , rIdx=0x%x",
		       __FUNCTION__,__LINE__,rIdx);

  pcop = Safe_calloc(1,sizeof(pCodeOpReg) );

  PCOR(pcop)->rIdx = rIdx;
  PCOR(pcop)->r = pic14_regWithIdx(rIdx);
  PCOR(pcop)->r->isFree = 0;
  PCOR(pcop)->r->wasUsed = 1;

  pcop->type = PCOR(pcop)->r->pc_type;


  return pcop;
}
/*-----------------------------------------------------------------*/
/* popGet - asm operator to pcode operator conversion              */
/*-----------------------------------------------------------------*/
pCodeOp *popGet (asmop *aop, int offset) //, bool bit16, bool dname)
{
  //char *s = buffer ;
    //char *rs;

    pCodeOp *pcop;

    //DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    /* offset is greater than
    size then zero */

    if (offset > (aop->size - 1) &&
        aop->type != AOP_LIT)
      return NULL;  //zero;

    /* depending on type */
    switch (aop->type) {
	
    case AOP_R0:
    case AOP_R1:
    case AOP_DPTR:
    case AOP_DPTR2:
    case AOP_ACC:
        DEBUGpic14_emitcode(";8051 legacy","%d type = %s",__LINE__,AopType(aop->type));
	return NULL;
	
    case AOP_IMMD:
      DEBUGpic14_emitcode(";","%d",__LINE__);
      return popGetImmd(aop->aopu.aop_immd,offset,0,0);

    case AOP_DIR:
      return popRegFromString(aop->aopu.aop_dir, aop->size, offset);
#if 0
	pcop = Safe_calloc(1,sizeof(pCodeOpReg) );
	pcop->type = PO_DIR;

	/*
	if (offset)
	    sprintf(s,"(%s + %d)",
		    aop->aopu.aop_dir,
		    offset);
	else
	    sprintf(s,"%s",aop->aopu.aop_dir);
	pcop->name = Safe_calloc(1,strlen(s)+1);
	strcpy(pcop->name,s);   
	*/
	pcop->name = Safe_calloc(1,strlen(aop->aopu.aop_dir)+1);
	strcpy(pcop->name,aop->aopu.aop_dir);   
	PCOR(pcop)->r = dirregWithName(aop->aopu.aop_dir);
	if(PCOR(pcop)->r == NULL) {
	  //fprintf(stderr,"%d - couldn't find %s in allocated registers, size =%d\n",__LINE__,aop->aopu.aop_dir,aop->size);
	  PCOR(pcop)->r = allocRegByName (aop->aopu.aop_dir,aop->size);
	  DEBUGpic14_emitcode(";","%d  %s   offset=%d - had to alloc by reg name",__LINE__,pcop->name,offset);
	} else {
	  DEBUGpic14_emitcode(";","%d  %s   offset=%d",__LINE__,pcop->name,offset);
	}
	PCOR(pcop)->instance = offset;

	return pcop;
#endif
	
    case AOP_REG:
      {
	int rIdx = aop->aopu.aop_reg[offset]->rIdx;

	pcop = Safe_calloc(1,sizeof(pCodeOpReg) );
	PCOR(pcop)->rIdx = rIdx;
	PCOR(pcop)->r = pic14_regWithIdx(rIdx);
	PCOR(pcop)->r->wasUsed=1;
	PCOR(pcop)->r->isFree=0;

	PCOR(pcop)->instance = offset;
	pcop->type = PCOR(pcop)->r->pc_type;
	//rs = aop->aopu.aop_reg[offset]->name;
	DEBUGpic14_emitcode(";","%d regiser idx = %d ",__LINE__,rIdx);
	return pcop;
      }

    case AOP_CRY:
      pcop = newpCodeOpBit(aop->aopu.aop_dir,-1,1);
      PCOR(pcop)->r = dirregWithName(aop->aopu.aop_dir);
      //if(PCOR(pcop)->r == NULL)
      //fprintf(stderr,"%d - couldn't find %s in allocated registers\n",__LINE__,aop->aopu.aop_dir);
      return pcop;
	
    case AOP_LIT:
      return newpCodeOpLit(pic14aopLiteral (aop->aopu.aop_lit,offset));

    case AOP_STR:
      DEBUGpic14_emitcode(";","%d  %s",__LINE__,aop->aopu.aop_str[offset]);
      return newpCodeOpRegFromStr(aop->aopu.aop_str[offset]);
      /*
      pcop = Safe_calloc(1,sizeof(pCodeOpReg) );
      PCOR(pcop)->r = allocRegByName(aop->aopu.aop_str[offset]);
      PCOR(pcop)->rIdx = PCOR(pcop)->r->rIdx;
      pcop->type = PCOR(pcop)->r->pc_type;
      pcop->name = PCOR(pcop)->r->name;

      return pcop;
      */

    case AOP_PCODE:
      DEBUGpic14_emitcode(";","popGet AOP_PCODE (%s) %d %s",pCodeOpType(aop->aopu.pcop),
			  __LINE__, 
			  ((aop->aopu.pcop->name)? (aop->aopu.pcop->name) : "no name"));
      pcop = pCodeOpCopy(aop->aopu.pcop);
      PCOI(pcop)->offset = offset;
      return pcop;
    }

    werror(E_INTERNAL_ERROR,__FILE__,__LINE__,
           "popGet got unsupported aop->type");
    exit(0);
}
/*-----------------------------------------------------------------*/
/* aopPut - puts a string for a aop                                */
/*-----------------------------------------------------------------*/
void aopPut (asmop *aop, char *s, int offset)
{
    char *d = buffer ;
    symbol *lbl ;

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

    if (aop->size && offset > ( aop->size - 1)) {
        werror(E_INTERNAL_ERROR,__FILE__,__LINE__,
               "aopPut got offset > aop->size");
        exit(0);
    }

    /* will assign value to value */
    /* depending on where it is ofcourse */
    switch (aop->type) {
    case AOP_DIR:
      if (offset) {
	sprintf(d,"(%s + %d)",
		aop->aopu.aop_dir,offset);
	fprintf(stderr,"oops aopPut:AOP_DIR did this %s\n",s);

      } else
	    sprintf(d,"%s",aop->aopu.aop_dir);
	
	if (strcmp(d,s)) {
	  DEBUGpic14_emitcode(";","%d",__LINE__);
	  if(strcmp(s,"W"))
	    pic14_emitcode("movf","%s,w",s);
	  pic14_emitcode("movwf","%s",d);

	  if(strcmp(s,"W")) {
	    pic14_emitcode(";BUG!? should have this:movf","%s,w   %d",s,__LINE__);
	    if(offset >= aop->size) {
	      emitpcode(POC_CLRF,popGet(aop,offset));
	      break;
	    } else
	      emitpcode(POC_MOVLW,popGetImmd(s,offset,0,0));
	  }

	  emitpcode(POC_MOVWF,popGet(aop,offset));


	}
	break;
	
    case AOP_REG:
      if (strcmp(aop->aopu.aop_reg[offset]->name,s) != 0) { // &&
	//strcmp(aop->aopu.aop_reg[offset]->dname,s)!= 0){
	  /*
	    if (*s == '@'           ||
		strcmp(s,"r0") == 0 ||
		strcmp(s,"r1") == 0 ||
		strcmp(s,"r2") == 0 ||
		strcmp(s,"r3") == 0 ||
		strcmp(s,"r4") == 0 ||
		strcmp(s,"r5") == 0 ||
		strcmp(s,"r6") == 0 || 
		strcmp(s,"r7") == 0 )
		pic14_emitcode("mov","%s,%s  ; %d",
			 aop->aopu.aop_reg[offset]->dname,s,__LINE__);
	    else
	  */

	  if(strcmp(s,"W")==0 )
	    pic14_emitcode("movf","%s,w  ; %d",s,__LINE__);

	  pic14_emitcode("movwf","%s",
		   aop->aopu.aop_reg[offset]->name);

	  if(strcmp(s,zero)==0) {
	    emitpcode(POC_CLRF,popGet(aop,offset));

	  } else if(strcmp(s,"W")==0) {
	    pCodeOp *pcop = Safe_calloc(1,sizeof(pCodeOpReg) );
	    pcop->type = PO_GPR_REGISTER;

	    PCOR(pcop)->rIdx = -1;
	    PCOR(pcop)->r = NULL;

	    DEBUGpic14_emitcode(";","%d",__LINE__);
	    pcop->name = Safe_strdup(s);
	    emitpcode(POC_MOVFW,pcop);
	    emitpcode(POC_MOVWF,popGet(aop,offset));
	  } else if(strcmp(s,one)==0) {
	    emitpcode(POC_CLRF,popGet(aop,offset));
	    emitpcode(POC_INCF,popGet(aop,offset));
	  } else {
	    emitpcode(POC_MOVWF,popGet(aop,offset));
	  }
	}
	break;
	
    case AOP_DPTR:
    case AOP_DPTR2:
    
    if (aop->type == AOP_DPTR2)
    {
        genSetDPTR(1);
    }
    
	if (aop->code) {
	    werror(E_INTERNAL_ERROR,__FILE__,__LINE__,
		   "aopPut writting to code space");
	    exit(0);
	}
	
	while (offset > aop->coff) {
	    aop->coff++;
	    pic14_emitcode ("inc","dptr");
	}
	
	while (offset < aop->coff) {
	    aop->coff-- ;
	    pic14_emitcode("lcall","__decdptr");
	}
	
	aop->coff = offset;
	
	/* if not in accumulater */
	MOVA(s);        
	
	pic14_emitcode ("movx","@dptr,a");
	
    if (aop->type == AOP_DPTR2)
    {
        genSetDPTR(0);
    }
	break;
	
    case AOP_R0:
    case AOP_R1:
	while (offset > aop->coff) {
	    aop->coff++;
	    pic14_emitcode("inc","%s",aop->aopu.aop_ptr->name);
	}
	while (offset < aop->coff) {
	    aop->coff-- ;
	    pic14_emitcode ("dec","%s",aop->aopu.aop_ptr->name);
	}
	aop->coff = offset;
	
	if (aop->paged) {
	    MOVA(s);           
	    pic14_emitcode("movx","@%s,a",aop->aopu.aop_ptr->name);
	    
	} else
	    if (*s == '@') {
		MOVA(s);
		pic14_emitcode("mov","@%s,a ; %d",aop->aopu.aop_ptr->name,__LINE__);
	    } else
		if (strcmp(s,"r0") == 0 ||
		    strcmp(s,"r1") == 0 ||
		    strcmp(s,"r2") == 0 ||
		    strcmp(s,"r3") == 0 ||
		    strcmp(s,"r4") == 0 ||
		    strcmp(s,"r5") == 0 ||
		    strcmp(s,"r6") == 0 || 
		    strcmp(s,"r7") == 0 ) {
		    char buffer[10];
		    sprintf(buffer,"a%s",s);
		    pic14_emitcode("mov","@%s,%s",
			     aop->aopu.aop_ptr->name,buffer);
		} else
		    pic14_emitcode("mov","@%s,%s",aop->aopu.aop_ptr->name,s);
	
	break;
	
    case AOP_STK:
	if (strcmp(s,"a") == 0)
	    pic14_emitcode("push","acc");
	else
	    pic14_emitcode("push","%s",s);
	
	break;
	
    case AOP_CRY:
	/* if bit variable */
	if (!aop->aopu.aop_dir) {
	    pic14_emitcode("clr","a");
	    pic14_emitcode("rlc","a");
	} else {
	    if (s == zero) 
		pic14_emitcode("clr","%s",aop->aopu.aop_dir);
	    else
		if (s == one)
		    pic14_emitcode("setb","%s",aop->aopu.aop_dir);
		else
		    if (!strcmp(s,"c"))
			pic14_emitcode("mov","%s,c",aop->aopu.aop_dir);
		    else {
			lbl = newiTempLabel(NULL);
			
			if (strcmp(s,"a")) {
			    MOVA(s);
			}
			pic14_emitcode("clr","c");
			pic14_emitcode("jz","%05d_DS_",lbl->key+100);
			pic14_emitcode("cpl","c");
			pic14_emitcode("","%05d_DS_:",lbl->key+100);
			pic14_emitcode("mov","%s,c",aop->aopu.aop_dir);
		    }
	}
	break;
	
    case AOP_STR:
	aop->coff = offset;
	if (strcmp(aop->aopu.aop_str[offset],s))
	    pic14_emitcode ("mov","%s,%s ; %d",aop->aopu.aop_str[offset],s,__LINE__);
	break;
	
    case AOP_ACC:
	aop->coff = offset;
	if (!offset && (strcmp(s,"acc") == 0))
	    break;
	
	if (strcmp(aop->aopu.aop_str[offset],s))
	    pic14_emitcode ("mov","%s,%s ; %d",aop->aopu.aop_str[offset],s, __LINE__);
	break;

    default :
	werror(E_INTERNAL_ERROR,__FILE__,__LINE__,
	       "aopPut got unsupported aop->type");
	exit(0);    
    }    

}

/*-----------------------------------------------------------------*/
/* mov2w - generate either a MOVLW or MOVFW based operand type     */
/*-----------------------------------------------------------------*/
void mov2w (asmop *aop, int offset)
{

  if(!aop)
    return;

  DEBUGpic14_emitcode ("; ***","%s  %d  offset=%d",__FUNCTION__,__LINE__,offset);

  if ( aop->type == AOP_PCODE ||
       aop->type == AOP_LIT ||
       aop->type == AOP_IMMD )
    emitpcode(POC_MOVLW,popGet(aop,offset));
  else
    emitpcode(POC_MOVFW,popGet(aop,offset));

}

/*-----------------------------------------------------------------*/
/* reAdjustPreg - points a register back to where it should        */
/*-----------------------------------------------------------------*/
static void reAdjustPreg (asmop *aop)
{
    int size ;

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    aop->coff = 0;
    if ((size = aop->size) <= 1)
        return ;
    size-- ;
    switch (aop->type) {
        case AOP_R0 :
        case AOP_R1 :
            while (size--)
                pic14_emitcode("dec","%s",aop->aopu.aop_ptr->name);
            break;          
        case AOP_DPTR :
        case AOP_DPTR2:
            if (aop->type == AOP_DPTR2)
    	    {
                genSetDPTR(1);
    	    } 
            while (size--)
            {
                pic14_emitcode("lcall","__decdptr");
            }
                
    	    if (aop->type == AOP_DPTR2)
    	    {
                genSetDPTR(0);
    	    }                
            break;  

    }   

}


#if 0
/*-----------------------------------------------------------------*/
/* opIsGptr: returns non-zero if the passed operand is		   */ 	
/* a generic pointer type.					   */
/*-----------------------------------------------------------------*/ 
static int opIsGptr(operand *op)
{
    sym_link *type = operandType(op);
    
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    if ((AOP_SIZE(op) == GPTRSIZE) && IS_GENPTR(type))
    {
        return 1;
    }
    return 0;        
}
#endif

/*-----------------------------------------------------------------*/
/* pic14_getDataSize - get the operand data size                         */
/*-----------------------------------------------------------------*/
int pic14_getDataSize(operand *op)
{
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);


    return AOP_SIZE(op);

    // tsd- in the pic port, the genptr size is 1, so this code here
    // fails. ( in the 8051 port, the size was 4).
#if 0
    int size;
    size = AOP_SIZE(op);
    if (size == GPTRSIZE)
    {
        sym_link *type = operandType(op);
        if (IS_GENPTR(type))
        {
            /* generic pointer; arithmetic operations
             * should ignore the high byte (pointer type).
             */
            size--;
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
        }
    }
    return size;
#endif
}

/*-----------------------------------------------------------------*/
/* pic14_outAcc - output Acc                                             */
/*-----------------------------------------------------------------*/
void pic14_outAcc(operand *result)
{
  int size,offset;
  DEBUGpic14_emitcode ("; ***","%s  %d - ",__FUNCTION__,__LINE__);
  DEBUGpic14_AopType(__LINE__,NULL,NULL,result);


  size = pic14_getDataSize(result);
  if(size){
    emitpcode(POC_MOVWF,popGet(AOP(result),0));
    size--;
    offset = 1;
    /* unsigned or positive */
    while(size--)
      emitpcode(POC_CLRF,popGet(AOP(result),offset++));
  }

}

/*-----------------------------------------------------------------*/
/* pic14_outBitC - output a bit C                                        */
/*-----------------------------------------------------------------*/
void pic14_outBitC(operand *result)
{

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    /* if the result is bit */
    if (AOP_TYPE(result) == AOP_CRY) 
        aopPut(AOP(result),"c",0);
    else {
        pic14_emitcode("clr","a  ; %d", __LINE__);
        pic14_emitcode("rlc","a");
        pic14_outAcc(result);
    }
}

/*-----------------------------------------------------------------*/
/* pic14_toBoolean - emit code for orl a,operator(sizeop)                */
/*-----------------------------------------------------------------*/
void pic14_toBoolean(operand *oper)
{
    int size = AOP_SIZE(oper) - 1;
    int offset = 1;

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

    if ( AOP_TYPE(oper) != AOP_ACC) {
      emitpcode(POC_MOVFW,popGet(AOP(oper),0));
    }
    while (size--) {
      emitpcode(POC_IORFW, popGet(AOP(oper),offset++));
    }
}


/*-----------------------------------------------------------------*/
/* genNot - generate code for ! operation                          */
/*-----------------------------------------------------------------*/
static void genNot (iCode *ic)
{
  symbol *tlbl;
  int size;

  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
  /* assign asmOps to operand & result */
  aopOp (IC_LEFT(ic),ic,FALSE);
  aopOp (IC_RESULT(ic),ic,TRUE);

  DEBUGpic14_AopType(__LINE__,IC_LEFT(ic),NULL,IC_RESULT(ic));
  /* if in bit space then a special case */
  if (AOP_TYPE(IC_LEFT(ic)) == AOP_CRY) {
    if (AOP_TYPE(IC_RESULT(ic)) == AOP_CRY) {
      emitpcode(POC_MOVLW,popGet(AOP(IC_LEFT(ic)),0));
      emitpcode(POC_XORWF,popGet(AOP(IC_RESULT(ic)),0));
    } else {
      emitpcode(POC_CLRF,popGet(AOP(IC_RESULT(ic)),0));
      emitpcode(POC_BTFSS,popGet(AOP(IC_LEFT(ic)),0));
      emitpcode(POC_INCF,popGet(AOP(IC_RESULT(ic)),0));
    }
    goto release;
  }

  size = AOP_SIZE(IC_LEFT(ic));
  if(size == 1) {
    emitpcode(POC_COMFW,popGet(AOP(IC_LEFT(ic)),0));
    emitpcode(POC_ANDLW,popGetLit(1));
    emitpcode(POC_MOVWF,popGet(AOP(IC_RESULT(ic)),0));
    goto release;
  }
  pic14_toBoolean(IC_LEFT(ic));

  tlbl = newiTempLabel(NULL);
  pic14_emitcode("cjne","a,#0x01,%05d_DS_",tlbl->key+100);
  pic14_emitcode("","%05d_DS_:",tlbl->key+100);
  pic14_outBitC(IC_RESULT(ic));

 release:    
  /* release the aops */
  freeAsmop(IC_LEFT(ic),NULL,ic,(RESULTONSTACK(ic) ? 0 : 1));
  freeAsmop(IC_RESULT(ic),NULL,ic,TRUE);
}


/*-----------------------------------------------------------------*/
/* genCpl - generate code for complement                           */
/*-----------------------------------------------------------------*/
static void genCpl (iCode *ic)
{
  operand *left, *result;
  int size, offset=0;  


  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
  aopOp((left = IC_LEFT(ic)),ic,FALSE);
  aopOp((result=IC_RESULT(ic)),ic,TRUE);

  /* if both are in bit space then 
     a special case */
  if (AOP_TYPE(result) == AOP_CRY &&
      AOP_TYPE(left) == AOP_CRY ) { 

    pic14_emitcode("mov","c,%s",left->aop->aopu.aop_dir); 
    pic14_emitcode("cpl","c"); 
    pic14_emitcode("mov","%s,c",result->aop->aopu.aop_dir); 
    goto release; 
  } 

  size = AOP_SIZE(result);
  while (size--) {

    if(AOP_TYPE(left) == AOP_ACC) 
      emitpcode(POC_XORLW, popGetLit(0xff));
    else
      emitpcode(POC_COMFW,popGet(AOP(left),offset));

    emitpcode(POC_MOVWF,popGet(AOP(result),offset));

  }


release:
    /* release the aops */
    freeAsmop(left,NULL,ic,(RESULTONSTACK(ic) ? 0 : 1));
    freeAsmop(result,NULL,ic,TRUE);
}

/*-----------------------------------------------------------------*/
/* genUminusFloat - unary minus for floating points                */
/*-----------------------------------------------------------------*/
static void genUminusFloat(operand *op,operand *result)
{
    int size ,offset =0 ;
    char *l;

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    /* for this we just need to flip the 
    first it then copy the rest in place */
    size = AOP_SIZE(op) - 1;
    l = aopGet(AOP(op),3,FALSE,FALSE);

    MOVA(l);    

    pic14_emitcode("cpl","acc.7");
    aopPut(AOP(result),"a",3);    

    while(size--) {
        aopPut(AOP(result),
               aopGet(AOP(op),offset,FALSE,FALSE),
               offset);
        offset++;
    }          
}

/*-----------------------------------------------------------------*/
/* genUminus - unary minus code generation                         */
/*-----------------------------------------------------------------*/
static void genUminus (iCode *ic)
{
  int size, i;
  sym_link *optype, *rtype;


  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
  /* assign asmops */
  aopOp(IC_LEFT(ic),ic,FALSE);
  aopOp(IC_RESULT(ic),ic,TRUE);

  /* if both in bit space then special
     case */
  if (AOP_TYPE(IC_RESULT(ic)) == AOP_CRY &&
      AOP_TYPE(IC_LEFT(ic)) == AOP_CRY ) { 

    emitpcode(POC_BCF,   popGet(AOP(IC_RESULT(ic)),0));
    emitpcode(POC_BTFSS, popGet(AOP(IC_LEFT(ic)),0));
    emitpcode(POC_BSF,   popGet(AOP(IC_RESULT(ic)),0));

    goto release; 
  } 

  optype = operandType(IC_LEFT(ic));
  rtype = operandType(IC_RESULT(ic));

  /* if float then do float stuff */
  if (IS_FLOAT(optype)) {
    genUminusFloat(IC_LEFT(ic),IC_RESULT(ic));
    goto release;
  }

  /* otherwise subtract from zero by taking the 2's complement */
  size = AOP_SIZE(IC_LEFT(ic));

  for(i=0; i<size; i++) {
    if (pic14_sameRegs(AOP(IC_LEFT(ic)), AOP(IC_RESULT(ic))) )
      emitpcode(POC_COMF,  popGet(AOP(IC_LEFT(ic)),i));
    else {
      emitpcode(POC_COMFW, popGet(AOP(IC_LEFT(ic)),i));
      emitpcode(POC_MOVWF, popGet(AOP(IC_RESULT(ic)),i));
    }
  }

  emitpcode(POC_INCF,  popGet(AOP(IC_RESULT(ic)),0));
  for(i=1; i<size; i++) {
    emitSKPNZ;
    emitpcode(POC_INCF,  popGet(AOP(IC_RESULT(ic)),i));
  }

 release:
  /* release the aops */
  freeAsmop(IC_LEFT(ic),NULL,ic,(RESULTONSTACK(ic) ? 0 : 1));
  freeAsmop(IC_RESULT(ic),NULL,ic,TRUE);    
}

/*-----------------------------------------------------------------*/
/* saveRegisters - will look for a call and save the registers     */
/*-----------------------------------------------------------------*/
static void saveRegisters(iCode *lic) 
{
    int i;
    iCode *ic;
    bitVect *rsave;
    sym_link *dtype;

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    /* look for call */
    for (ic = lic ; ic ; ic = ic->next) 
        if (ic->op == CALL || ic->op == PCALL)
            break;

    if (!ic) {
        fprintf(stderr,"found parameter push with no function call\n");
        return ;
    }

    /* if the registers have been saved already then
    do nothing */
    if (ic->regsSaved || IFFUNC_CALLEESAVES(OP_SYMBOL(IC_LEFT(ic))->type))
        return ;

    /* find the registers in use at this time 
    and push them away to safety */
    rsave = bitVectCplAnd(bitVectCopy(ic->rMask),
                          ic->rUsed);

    ic->regsSaved = 1;
    if (options.useXstack) {
	if (bitVectBitValue(rsave,R0_IDX))
	    pic14_emitcode("mov","b,r0");
	pic14_emitcode("mov","r0,%s",spname);
	for (i = 0 ; i < pic14_nRegs ; i++) {
	    if (bitVectBitValue(rsave,i)) {
		if (i == R0_IDX)
		    pic14_emitcode("mov","a,b");
		else
		    pic14_emitcode("mov","a,%s",pic14_regWithIdx(i)->name);
		pic14_emitcode("movx","@r0,a");
		pic14_emitcode("inc","r0");
	    }
	}
	pic14_emitcode("mov","%s,r0",spname);
	if (bitVectBitValue(rsave,R0_IDX))
	    pic14_emitcode("mov","r0,b");	    
    }// else
    //for (i = 0 ; i < pic14_nRegs ; i++) {
    //    if (bitVectBitValue(rsave,i))
    //	pic14_emitcode("push","%s",pic14_regWithIdx(i)->dname);
    //}

    dtype = operandType(IC_LEFT(ic));
    if (currFunc && dtype && 
        (FUNC_REGBANK(currFunc->type) != FUNC_REGBANK(dtype)) &&
	IFFUNC_ISISR(currFunc->type) &&
        !ic->bankSaved) 

        saverbank(FUNC_REGBANK(dtype),ic,TRUE);

}
/*-----------------------------------------------------------------*/
/* unsaveRegisters - pop the pushed registers                      */
/*-----------------------------------------------------------------*/
static void unsaveRegisters (iCode *ic)
{
    int i;
    bitVect *rsave;

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    /* find the registers in use at this time 
    and push them away to safety */
    rsave = bitVectCplAnd(bitVectCopy(ic->rMask),
                          ic->rUsed);
    
    if (options.useXstack) {
	pic14_emitcode("mov","r0,%s",spname);	
	for (i =  pic14_nRegs ; i >= 0 ; i--) {
	    if (bitVectBitValue(rsave,i)) {
		pic14_emitcode("dec","r0");
		pic14_emitcode("movx","a,@r0");
		if (i == R0_IDX)
		    pic14_emitcode("mov","b,a");
		else
		    pic14_emitcode("mov","%s,a",pic14_regWithIdx(i)->name);
	    }	    

	}
	pic14_emitcode("mov","%s,r0",spname);
	if (bitVectBitValue(rsave,R0_IDX))
	    pic14_emitcode("mov","r0,b");
    } //else
    //for (i =  pic14_nRegs ; i >= 0 ; i--) {
    //    if (bitVectBitValue(rsave,i))
    //	pic14_emitcode("pop","%s",pic14_regWithIdx(i)->dname);
    //}

}  


/*-----------------------------------------------------------------*/
/* pushSide -							   */
/*-----------------------------------------------------------------*/
static void pushSide(operand * oper, int size)
{
#if 0
	int offset = 0;
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
	while (size--) {
		char *l = aopGet(AOP(oper),offset++,FALSE,TRUE);
		if (AOP_TYPE(oper) != AOP_REG &&
		    AOP_TYPE(oper) != AOP_DIR &&
		    strcmp(l,"a") ) {
			pic14_emitcode("mov","a,%s",l);
			pic14_emitcode("push","acc");
		} else
			pic14_emitcode("push","%s",l);
	}
#endif
}

/*-----------------------------------------------------------------*/
/* assignResultValue -						   */
/*-----------------------------------------------------------------*/
static void assignResultValue(operand * oper)
{
  int size = AOP_SIZE(oper);

  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

  DEBUGpic14_AopType(__LINE__,oper,NULL,NULL);

  if(!GpsuedoStkPtr) {
    /* The last byte in the assignment is in W */
    size--;
    emitpcode(POC_MOVWF, popGet(AOP(oper),size));
    GpsuedoStkPtr++;
  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
  }

  while (size--) {
    emitpcode(POC_MOVFW,popRegFromIdx(GpsuedoStkPtr-1 + Gstack_base_addr));
    GpsuedoStkPtr++;
    emitpcode(POC_MOVWF, popGet(AOP(oper),size));
  }
}


/*-----------------------------------------------------------------*/
/* genIpush - genrate code for pushing this gets a little complex  */
/*-----------------------------------------------------------------*/
static void genIpush (iCode *ic)
{

  DEBUGpic14_emitcode ("; ***","%s  %d - WARNING no code generated",__FUNCTION__,__LINE__);
#if 0
    int size, offset = 0 ;
    char *l;


    /* if this is not a parm push : ie. it is spill push 
    and spill push is always done on the local stack */
    if (!ic->parmPush) {

        /* and the item is spilt then do nothing */
        if (OP_SYMBOL(IC_LEFT(ic))->isspilt)
            return ;

        aopOp(IC_LEFT(ic),ic,FALSE);
        size = AOP_SIZE(IC_LEFT(ic));
        /* push it on the stack */
        while(size--) {
            l = aopGet(AOP(IC_LEFT(ic)),offset++,FALSE,TRUE);
            if (*l == '#') {
                MOVA(l);
                l = "acc";
            }
            pic14_emitcode("push","%s",l);
        }
        return ;        
    }

    /* this is a paramter push: in this case we call
    the routine to find the call and save those
    registers that need to be saved */   
    saveRegisters(ic);

    /* then do the push */
    aopOp(IC_LEFT(ic),ic,FALSE);


	// pushSide(IC_LEFT(ic), AOP_SIZE(IC_LEFT(ic)));
    size = AOP_SIZE(IC_LEFT(ic));

    while (size--) {
        l = aopGet(AOP(IC_LEFT(ic)),offset++,FALSE,TRUE);
        if (AOP_TYPE(IC_LEFT(ic)) != AOP_REG && 
            AOP_TYPE(IC_LEFT(ic)) != AOP_DIR &&
            strcmp(l,"a") ) {
            pic14_emitcode("mov","a,%s",l);
            pic14_emitcode("push","acc");
        } else
            pic14_emitcode("push","%s",l);
    }       

    freeAsmop(IC_LEFT(ic),NULL,ic,TRUE);
#endif
}

/*-----------------------------------------------------------------*/
/* genIpop - recover the registers: can happen only for spilling   */
/*-----------------------------------------------------------------*/
static void genIpop (iCode *ic)
{
  DEBUGpic14_emitcode ("; ***","%s  %d - WARNING no code generated",__FUNCTION__,__LINE__);
#if 0
    int size,offset ;


    /* if the temp was not pushed then */
    if (OP_SYMBOL(IC_LEFT(ic))->isspilt)
        return ;

    aopOp(IC_LEFT(ic),ic,FALSE);
    size = AOP_SIZE(IC_LEFT(ic));
    offset = (size-1);
    while (size--) 
        pic14_emitcode("pop","%s",aopGet(AOP(IC_LEFT(ic)),offset--,
                                   FALSE,TRUE));

    freeAsmop(IC_LEFT(ic),NULL,ic,TRUE);
#endif
}

/*-----------------------------------------------------------------*/
/* unsaverbank - restores the resgister bank from stack            */
/*-----------------------------------------------------------------*/
static void unsaverbank (int bank,iCode *ic,bool popPsw)
{
  DEBUGpic14_emitcode ("; ***","%s  %d - WARNING no code generated",__FUNCTION__,__LINE__);
#if 0
    int i;
    asmop *aop ;
    regs *r = NULL;

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    if (popPsw) {
	if (options.useXstack) {
	    aop = newAsmop(0);
	    r = getFreePtr(ic,&aop,FALSE);
	    
	    
	    pic14_emitcode("mov","%s,_spx",r->name);
	    pic14_emitcode("movx","a,@%s",r->name);
	    pic14_emitcode("mov","psw,a");
	    pic14_emitcode("dec","%s",r->name);
	    
	}else
	    pic14_emitcode ("pop","psw");
    }

    for (i = (pic14_nRegs - 1) ; i >= 0 ;i--) {
        if (options.useXstack) {       
            pic14_emitcode("movx","a,@%s",r->name);
            //pic14_emitcode("mov","(%s+%d),a",
	    //       regspic14[i].base,8*bank+regspic14[i].offset);
            pic14_emitcode("dec","%s",r->name);

        } else 
	  pic14_emitcode("pop",""); //"(%s+%d)",
	//regspic14[i].base,8*bank); //+regspic14[i].offset);
    }

    if (options.useXstack) {

	pic14_emitcode("mov","_spx,%s",r->name);
	freeAsmop(NULL,aop,ic,TRUE);

    }
#endif 
}

/*-----------------------------------------------------------------*/
/* saverbank - saves an entire register bank on the stack          */
/*-----------------------------------------------------------------*/
static void saverbank (int bank, iCode *ic, bool pushPsw)
{
  DEBUGpic14_emitcode ("; ***","%s  %d - WARNING no code generated",__FUNCTION__,__LINE__);
#if 0
    int i;
    asmop *aop ;
    regs *r = NULL;

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    if (options.useXstack) {

	aop = newAsmop(0);
	r = getFreePtr(ic,&aop,FALSE);	
	pic14_emitcode("mov","%s,_spx",r->name);

    }

    for (i = 0 ; i < pic14_nRegs ;i++) {
        if (options.useXstack) {
            pic14_emitcode("inc","%s",r->name);
            //pic14_emitcode("mov","a,(%s+%d)",
            //         regspic14[i].base,8*bank+regspic14[i].offset);
            pic14_emitcode("movx","@%s,a",r->name);           
        } else 
	  pic14_emitcode("push","");// "(%s+%d)",
                     //regspic14[i].base,8*bank+regspic14[i].offset);
    }
    
    if (pushPsw) {
	if (options.useXstack) {
	    pic14_emitcode("mov","a,psw");
	    pic14_emitcode("movx","@%s,a",r->name);	
	    pic14_emitcode("inc","%s",r->name);
	    pic14_emitcode("mov","_spx,%s",r->name);       
	    freeAsmop (NULL,aop,ic,TRUE);
	    
	} else
	    pic14_emitcode("push","psw");
	
	pic14_emitcode("mov","psw,#0x%02x",(bank << 3)&0x00ff);
    }
    ic->bankSaved = 1;
#endif
}

/*-----------------------------------------------------------------*/
/* genCall - generates a call statement                            */
/*-----------------------------------------------------------------*/
static void genCall (iCode *ic)
{
  sym_link *dtype;   

  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

  /* if caller saves & we have not saved then */
  if (!ic->regsSaved)
    saveRegisters(ic);

  /* if we are calling a function that is not using
     the same register bank then we need to save the
     destination registers on the stack */
  dtype = operandType(IC_LEFT(ic));
  if (currFunc && dtype && 
      (FUNC_REGBANK(currFunc->type) != FUNC_REGBANK(dtype)) &&
      IFFUNC_ISISR(currFunc->type) &&
      !ic->bankSaved) 

    saverbank(FUNC_REGBANK(dtype),ic,TRUE);

  /* if send set is not empty the assign */
  if (_G.sendSet) {
    iCode *sic;
    /* For the Pic port, there is no data stack.
     * So parameters passed to functions are stored
     * in registers. (The pCode optimizer will get
     * rid of most of these :).
     */
    int psuedoStkPtr=-1; 
    int firstTimeThruLoop = 1;

    _G.sendSet = reverseSet(_G.sendSet);

    /* First figure how many parameters are getting passed */
    for (sic = setFirstItem(_G.sendSet) ; sic ; 
	 sic = setNextItem(_G.sendSet)) {

      aopOp(IC_LEFT(sic),sic,FALSE);
      psuedoStkPtr += AOP_SIZE(IC_LEFT(sic));
      freeAsmop (IC_LEFT(sic),NULL,sic,FALSE);
    }

    for (sic = setFirstItem(_G.sendSet) ; sic ; 
	 sic = setNextItem(_G.sendSet)) {
      int size, offset = 0;

      aopOp(IC_LEFT(sic),sic,FALSE);
      size = AOP_SIZE(IC_LEFT(sic));

      while (size--) {
	DEBUGpic14_emitcode ("; ","%d left %s",__LINE__,
			     AopType(AOP_TYPE(IC_LEFT(sic))));

	if(!firstTimeThruLoop) {
	  /* If this is not the first time we've been through the loop
	   * then we need to save the parameter in a temporary
	   * register. The last byte of the last parameter is
	   * passed in W. */
	  emitpcode(POC_MOVWF,popRegFromIdx(--psuedoStkPtr + Gstack_base_addr));

	}
	firstTimeThruLoop=0;

	//if (strcmp(l,fReturn[offset])) {
	mov2w (AOP(IC_LEFT(sic)),  offset);
/*
	if ( ((AOP(IC_LEFT(sic))->type) == AOP_PCODE) ||
	     ((AOP(IC_LEFT(sic))->type) == AOP_LIT) )
	  emitpcode(POC_MOVLW,popGet(AOP(IC_LEFT(sic)),offset));
	else
	  emitpcode(POC_MOVFW,popGet(AOP(IC_LEFT(sic)),offset));
*/
	//}
	offset++;
      }
      freeAsmop (IC_LEFT(sic),NULL,sic,TRUE);
    }
    _G.sendSet = NULL;
  }
  /* make the call */
  emitpcode(POC_CALL,popGetWithString(OP_SYMBOL(IC_LEFT(ic))->rname[0] ?
				      OP_SYMBOL(IC_LEFT(ic))->rname :
				      OP_SYMBOL(IC_LEFT(ic))->name));

  GpsuedoStkPtr=0;
  /* if we need assign a result value */
  if ((IS_ITEMP(IC_RESULT(ic)) && 
       (OP_SYMBOL(IC_RESULT(ic))->nRegs ||
	OP_SYMBOL(IC_RESULT(ic))->spildir )) ||
      IS_TRUE_SYMOP(IC_RESULT(ic)) ) {

    _G.accInUse++;
    aopOp(IC_RESULT(ic),ic,FALSE);
    _G.accInUse--;

    assignResultValue(IC_RESULT(ic));

    DEBUGpic14_emitcode ("; ","%d left %s",__LINE__,
			 AopType(AOP_TYPE(IC_RESULT(ic))));
		
    freeAsmop(IC_RESULT(ic),NULL, ic,TRUE);
  }

  /* adjust the stack for parameters if 
     required */
  if (ic->parmBytes) {
    int i;
    if (ic->parmBytes > 3) {
      pic14_emitcode("mov","a,%s",spname);
      pic14_emitcode("add","a,#0x%02x", (- ic->parmBytes) & 0xff);
      pic14_emitcode("mov","%s,a",spname);
    } else 
      for ( i = 0 ; i <  ic->parmBytes ;i++)
	pic14_emitcode("dec","%s",spname);

  }

  /* if register bank was saved then pop them */
  if (ic->bankSaved)
    unsaverbank(FUNC_REGBANK(dtype),ic,TRUE);

  /* if we hade saved some registers then unsave them */
  if (ic->regsSaved && !IFFUNC_CALLEESAVES(dtype))
    unsaveRegisters (ic);


}

/*-----------------------------------------------------------------*/
/* genPcall - generates a call by pointer statement                */
/*-----------------------------------------------------------------*/
static void genPcall (iCode *ic)
{
    sym_link *dtype;
    symbol *albl = newiTempLabel(NULL);
    symbol *blbl = newiTempLabel(NULL);
    PIC_OPCODE poc;
    operand *left;

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    /* if caller saves & we have not saved then */
    if (!ic->regsSaved)
        saveRegisters(ic);

    /* if we are calling a function that is not using
    the same register bank then we need to save the
    destination registers on the stack */
    dtype = operandType(IC_LEFT(ic));
    if (currFunc && dtype && 
	IFFUNC_ISISR(currFunc->type) &&
        (FUNC_REGBANK(currFunc->type) != FUNC_REGBANK(dtype)))
        saverbank(FUNC_REGBANK(dtype),ic,TRUE);

    left = IC_LEFT(ic);
    aopOp(left,ic,FALSE);
    DEBUGpic14_AopType(__LINE__,left,NULL,NULL);

    pushSide(IC_LEFT(ic), FPTRSIZE);

    /* if send set is not empty, assign parameters */
    if (_G.sendSet) {

	DEBUGpic14_emitcode ("; ***","%s  %d - WARNING arg-passing to indirect call not supported",__FUNCTION__,__LINE__);
	/* no way to pass args - W always gets used to make the call */
    }
/* first idea - factor out a common helper function and call it.
   But don't know how to get it generated only once in its own block

    if(AOP_TYPE(IC_LEFT(ic)) == AOP_DIR) {
	    char *rname;
	    char *buffer;
	    rname = IC_LEFT(ic)->aop->aopu.aop_dir;
	    DEBUGpic14_emitcode ("; ***","%s  %d AOP_DIR %s",__FUNCTION__,__LINE__,rname);
	    buffer = Safe_calloc(1,strlen(rname)+16);
	    sprintf(buffer, "%s_goto_helper", rname);
	    addpCode2pBlock(pb,newpCode(POC_CALL,newpCodeOp(buffer,PO_STR)));
	    free(buffer);
    }
*/
    emitpcode(POC_CALL,popGetLabel(albl->key));
    emitpcodePagesel(popGetLabel(blbl->key)->name); /* Must restore PCLATH before goto, without destroying W */
    emitpcode(POC_GOTO,popGetLabel(blbl->key));
    emitpLabel(albl->key);

    poc = ( (AOP_TYPE(left) == AOP_PCODE) ? POC_MOVLW : POC_MOVFW);
    
    emitpcode(poc,popGet(AOP(left),1));
    emitpcode(POC_MOVWF,popCopyReg(&pc_pclath));
    emitpcode(poc,popGet(AOP(left),0));
    emitpcode(POC_MOVWF,popCopyReg(&pc_pcl));

    emitpLabel(blbl->key);

    freeAsmop(IC_LEFT(ic),NULL,ic,TRUE); 

    /* if we need to assign a result value */
    if ((IS_ITEMP(IC_RESULT(ic)) &&
         (OP_SYMBOL(IC_RESULT(ic))->nRegs ||
          OP_SYMBOL(IC_RESULT(ic))->spildir)) ||
        IS_TRUE_SYMOP(IC_RESULT(ic)) ) {

        _G.accInUse++;
        aopOp(IC_RESULT(ic),ic,FALSE);
        _G.accInUse--;
	
	assignResultValue(IC_RESULT(ic));

        freeAsmop(IC_RESULT(ic),NULL,ic,TRUE);
    }

    /* if register bank was saved then unsave them */
    if (currFunc && dtype && 
        (FUNC_REGBANK(currFunc->type) != FUNC_REGBANK(dtype)))
        unsaverbank(FUNC_REGBANK(dtype),ic,TRUE);

    /* if we hade saved some registers then
    unsave them */
    if (ic->regsSaved)
        unsaveRegisters (ic);

}

/*-----------------------------------------------------------------*/
/* resultRemat - result  is rematerializable                       */
/*-----------------------------------------------------------------*/
static int resultRemat (iCode *ic)
{
  //    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
  if (SKIP_IC(ic) || ic->op == IFX)
    return 0;

  if (IC_RESULT(ic) && IS_ITEMP(IC_RESULT(ic))) {
    symbol *sym = OP_SYMBOL(IC_RESULT(ic));
    if (sym->remat && !POINTER_SET(ic)) 
      return 1;
  }

  return 0;
}

#if defined(__BORLANDC__) || defined(_MSC_VER)
#define STRCASECMP stricmp
#else
#define STRCASECMP strcasecmp
#endif

#if 0
/*-----------------------------------------------------------------*/
/* inExcludeList - return 1 if the string is in exclude Reg list   */
/*-----------------------------------------------------------------*/
static bool inExcludeList(char *s)
{
  DEBUGpic14_emitcode ("; ***","%s  %d - WARNING no code generated",__FUNCTION__,__LINE__);
    int i =0;
    
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    if (options.excludeRegs[i] &&
    STRCASECMP(options.excludeRegs[i],"none") == 0)
	return FALSE ;

    for ( i = 0 ; options.excludeRegs[i]; i++) {
	if (options.excludeRegs[i] &&
        STRCASECMP(s,options.excludeRegs[i]) == 0)
	    return TRUE;
    }
    return FALSE ;
}
#endif

/*-----------------------------------------------------------------*/
/* genFunction - generated code for function entry                 */
/*-----------------------------------------------------------------*/
static void genFunction (iCode *ic)
{
    symbol *sym;
    sym_link *ftype;

    DEBUGpic14_emitcode ("; ***","%s  %d curr label offset=%dprevious max_key=%d ",__FUNCTION__,__LINE__,labelOffset,max_key);

    labelOffset += (max_key+4);
    max_key=0;
    GpsuedoStkPtr=0;
    _G.nRegsSaved = 0;
    /* create the function header */
    pic14_emitcode(";","-----------------------------------------");
    pic14_emitcode(";"," function %s",(sym = OP_SYMBOL(IC_LEFT(ic)))->name);
    pic14_emitcode(";","-----------------------------------------");

    pic14_emitcode("","%s:",sym->rname);
    addpCode2pBlock(pb,newpCodeFunction(NULL,sym->rname));

    ftype = operandType(IC_LEFT(ic));

    /* if critical function then turn interrupts off */
    if (IFFUNC_ISCRITICAL(ftype))
        pic14_emitcode("clr","ea");

    /* here we need to generate the equates for the
       register bank if required */
#if 0
    if (FUNC_REGBANK(ftype) != rbank) {
        int i ;

        rbank = FUNC_REGBANK(ftype);
        for ( i = 0 ; i < pic14_nRegs ; i++ ) {
            if (strcmp(regspic14[i].base,"0") == 0)
                pic14_emitcode("","%s = 0x%02x",
                         regspic14[i].dname,
                         8*rbank+regspic14[i].offset);
            else
                pic14_emitcode ("","%s = %s + 0x%02x",
                          regspic14[i].dname,
                          regspic14[i].base,
                          8*rbank+regspic14[i].offset);
        }
    }
#endif

    /* if this is an interrupt service routine */
    if (IFFUNC_ISISR(sym->type)) {
/*  already done in pic14createInterruptVect() - delete me
      addpCode2pBlock(pb,newpCode(POC_GOTO,newpCodeOp("END_OF_INTERRUPT+1",PO_STR)));
      emitpcodeNULLop(POC_NOP);
      emitpcodeNULLop(POC_NOP);
      emitpcodeNULLop(POC_NOP);
*/
      emitpcode(POC_MOVWF,  popCopyReg(&pc_wsave));
      emitpcode(POC_SWAPFW, popCopyReg(&pc_status));
      emitpcode(POC_CLRF,   popCopyReg(&pc_status));
      emitpcode(POC_MOVWF,  popCopyReg(&pc_ssave));
      emitpcode(POC_MOVFW,  popCopyReg(&pc_pclath));
      emitpcode(POC_MOVWF,  popCopyReg(&pc_psave));
      emitpcode(POC_CLRF,   popCopyReg(&pc_pclath));/* durring an interrupt PCLATH must be cleared before a goto or call statement */

      pBlockConvert2ISR(pb);
#if 0  
	if (!inExcludeList("acc")) 	    
	    pic14_emitcode ("push","acc");	
	if (!inExcludeList("b"))
	    pic14_emitcode ("push","b");
	if (!inExcludeList("dpl"))
	    pic14_emitcode ("push","dpl");
	if (!inExcludeList("dph"))
	    pic14_emitcode ("push","dph");
	if (options.model == MODEL_FLAT24 && !inExcludeList("dpx"))
	{
	    pic14_emitcode ("push", "dpx");
	    /* Make sure we're using standard DPTR */
	    pic14_emitcode ("push", "dps");
	    pic14_emitcode ("mov", "dps, #0x00");
	    if (options.stack10bit)
	    {	
	    	/* This ISR could conceivably use DPTR2. Better save it. */
	    	pic14_emitcode ("push", "dpl1");
	    	pic14_emitcode ("push", "dph1");
	    	pic14_emitcode ("push", "dpx1");
	    }
	}
	/* if this isr has no bank i.e. is going to
	   run with bank 0 , then we need to save more
	   registers :-) */
	if (!FUNC_REGBANK(sym->type)) {

	    /* if this function does not call any other
	       function then we can be economical and
	       save only those registers that are used */
	    if (! IFFUNC_HASFCALL(sym->type)) {
		int i;

		/* if any registers used */
		if (sym->regsUsed) {
		    /* save the registers used */
		    for ( i = 0 ; i < sym->regsUsed->size ; i++) {
			if (bitVectBitValue(sym->regsUsed,i) ||
                          (pic14_ptrRegReq && (i == R0_IDX || i == R1_IDX)) )
			  pic14_emitcode("push","junk");//"%s",pic14_regWithIdx(i)->dname);			    
		    }
		}
		
	    } else {
		/* this function has  a function call cannot
		   determines register usage so we will have the
		   entire bank */
		saverbank(0,ic,FALSE);
	    }	    
	}
#endif
    } else {
	/* if callee-save to be used for this function
	   then save the registers being used in this function */
	if (IFFUNC_CALLEESAVES(sym->type)) {
	    int i;
	    
	    /* if any registers used */
	    if (sym->regsUsed) {
		/* save the registers used */
		for ( i = 0 ; i < sym->regsUsed->size ; i++) {
		    if (bitVectBitValue(sym->regsUsed,i) ||
                      (pic14_ptrRegReq && (i == R0_IDX || i == R1_IDX)) ) {
		      //pic14_emitcode("push","%s",pic14_regWithIdx(i)->dname);
			_G.nRegsSaved++;
		    }
		}
	    }
	}
    }

    /* set the register bank to the desired value */
    if (FUNC_REGBANK(sym->type) || FUNC_ISISR(sym->type)) {
        pic14_emitcode("push","psw");
        pic14_emitcode("mov","psw,#0x%02x",(FUNC_REGBANK(sym->type) << 3)&0x00ff);   
    }

    if (IFFUNC_ISREENT(sym->type) || options.stackAuto) {

	if (options.useXstack) {
	    pic14_emitcode("mov","r0,%s",spname);
	    pic14_emitcode("mov","a,_bp");
	    pic14_emitcode("movx","@r0,a");
	    pic14_emitcode("inc","%s",spname);
	}
	else
	{
	    /* set up the stack */
	    pic14_emitcode ("push","_bp");     /* save the callers stack  */
	}
	pic14_emitcode ("mov","_bp,%s",spname);
    }

    /* adjust the stack for the function */
    if (sym->stack) {

	int i = sym->stack;
	if (i > 256 ) 
	    werror(W_STACK_OVERFLOW,sym->name);

	if (i > 3 && sym->recvSize < 4) {	       

	    pic14_emitcode ("mov","a,sp");
	    pic14_emitcode ("add","a,#0x%02x",((char)sym->stack & 0xff));
	    pic14_emitcode ("mov","sp,a");
	   
	}
	else
	    while(i--)
		pic14_emitcode("inc","sp");
    }

     if (sym->xstack) {

	pic14_emitcode ("mov","a,_spx");
	pic14_emitcode ("add","a,#0x%02x",((char)sym->xstack & 0xff));
	pic14_emitcode ("mov","_spx,a");
    }    

}

/*-----------------------------------------------------------------*/
/* genEndFunction - generates epilogue for functions               */
/*-----------------------------------------------------------------*/
static void genEndFunction (iCode *ic)
{
    symbol *sym = OP_SYMBOL(IC_LEFT(ic));

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

    if (IFFUNC_ISREENT(sym->type) || options.stackAuto)
    {
        pic14_emitcode ("mov","%s,_bp",spname);
    }

    /* if use external stack but some variables were
    added to the local stack then decrement the
    local stack */
    if (options.useXstack && sym->stack) {      
        pic14_emitcode("mov","a,sp");
        pic14_emitcode("add","a,#0x%02x",((char)-sym->stack) & 0xff);
        pic14_emitcode("mov","sp,a");
    }


    if ((IFFUNC_ISREENT(sym->type) || options.stackAuto)) {
	if (options.useXstack) {
	    pic14_emitcode("mov","r0,%s",spname);
	    pic14_emitcode("movx","a,@r0");
	    pic14_emitcode("mov","_bp,a");
	    pic14_emitcode("dec","%s",spname);
	}
	else
	{
	    pic14_emitcode ("pop","_bp");
	}
    }

    /* restore the register bank  */    
    if (FUNC_REGBANK(sym->type) || FUNC_ISISR(sym->type))
        pic14_emitcode ("pop","psw");

    if (IFFUNC_ISISR(sym->type)) {

	/* now we need to restore the registers */
	/* if this isr has no bank i.e. is going to
	   run with bank 0 , then we need to save more
	   registers :-) */
	if (!FUNC_REGBANK(sym->type)) {
	    
	    /* if this function does not call any other
	       function then we can be economical and
	       save only those registers that are used */
	    if (! IFFUNC_HASFCALL(sym->type)) {
		int i;
		
		/* if any registers used */
		if (sym->regsUsed) {
		    /* save the registers used */
		    for ( i = sym->regsUsed->size ; i >= 0 ; i--) {
			if (bitVectBitValue(sym->regsUsed,i) ||
                          (pic14_ptrRegReq && (i == R0_IDX || i == R1_IDX)) )
			  pic14_emitcode("pop","junk");//"%s",pic14_regWithIdx(i)->dname);
		    }
		}
		
	    } else {
		/* this function has  a function call cannot
		   determines register usage so we will have the
		   entire bank */
		unsaverbank(0,ic,FALSE);
	    }	    
	}
#if 0
	if (options.model == MODEL_FLAT24 && !inExcludeList("dpx"))
	{
	    if (options.stack10bit)
	    {
	        pic14_emitcode ("pop", "dpx1");
	        pic14_emitcode ("pop", "dph1");
	        pic14_emitcode ("pop", "dpl1");
	    }	
	    pic14_emitcode ("pop", "dps");
	    pic14_emitcode ("pop", "dpx");
	}
	if (!inExcludeList("dph"))
	    pic14_emitcode ("pop","dph");
	if (!inExcludeList("dpl"))
	    pic14_emitcode ("pop","dpl");
	if (!inExcludeList("b"))
	    pic14_emitcode ("pop","b");
	if (!inExcludeList("acc"))
	    pic14_emitcode ("pop","acc");

        if (IFFUNC_ISCRITICAL(sym->type))
            pic14_emitcode("setb","ea");
#endif

	/* if debug then send end of function */
/* 	if (options.debug && currFunc) { */
	if (currFunc) {
	    _G.debugLine = 1;
	    pic14_emitcode(";","C$%s$%d$%d$%d ==.",
		     FileBaseName(ic->filename),currFunc->lastLine,
		     ic->level,ic->block); 
	    if (IS_STATIC(currFunc->etype))	    
		pic14_emitcode(";","XF%s$%s$0$0 ==.",moduleName,currFunc->name); 
	    else
		pic14_emitcode(";","XG$%s$0$0 ==.",currFunc->name);
	    _G.debugLine = 0;
	}

        pic14_emitcode ("reti","");
        emitpcode(POC_MOVFW,  popCopyReg(&pc_psave));
        emitpcode(POC_MOVWF,  popCopyReg(&pc_pclath));
        emitpcode(POC_CLRF,   popCopyReg(&pc_status));
        emitpcode(POC_SWAPFW, popCopyReg(&pc_ssave));
        emitpcode(POC_MOVWF,  popCopyReg(&pc_status));
        emitpcode(POC_SWAPF,  popCopyReg(&pc_wsave));
        emitpcode(POC_SWAPFW, popCopyReg(&pc_wsave));
        addpCode2pBlock(pb,newpCodeLabel("END_OF_INTERRUPT",-1));
        emitpcodeNULLop(POC_RETFIE);
    }
    else {
        if (IFFUNC_ISCRITICAL(sym->type))
            pic14_emitcode("setb","ea");
	
	if (IFFUNC_CALLEESAVES(sym->type)) {
	    int i;
	    
	    /* if any registers used */
	    if (sym->regsUsed) {
		/* save the registers used */
		for ( i = sym->regsUsed->size ; i >= 0 ; i--) {
		    if (bitVectBitValue(sym->regsUsed,i) ||
                      (pic14_ptrRegReq && (i == R0_IDX || i == R1_IDX)) )
		      pic14_emitcode("pop","junk");//"%s",pic14_regWithIdx(i)->dname);
		}
	    }
	    
	}

	/* if debug then send end of function */
	if (currFunc) {
	    _G.debugLine = 1;
	    pic14_emitcode(";","C$%s$%d$%d$%d ==.",
		     FileBaseName(ic->filename),currFunc->lastLine,
		     ic->level,ic->block); 
	    if (IS_STATIC(currFunc->etype))	    
		pic14_emitcode(";","XF%s$%s$0$0 ==.",moduleName,currFunc->name); 
	    else
		pic14_emitcode(";","XG$%s$0$0 ==.",currFunc->name);
	    _G.debugLine = 0;
	}

        pic14_emitcode ("return","");
	emitpcodeNULLop(POC_RETURN);

	/* Mark the end of a function */
	addpCode2pBlock(pb,newpCodeFunction(NULL,NULL));
    }

}

/*-----------------------------------------------------------------*/
/* genRet - generate code for return statement                     */
/*-----------------------------------------------------------------*/
static void genRet (iCode *ic)
{
  int size,offset = 0 , pushed = 0;
    
  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
  /* if we have no return value then
     just generate the "ret" */
  if (!IC_LEFT(ic)) 
    goto jumpret;       
    
  /* we have something to return then
     move the return value into place */
  aopOp(IC_LEFT(ic),ic,FALSE);
  size = AOP_SIZE(IC_LEFT(ic));
    
  while (size--) {
    char *l ;
    if (AOP_TYPE(IC_LEFT(ic)) == AOP_DPTR) {
      /* #NOCHANGE */
      l = aopGet(AOP(IC_LEFT(ic)),offset++,
		 FALSE,TRUE);
      pic14_emitcode("push","%s",l);
      pushed++;
    } else {
      l = aopGet(AOP(IC_LEFT(ic)),offset,
		 FALSE,FALSE);
      if (strcmp(fReturn[offset],l)) {
      if ((((AOP(IC_LEFT(ic))->type) == AOP_PCODE) && 
	   AOP(IC_LEFT(ic))->aopu.pcop->type == PO_IMMEDIATE) ||
	  ( (AOP(IC_LEFT(ic))->type) == AOP_IMMD) ||
	  ( (AOP(IC_LEFT(ic))->type) == AOP_LIT) ) {
	  emitpcode(POC_MOVLW, popGet(AOP(IC_LEFT(ic)),offset));
	}else {
	  emitpcode(POC_MOVFW, popGet(AOP(IC_LEFT(ic)),offset));
	}
	if(size) {
	  emitpcode(POC_MOVWF,popRegFromIdx(offset + Gstack_base_addr));
	}
	offset++;
      }
    }
  }    

  if (pushed) {
    while(pushed) {
      pushed--;
      if (strcmp(fReturn[pushed],"a"))
	pic14_emitcode("pop",fReturn[pushed]);
      else
	pic14_emitcode("pop","acc");
    }
  }
  freeAsmop (IC_LEFT(ic),NULL,ic,TRUE);
    
 jumpret:
  /* generate a jump to the return label
     if the next is not the return statement */
  if (!(ic->next && ic->next->op == LABEL &&
	IC_LABEL(ic->next) == returnLabel)) {
	
    emitpcode(POC_GOTO,popGetLabel(returnLabel->key));
    pic14_emitcode("goto","_%05d_DS_",returnLabel->key+100 + labelOffset);
  }
    
}

/*-----------------------------------------------------------------*/
/* genLabel - generates a label                                    */
/*-----------------------------------------------------------------*/
static void genLabel (iCode *ic)
{
    /* special case never generate */
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    if (IC_LABEL(ic) == entryLabel)
        return ;

    emitpLabel(IC_LABEL(ic)->key);
    pic14_emitcode("","_%05d_DS_:",(IC_LABEL(ic)->key+100 + labelOffset));
}

/*-----------------------------------------------------------------*/
/* genGoto - generates a goto                                      */
/*-----------------------------------------------------------------*/
//tsd
static void genGoto (iCode *ic)
{
  emitpcode(POC_GOTO,popGetLabel(IC_LABEL(ic)->key));
  pic14_emitcode ("goto","_%05d_DS_",(IC_LABEL(ic)->key+100)+labelOffset);
}


/*-----------------------------------------------------------------*/
/* genMultbits :- multiplication of bits                           */
/*-----------------------------------------------------------------*/
static void genMultbits (operand *left, 
                         operand *right, 
                         operand *result)
{
  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

  if(!pic14_sameRegs(AOP(result),AOP(right)))
    emitpcode(POC_BSF,  popGet(AOP(result),0));

  emitpcode(POC_BTFSC,popGet(AOP(right),0));
  emitpcode(POC_BTFSS,popGet(AOP(left),0));
  emitpcode(POC_BCF,  popGet(AOP(result),0));

}


/*-----------------------------------------------------------------*/
/* genMultOneByte : 8 bit multiplication & division                */
/*-----------------------------------------------------------------*/
static void genMultOneByte (operand *left,
                            operand *right,
                            operand *result)
{
  sym_link *opetype = operandType(result);

  // symbol *lbl ;
  int size,offset;

  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
  DEBUGpic14_AopType(__LINE__,left,right,result);
  DEBUGpic14_AopTypeSign(__LINE__,left,right,result);

  /* (if two literals, the value is computed before) */
  /* if one literal, literal on the right */
  if (AOP_TYPE(left) == AOP_LIT){
    operand *t = right;
    right = left;
    left = t;
  }

  size = AOP_SIZE(result);
  if(size == 1) {

    if (AOP_TYPE(right) == AOP_LIT){
      pic14_emitcode("multiply ","lit val:%s by variable %s and store in %s", 
		     aopGet(AOP(right),0,FALSE,FALSE), 
		     aopGet(AOP(left),0,FALSE,FALSE), 
		     aopGet(AOP(result),0,FALSE,FALSE));
      pic14_emitcode("call","genMultLit");
    } else {
      pic14_emitcode("multiply ","variable :%s by variable %s and store in %s", 
		     aopGet(AOP(right),0,FALSE,FALSE), 
		     aopGet(AOP(left),0,FALSE,FALSE), 
		     aopGet(AOP(result),0,FALSE,FALSE));
      pic14_emitcode("call","genMult8X8_8");

    }
    genMult8X8_8 (left, right,result);


    /* signed or unsigned */
    //pic14_emitcode("mov","b,%s", aopGet(AOP(right),0,FALSE,FALSE));
    //l = aopGet(AOP(left),0,FALSE,FALSE);
    //MOVA(l);       
    //pic14_emitcode("mul","ab");
    /* if result size = 1, mul signed = mul unsigned */
    //aopPut(AOP(result),"a",0);

  } else {  // (size > 1)

    pic14_emitcode("multiply (size>1) ","variable :%s by variable %s and store in %s", 
		   aopGet(AOP(right),0,FALSE,FALSE), 
		   aopGet(AOP(left),0,FALSE,FALSE), 
		   aopGet(AOP(result),0,FALSE,FALSE));

    if (SPEC_USIGN(opetype)){
      pic14_emitcode("multiply ","unsigned result. size = %d",AOP_SIZE(result));
      genUMult8X8_16 (left, right, result, NULL);

      if (size > 2) {
	/* for filling the MSBs */
	emitpcode(POC_CLRF,  popGet(AOP(result),2));
	emitpcode(POC_CLRF,  popGet(AOP(result),3));
      }
    }
    else{
      pic14_emitcode("multiply ","signed result. size = %d",AOP_SIZE(result));

      pic14_emitcode("mov","a,b");

      /* adjust the MSB if left or right neg */

      /* if one literal */
      if (AOP_TYPE(right) == AOP_LIT){
	pic14_emitcode("multiply ","right is a lit");
	/* AND literal negative */
	if((int) floatFromVal (AOP(right)->aopu.aop_lit) < 0){
	  /* adjust MSB (c==0 after mul) */
	  pic14_emitcode("subb","a,%s", aopGet(AOP(left),0,FALSE,FALSE));
	}
      }
      else{
	genSMult8X8_16 (left, right, result, NULL);
      }

      if(size > 2){
	pic14_emitcode("multiply ","size is greater than 2, so propogate sign");
	/* get the sign */
	pic14_emitcode("rlc","a");
	pic14_emitcode("subb","a,acc");
      }
    }

    size -= 2;   
    offset = 2;
    if (size > 0)
      while (size--)
	pic14_emitcode("multiply ","size is way greater than 2, so propogate sign");
    //aopPut(AOP(result),"a",offset++);
  }
}

/*-----------------------------------------------------------------*/
/* genMult - generates code for multiplication                     */
/*-----------------------------------------------------------------*/
static void genMult (iCode *ic)
{
    operand *left = IC_LEFT(ic);
    operand *right = IC_RIGHT(ic);
    operand *result= IC_RESULT(ic);   

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    /* assign the amsops */
    aopOp (left,ic,FALSE);
    aopOp (right,ic,FALSE);
    aopOp (result,ic,TRUE);

  DEBUGpic14_AopType(__LINE__,left,right,result);

    /* special cases first */
    /* both are bits */
    if (AOP_TYPE(left) == AOP_CRY &&
        AOP_TYPE(right)== AOP_CRY) {
        genMultbits(left,right,result);
        goto release ;
    }

    /* if both are of size == 1 */
    if (AOP_SIZE(left) == 1 &&
        AOP_SIZE(right) == 1 ) {
        genMultOneByte(left,right,result);
        goto release ;
    }

    pic14_emitcode("multiply ","sizes are greater than 2... need to insert proper algor.");

    /* should have been converted to function call */
    //assert(0) ;

release :
    freeAsmop(left,NULL,ic,(RESULTONSTACK(ic) ? FALSE : TRUE));
    freeAsmop(right,NULL,ic,(RESULTONSTACK(ic) ? FALSE : TRUE));
    freeAsmop(result,NULL,ic,TRUE); 
}

/*-----------------------------------------------------------------*/
/* genDivbits :- division of bits                                  */
/*-----------------------------------------------------------------*/
static void genDivbits (operand *left, 
                        operand *right, 
                        operand *result)
{

    char *l;

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    /* the result must be bit */    
    pic14_emitcode("mov","b,%s",aopGet(AOP(right),0,FALSE,FALSE));
    l = aopGet(AOP(left),0,FALSE,FALSE);

    MOVA(l);    

    pic14_emitcode("div","ab");
    pic14_emitcode("rrc","a");
    aopPut(AOP(result),"c",0);
}

/*-----------------------------------------------------------------*/
/* genDivOneByte : 8 bit division                                  */
/*-----------------------------------------------------------------*/
static void genDivOneByte (operand *left,
                           operand *right,
                           operand *result)
{
    sym_link *opetype = operandType(result);
    char *l ;
    symbol *lbl ;
    int size,offset;

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    size = AOP_SIZE(result) - 1;
    offset = 1;
    /* signed or unsigned */
    if (SPEC_USIGN(opetype)) {
        /* unsigned is easy */
        pic14_emitcode("mov","b,%s", aopGet(AOP(right),0,FALSE,FALSE));
        l = aopGet(AOP(left),0,FALSE,FALSE);
        MOVA(l);        
        pic14_emitcode("div","ab");
        aopPut(AOP(result),"a",0);
        while (size--)
            aopPut(AOP(result),zero,offset++);
        return ;
    }

    /* signed is a little bit more difficult */

    /* save the signs of the operands */
    l = aopGet(AOP(left),0,FALSE,FALSE);    
    MOVA(l);    
    pic14_emitcode("xrl","a,%s",aopGet(AOP(right),0,FALSE,TRUE));
    pic14_emitcode("push","acc"); /* save it on the stack */

    /* now sign adjust for both left & right */
    l =  aopGet(AOP(right),0,FALSE,FALSE);    
    MOVA(l);       
    lbl = newiTempLabel(NULL);
    pic14_emitcode("jnb","acc.7,%05d_DS_",(lbl->key+100));   
    pic14_emitcode("cpl","a");   
    pic14_emitcode("inc","a");
    pic14_emitcode("","%05d_DS_:",(lbl->key+100));
    pic14_emitcode("mov","b,a");

    /* sign adjust left side */
    l =  aopGet(AOP(left),0,FALSE,FALSE);    
    MOVA(l);

    lbl = newiTempLabel(NULL);
    pic14_emitcode("jnb","acc.7,%05d_DS_",(lbl->key+100));
    pic14_emitcode("cpl","a");
    pic14_emitcode("inc","a");
    pic14_emitcode("","%05d_DS_:",(lbl->key+100));

    /* now the division */
    pic14_emitcode("div","ab");
    /* we are interested in the lower order
    only */
    pic14_emitcode("mov","b,a");
    lbl = newiTempLabel(NULL);
    pic14_emitcode("pop","acc");   
    /* if there was an over flow we don't 
    adjust the sign of the result */
    pic14_emitcode("jb","ov,%05d_DS_",(lbl->key+100));
    pic14_emitcode("jnb","acc.7,%05d_DS_",(lbl->key+100));
    CLRC;
    pic14_emitcode("clr","a");
    pic14_emitcode("subb","a,b");
    pic14_emitcode("mov","b,a");
    pic14_emitcode("","%05d_DS_:",(lbl->key+100));

    /* now we are done */
    aopPut(AOP(result),"b",0);
    if(size > 0){
        pic14_emitcode("mov","c,b.7");
        pic14_emitcode("subb","a,acc");   
    }
    while (size--)
        aopPut(AOP(result),"a",offset++);

}

/*-----------------------------------------------------------------*/
/* genDiv - generates code for division                            */
/*-----------------------------------------------------------------*/
static void genDiv (iCode *ic)
{
    operand *left = IC_LEFT(ic);
    operand *right = IC_RIGHT(ic);
    operand *result= IC_RESULT(ic);   

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    /* assign the amsops */
    aopOp (left,ic,FALSE);
    aopOp (right,ic,FALSE);
    aopOp (result,ic,TRUE);

    /* special cases first */
    /* both are bits */
    if (AOP_TYPE(left) == AOP_CRY &&
        AOP_TYPE(right)== AOP_CRY) {
        genDivbits(left,right,result);
        goto release ;
    }

    /* if both are of size == 1 */
    if (AOP_SIZE(left) == 1 &&
        AOP_SIZE(right) == 1 ) {
        genDivOneByte(left,right,result);
        goto release ;
    }

    /* should have been converted to function call */
    assert(0);
release :
    freeAsmop(left,NULL,ic,(RESULTONSTACK(ic) ? FALSE : TRUE));
    freeAsmop(right,NULL,ic,(RESULTONSTACK(ic) ? FALSE : TRUE));
    freeAsmop(result,NULL,ic,TRUE); 
}

/*-----------------------------------------------------------------*/
/* genModbits :- modulus of bits                                   */
/*-----------------------------------------------------------------*/
static void genModbits (operand *left, 
                        operand *right, 
                        operand *result)
{

    char *l;

    /* the result must be bit */    
    pic14_emitcode("mov","b,%s",aopGet(AOP(right),0,FALSE,FALSE));
    l = aopGet(AOP(left),0,FALSE,FALSE);

    MOVA(l);       

    pic14_emitcode("div","ab");
    pic14_emitcode("mov","a,b");
    pic14_emitcode("rrc","a");
    aopPut(AOP(result),"c",0);
}

/*-----------------------------------------------------------------*/
/* genModOneByte : 8 bit modulus                                   */
/*-----------------------------------------------------------------*/
static void genModOneByte (operand *left,
                           operand *right,
                           operand *result)
{
    sym_link *opetype = operandType(result);
    char *l ;
    symbol *lbl ;

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    /* signed or unsigned */
    if (SPEC_USIGN(opetype)) {
        /* unsigned is easy */
        pic14_emitcode("mov","b,%s", aopGet(AOP(right),0,FALSE,FALSE));
        l = aopGet(AOP(left),0,FALSE,FALSE);
        MOVA(l);    
        pic14_emitcode("div","ab");
        aopPut(AOP(result),"b",0);
        return ;
    }

    /* signed is a little bit more difficult */

    /* save the signs of the operands */
    l = aopGet(AOP(left),0,FALSE,FALSE);    
    MOVA(l);

    pic14_emitcode("xrl","a,%s",aopGet(AOP(right),0,FALSE,FALSE));
    pic14_emitcode("push","acc"); /* save it on the stack */

    /* now sign adjust for both left & right */
    l =  aopGet(AOP(right),0,FALSE,FALSE);    
    MOVA(l);

    lbl = newiTempLabel(NULL);
    pic14_emitcode("jnb","acc.7,%05d_DS_",(lbl->key+100));  
    pic14_emitcode("cpl","a");   
    pic14_emitcode("inc","a");
    pic14_emitcode("","%05d_DS_:",(lbl->key+100));
    pic14_emitcode("mov","b,a"); 

    /* sign adjust left side */
    l =  aopGet(AOP(left),0,FALSE,FALSE);    
    MOVA(l);

    lbl = newiTempLabel(NULL);
    pic14_emitcode("jnb","acc.7,%05d_DS_",(lbl->key+100));
    pic14_emitcode("cpl","a");   
    pic14_emitcode("inc","a");
    pic14_emitcode("","%05d_DS_:",(lbl->key+100));

    /* now the multiplication */
    pic14_emitcode("div","ab");
    /* we are interested in the lower order
    only */
    lbl = newiTempLabel(NULL);
    pic14_emitcode("pop","acc");   
    /* if there was an over flow we don't 
    adjust the sign of the result */
    pic14_emitcode("jb","ov,%05d_DS_",(lbl->key+100));
    pic14_emitcode("jnb","acc.7,%05d_DS_",(lbl->key+100));
    CLRC ;
    pic14_emitcode("clr","a");
    pic14_emitcode("subb","a,b");
    pic14_emitcode("mov","b,a");
    pic14_emitcode("","%05d_DS_:",(lbl->key+100));

    /* now we are done */
    aopPut(AOP(result),"b",0);

}

/*-----------------------------------------------------------------*/
/* genMod - generates code for division                            */
/*-----------------------------------------------------------------*/
static void genMod (iCode *ic)
{
    operand *left = IC_LEFT(ic);
    operand *right = IC_RIGHT(ic);
    operand *result= IC_RESULT(ic);  

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    /* assign the amsops */
    aopOp (left,ic,FALSE);
    aopOp (right,ic,FALSE);
    aopOp (result,ic,TRUE);

    /* special cases first */
    /* both are bits */
    if (AOP_TYPE(left) == AOP_CRY &&
        AOP_TYPE(right)== AOP_CRY) {
        genModbits(left,right,result);
        goto release ;
    }

    /* if both are of size == 1 */
    if (AOP_SIZE(left) == 1 &&
        AOP_SIZE(right) == 1 ) {
        genModOneByte(left,right,result);
        goto release ;
    }

    /* should have been converted to function call */
    assert(0);

release :
    freeAsmop(left,NULL,ic,(RESULTONSTACK(ic) ? FALSE : TRUE));
    freeAsmop(right,NULL,ic,(RESULTONSTACK(ic) ? FALSE : TRUE));
    freeAsmop(result,NULL,ic,TRUE); 
}

/*-----------------------------------------------------------------*/
/* genIfxJump :- will create a jump depending on the ifx           */
/*-----------------------------------------------------------------*/
/*
  note: May need to add parameter to indicate when a variable is in bit space.
*/
static void genIfxJump (iCode *ic, char *jval)
{

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    /* if true label then we jump if condition
    supplied is true */
    if ( IC_TRUE(ic) ) {

	if(strcmp(jval,"a") == 0)
	  emitSKPZ;
	else if (strcmp(jval,"c") == 0)
	  emitSKPC;
	else {
	  DEBUGpic14_emitcode ("; ***","%d - assuming %s is in bit space",__LINE__,jval);	  
	  emitpcode(POC_BTFSC,  newpCodeOpBit(jval,-1,1));
	}

	emitpcode(POC_GOTO,popGetLabel(IC_TRUE(ic)->key));
	pic14_emitcode(" goto","_%05d_DS_",IC_TRUE(ic)->key+100 + labelOffset);

    }
    else {
        /* false label is present */
	if(strcmp(jval,"a") == 0)
	  emitSKPNZ;
	else if (strcmp(jval,"c") == 0)
	  emitSKPNC;
	else {
	  DEBUGpic14_emitcode ("; ***","%d - assuming %s is in bit space",__LINE__,jval);	  
	  emitpcode(POC_BTFSS,  newpCodeOpBit(jval,-1,1));
	}

	emitpcode(POC_GOTO,popGetLabel(IC_FALSE(ic)->key));
	pic14_emitcode(" goto","_%05d_DS_",IC_FALSE(ic)->key+100 + labelOffset);

    }


    /* mark the icode as generated */
    ic->generated = 1;
}

/*-----------------------------------------------------------------*/
/* genSkip                                                         */
/*-----------------------------------------------------------------*/
static void genSkip(iCode *ifx,int status_bit)
{
  if(!ifx)
    return;

  if ( IC_TRUE(ifx) ) {
    switch(status_bit) {
    case 'z':
      emitSKPNZ;
      break;

    case 'c':
      emitSKPNC;
      break;

    case 'd':
      emitSKPDC;
      break;

    }

    emitpcode(POC_GOTO,popGetLabel(IC_TRUE(ifx)->key));
    pic14_emitcode("goto","_%05d_DS_",IC_TRUE(ifx)->key+100+labelOffset);

  } else {

    switch(status_bit) {

    case 'z':
      emitSKPZ;
      break;

    case 'c':
      emitSKPC;
      break;

    case 'd':
      emitSKPDC;
      break;
    }
    emitpcode(POC_GOTO,popGetLabel(IC_FALSE(ifx)->key));
    pic14_emitcode("goto","_%05d_DS_",IC_FALSE(ifx)->key+100+labelOffset);

  }

}

/*-----------------------------------------------------------------*/
/* genSkipc                                                        */
/*-----------------------------------------------------------------*/
static void genSkipc(resolvedIfx *rifx)
{
  if(!rifx)
    return;

  if(rifx->condition)
    emitSKPC;
  else
    emitSKPNC;

  emitpcode(POC_GOTO,popGetLabel(rifx->lbl->key));
  rifx->generated = 1;
}

/*-----------------------------------------------------------------*/
/* genSkipz2                                                       */
/*-----------------------------------------------------------------*/
static void genSkipz2(resolvedIfx *rifx, int invert_condition)
{
  if(!rifx)
    return;

  if( (rifx->condition ^ invert_condition) & 1)
    emitSKPZ;
  else
    emitSKPNZ;

  emitpcode(POC_GOTO,popGetLabel(rifx->lbl->key));
  rifx->generated = 1;
}

/*-----------------------------------------------------------------*/
/* genSkipz                                                        */
/*-----------------------------------------------------------------*/
static void genSkipz(iCode *ifx, int condition)
{
  if(!ifx)
    return;

  if(condition)
    emitSKPNZ;
  else
    emitSKPZ;

  if ( IC_TRUE(ifx) )
    emitpcode(POC_GOTO,popGetLabel(IC_TRUE(ifx)->key));
  else
    emitpcode(POC_GOTO,popGetLabel(IC_FALSE(ifx)->key));

  if ( IC_TRUE(ifx) )
    pic14_emitcode("goto","_%05d_DS_",IC_TRUE(ifx)->key+100+labelOffset);
  else
    pic14_emitcode("goto","_%05d_DS_",IC_FALSE(ifx)->key+100+labelOffset);

}
/*-----------------------------------------------------------------*/
/* genSkipCond                                                     */
/*-----------------------------------------------------------------*/
static void genSkipCond(resolvedIfx *rifx,operand *op, int offset, int bit)
{
  if(!rifx)
    return;

  if(rifx->condition)
    emitpcode(POC_BTFSC, newpCodeOpBit(aopGet(AOP(op),offset,FALSE,FALSE),bit,0));
  else
    emitpcode(POC_BTFSS, newpCodeOpBit(aopGet(AOP(op),offset,FALSE,FALSE),bit,0));


  emitpcode(POC_GOTO,popGetLabel(rifx->lbl->key));
  rifx->generated = 1;
}

#if 0
/*-----------------------------------------------------------------*/
/* genChkZeroes :- greater or less than comparison                 */
/*     For each byte in a literal that is zero, inclusive or the   */
/*     the corresponding byte in the operand with W                */
/*     returns true if any of the bytes are zero                   */
/*-----------------------------------------------------------------*/
static int genChkZeroes(operand *op, int lit,  int size)
{

  int i;
  int flag =1;

  while(size--) {
    i = (lit >> (size*8)) & 0xff;

    if(i==0) {
      if(flag) 
	emitpcode(POC_MOVFW, popGet(AOP(op),size));
      else
	emitpcode(POC_IORFW, popGet(AOP(op),size));
      flag = 0;
    }
  }

  return (flag==0);
}
#endif

/*-----------------------------------------------------------------*/
/* genCmp :- greater or less than comparison                       */
/*-----------------------------------------------------------------*/
static void genCmp (operand *left,operand *right,
                    operand *result, iCode *ifx, int sign)
{
  int size; //, offset = 0 ;
  unsigned long lit = 0L,i = 0;
  resolvedIfx rFalseIfx;
  //  resolvedIfx rTrueIfx;
  symbol *truelbl;
  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
/*
  if(ifx) {
    DEBUGpic14_emitcode ("; ***","true ifx is %s",((IC_TRUE(ifx) == NULL) ? "false" : "true"));
    DEBUGpic14_emitcode ("; ***","false ifx is %s",((IC_FALSE(ifx) == NULL) ? "false" : "true"));
  }
*/

  resolveIfx(&rFalseIfx,ifx);
  truelbl  = newiTempLabel(NULL);
  size = max(AOP_SIZE(left),AOP_SIZE(right));

  DEBUGpic14_AopType(__LINE__,left,right,result);

#define _swapp

  /* if literal is on the right then swap with left */
  if ((AOP_TYPE(right) == AOP_LIT)) {
    operand *tmp = right ;
    unsigned long mask = (0x100 << (8*(size-1))) - 1;
    lit = (unsigned long)floatFromVal(AOP(right)->aopu.aop_lit);
#ifdef _swapp

    lit = (lit - 1) & mask;
    right = left;
    left = tmp;
    rFalseIfx.condition ^= 1;
#endif

  } else if ((AOP_TYPE(left) == AOP_LIT)) {
    lit = (unsigned long)floatFromVal(AOP(left)->aopu.aop_lit);
  }


  //if(IC_TRUE(ifx) == NULL)
  /* if left & right are bit variables */
  if (AOP_TYPE(left) == AOP_CRY &&
      AOP_TYPE(right) == AOP_CRY ) {
    pic14_emitcode("mov","c,%s",AOP(right)->aopu.aop_dir);
    pic14_emitcode("anl","c,/%s",AOP(left)->aopu.aop_dir);
  } else {
    /* subtract right from left if at the
       end the carry flag is set then we know that
       left is greater than right */

    //    {

    symbol *lbl  = newiTempLabel(NULL);

#ifndef _swapp
    if(AOP_TYPE(right) == AOP_LIT) {

      //lit = (unsigned long)floatFromVal(AOP(right)->aopu.aop_lit);

      DEBUGpic14_emitcode(";right lit","lit = 0x%x,sign=%d",lit,sign);

      /* special cases */

      if(lit == 0) {

	if(sign != 0) 
	  genSkipCond(&rFalseIfx,left,size-1,7);
	else 
	  /* no need to compare to 0...*/
	  /* NOTE: this is a de-generate compare that most certainly 
	   *       creates some dead code. */
	  emitpcode(POC_GOTO,popGetLabel(rFalseIfx.lbl->key));

	if(ifx) ifx->generated = 1;
	return;

      }
      size--;

      if(size == 0) {
	//i = (lit >> (size*8)) & 0xff;
	DEBUGpic14_emitcode(";right lit","line = %d",__LINE__);
	
	emitpcode(POC_MOVFW, popGet(AOP(left),size));

	i = ((0-lit) & 0xff);
	if(sign) {
	  if( i == 0x81) { 
	    /* lit is 0x7f, all signed chars are less than
	     * this except for 0x7f itself */
	    emitpcode(POC_XORLW, popGetLit(0x7f));
	    genSkipz2(&rFalseIfx,0);
	  } else {
	    emitpcode(POC_ADDLW, popGetLit(0x80));
	    emitpcode(POC_ADDLW, popGetLit(i^0x80));
	    genSkipc(&rFalseIfx);
	  }

	} else {
	  if(lit == 1) {
	    genSkipz2(&rFalseIfx,1);
	  } else {
	    emitpcode(POC_ADDLW, popGetLit(i));
	    genSkipc(&rFalseIfx);
	  }
	}

	if(ifx) ifx->generated = 1;
	return;
      }

      /* chars are out of the way. now do ints and longs */


      DEBUGpic14_emitcode(";right lit","line = %d",__LINE__);
	
      /* special cases */

      if(sign) {

	if(lit == 0) {
	  genSkipCond(&rFalseIfx,left,size,7);
	  if(ifx) ifx->generated = 1;
	  return;
	}

	if(lit <0x100) {
	  DEBUGpic14_emitcode(";right lit","line = %d signed compare to 0x%x",__LINE__,lit);

	  //rFalseIfx.condition ^= 1;
	  //genSkipCond(&rFalseIfx,left,size,7);
	  //rFalseIfx.condition ^= 1;

	  emitpcode(POC_BTFSC, newpCodeOpBit(aopGet(AOP(left),size,FALSE,FALSE),7,0));
	  if(rFalseIfx.condition)
	    emitpcode(POC_GOTO,  popGetLabel(rFalseIfx.lbl->key));
	  else
	    emitpcode(POC_GOTO,  popGetLabel(truelbl->key));

	  emitpcode(POC_MOVLW, popGetLit(0x100-lit));
	  emitpcode(POC_ADDFW, popGet(AOP(left),0));
	  emitpcode(POC_MOVFW, popGet(AOP(left),1));

	  while(size > 1)
	    emitpcode(POC_IORFW, popGet(AOP(left),size--));

	  if(rFalseIfx.condition) {
	    emitSKPZ;
	    emitpcode(POC_GOTO,  popGetLabel(truelbl->key));

	  } else {
	    emitSKPNZ;
	  }

	  genSkipc(&rFalseIfx);
	  emitpLabel(truelbl->key);
	  if(ifx) ifx->generated = 1;
	  return;

	}

	if(size == 1) {

	  if( (lit & 0xff) == 0) {
	    /* lower byte is zero */
	    DEBUGpic14_emitcode(";right lit","line = %d signed compare to 0x%x",__LINE__,lit);
	    i = ((lit >> 8) & 0xff) ^0x80;
	    emitpcode(POC_MOVFW, popGet(AOP(left),size));
	    emitpcode(POC_ADDLW, popGetLit( 0x80));
	    emitpcode(POC_ADDLW, popGetLit(0x100-i));
	    genSkipc(&rFalseIfx);


	    if(ifx) ifx->generated = 1;
	    return;

	  }
	} else {
	  /* Special cases for signed longs */
	  if( (lit & 0xffffff) == 0) {
	    /* lower byte is zero */
	    DEBUGpic14_emitcode(";right lit","line = %d signed compare to 0x%x",__LINE__,lit);
	    i = ((lit >> 8*3) & 0xff) ^0x80;
	    emitpcode(POC_MOVFW, popGet(AOP(left),size));
	    emitpcode(POC_ADDLW, popGetLit( 0x80));
	    emitpcode(POC_ADDLW, popGetLit(0x100-i));
	    genSkipc(&rFalseIfx);


	    if(ifx) ifx->generated = 1;
	    return;

	  }

	}


	if(lit & (0x80 << (size*8))) {
	  /* lit is negative */
	  DEBUGpic14_emitcode(";right lit","line = %d signed compare to 0x%x",__LINE__,lit);

	  //genSkipCond(&rFalseIfx,left,size,7);

	  emitpcode(POC_BTFSS, newpCodeOpBit(aopGet(AOP(left),size,FALSE,FALSE),7,0));

	  if(rFalseIfx.condition)
	    emitpcode(POC_GOTO,  popGetLabel(truelbl->key));
	  else
	    emitpcode(POC_GOTO,  popGetLabel(rFalseIfx.lbl->key));


	} else {
	  /* lit is positive */
	  DEBUGpic14_emitcode(";right lit","line = %d signed compare to 0x%x",__LINE__,lit);
	  emitpcode(POC_BTFSC, newpCodeOpBit(aopGet(AOP(left),size,FALSE,FALSE),7,0));
	  if(rFalseIfx.condition)
	    emitpcode(POC_GOTO,  popGetLabel(rFalseIfx.lbl->key));
	  else
	    emitpcode(POC_GOTO,  popGetLabel(truelbl->key));

	}

	/*
	  This works, but is only good for ints.
	  It also requires a "known zero" register.
	  emitpcode(POC_MOVLW, popGetLit(mlit & 0xff));
	  emitpcode(POC_ADDFW, popGet(AOP(left),0));
	  emitpcode(POC_RLFW,  popCopyReg(&pc_kzero));
	  emitpcode(POC_ADDLW, popGetLit( ((mlit>>8) & 0xff)));
	  emitpcode(POC_ADDFW, popGet(AOP(left),1));
	  genSkipc(&rFalseIfx);

	  emitpLabel(truelbl->key);
	  if(ifx) ifx->generated = 1;
	  return;
	**/
	  
	/* There are no more special cases, so perform a general compare */
  
	emitpcode(POC_MOVLW, popGetLit((lit >> (size*8)) & 0xff));
	emitpcode(POC_SUBFW, popGet(AOP(left),size));

	while(size--) {

	  emitpcode(POC_MOVLW, popGetLit((lit >> (size*8)) & 0xff));
	  emitSKPNZ;
	  emitpcode(POC_SUBFW, popGet(AOP(left),size));
	}
	//rFalseIfx.condition ^= 1;
	genSkipc(&rFalseIfx);

	emitpLabel(truelbl->key);

	if(ifx) ifx->generated = 1;
	return;


      }


      /* sign is out of the way. So now do an unsigned compare */
      DEBUGpic14_emitcode(";right lit","line = %d unsigned compare to 0x%x",__LINE__,lit);


      /* General case - compare to an unsigned literal on the right.*/

      i = (lit >> (size*8)) & 0xff;
      emitpcode(POC_MOVLW, popGetLit(i));
      emitpcode(POC_SUBFW, popGet(AOP(left),size));
      while(size--) {
	i = (lit >> (size*8)) & 0xff;

	if(i) {
	  emitpcode(POC_MOVLW, popGetLit(i));
	  emitSKPNZ;
	  emitpcode(POC_SUBFW, popGet(AOP(left),size));
	} else {
	  /* this byte of the lit is zero, 
	   *if it's not the last then OR in the variable */
	  if(size)
	    emitpcode(POC_IORFW, popGet(AOP(left),size));
	}
      }


      emitpLabel(lbl->key);
      //if(emitFinalCheck)
      genSkipc(&rFalseIfx);
      if(sign)
	emitpLabel(truelbl->key);

      if(ifx) ifx->generated = 1;
      return;


    }
#endif  // _swapp

    if(AOP_TYPE(left) == AOP_LIT) {
      //symbol *lbl = newiTempLabel(NULL);

      //EXPERIMENTAL lit = (unsigned long)(floatFromVal(AOP(left)->aopu.aop_lit));


      DEBUGpic14_emitcode(";left lit","lit = 0x%x,sign=%d",lit,sign);

      /* Special cases */
      if((lit == 0) && (sign == 0)){

	size--;
	emitpcode(POC_MOVFW, popGet(AOP(right),size));
	while(size) 
	  emitpcode(POC_IORFW, popGet(AOP(right),--size));

	genSkipz2(&rFalseIfx,0);
	if(ifx) ifx->generated = 1;
	return;
      }

      if(size==1) {
	/* Special cases */
	lit &= 0xff;
	if(((lit == 0xff) && !sign) || ((lit==0x7f) && sign)) {
	  /* degenerate compare can never be true */
	  if(rFalseIfx.condition == 0)
	    emitpcode(POC_GOTO,popGetLabel(rFalseIfx.lbl->key));

	  if(ifx) ifx->generated = 1;
	  return;
	}

	if(sign) {
	  /* signed comparisons to a literal byte */

	  int lp1 = (lit+1) & 0xff;

	  DEBUGpic14_emitcode(";left lit","line = %d lit = 0x%x",__LINE__,lit);
	  switch (lp1) {
	  case 0:
	    rFalseIfx.condition ^= 1;
	    genSkipCond(&rFalseIfx,right,0,7);
	    break;
	  case 0x7f:
	    emitpcode(POC_MOVFW, popGet(AOP(right),0));
	    emitpcode(POC_XORLW, popGetLit(0x7f));
	    genSkipz2(&rFalseIfx,1);
	    break;
	  default:
	    emitpcode(POC_MOVFW, popGet(AOP(right),0));
	    emitpcode(POC_ADDLW, popGetLit(0x80));
	    emitpcode(POC_ADDLW, popGetLit(((-(lit+1)) & 0xff) ^ 0x80));
	    rFalseIfx.condition ^= 1;
	    genSkipc(&rFalseIfx);
	    break;
	  }
	  if(ifx) ifx->generated = 1;
	} else {
	  /* unsigned comparisons to a literal byte */

	  switch(lit & 0xff ) {
	  case 0:
	    emitpcode(POC_MOVFW, popGet(AOP(right),0));
	    genSkipz2(&rFalseIfx,0);
	    if(ifx) ifx->generated = 1;
	    break;
	  case 0x7f:
	    rFalseIfx.condition ^= 1;
	    genSkipCond(&rFalseIfx,right,0,7);
	    if(ifx) ifx->generated = 1;
	    break;

	  default:
	    emitpcode(POC_MOVLW, popGetLit((lit+1) & 0xff));
	    emitpcode(POC_SUBFW, popGet(AOP(right),0));
	    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
	    rFalseIfx.condition ^= 1;
	    if (AOP_TYPE(result) == AOP_CRY) {
	      genSkipc(&rFalseIfx);
	      if(ifx) ifx->generated = 1;
	    } else {
	      DEBUGpic14_emitcode ("; ***","%s  %d RFIfx.cond=%d",__FUNCTION__,__LINE__, rFalseIfx.condition);
	      emitpcode(POC_CLRF, popGet(AOP(result),0));
	      emitpcode(POC_RLF, popGet(AOP(result),0));
	      emitpcode(POC_MOVLW, popGetLit(0x01));
	      emitpcode(POC_XORWF, popGet(AOP(result),0));
	    }	      
	    break;
	  }
	}

	//goto check_carry;
	return;

      } else {

	/* Size is greater than 1 */

	if(sign) {
	  int lp1 = lit+1;

	  size--;

	  if(lp1 == 0) {
	    /* this means lit = 0xffffffff, or -1 */


	    DEBUGpic14_emitcode(";left lit = -1","line = %d ",__LINE__);
	    rFalseIfx.condition ^= 1;
	    genSkipCond(&rFalseIfx,right,size,7);
	    if(ifx) ifx->generated = 1;
	    return;
	  }

	  if(lit == 0) {
	    int s = size;

	    if(rFalseIfx.condition) {
	      emitpcode(POC_BTFSC, newpCodeOpBit(aopGet(AOP(right),size,FALSE,FALSE),7,0));
	      emitpcode(POC_GOTO,  popGetLabel(truelbl->key));
	    }

	    emitpcode(POC_MOVFW, popGet(AOP(right),size));
	    while(size--)
	      emitpcode(POC_IORFW, popGet(AOP(right),size));


	    emitSKPZ;
	    if(rFalseIfx.condition) {
	      emitpcode(POC_GOTO,  popGetLabel(rFalseIfx.lbl->key));
	      emitpLabel(truelbl->key);
	    }else {
	      rFalseIfx.condition ^= 1;
	      genSkipCond(&rFalseIfx,right,s,7);
	    }

	    if(ifx) ifx->generated = 1;
	    return;
	  }

	  if((size == 1) &&  (0 == (lp1&0xff))) {
	    /* lower byte of signed word is zero */
	    DEBUGpic14_emitcode(";left lit","line = %d  0x%x+1 low byte is zero",__LINE__,lit);
	    i = ((lp1 >> 8) & 0xff) ^0x80;
	    emitpcode(POC_MOVFW, popGet(AOP(right),size));
	    emitpcode(POC_ADDLW, popGetLit( 0x80));
	    emitpcode(POC_ADDLW, popGetLit(0x100-i));
	    rFalseIfx.condition ^= 1;
	    genSkipc(&rFalseIfx);


	    if(ifx) ifx->generated = 1;
	    return;
	  }

	  if(lit & (0x80 << (size*8))) {
	    /* Lit is less than zero */
	    DEBUGpic14_emitcode(";left lit","line = %d  0x%x is less than 0",__LINE__,lit);
	    //rFalseIfx.condition ^= 1;
	    //genSkipCond(&rFalseIfx,left,size,7);
	    //rFalseIfx.condition ^= 1;
	    emitpcode(POC_BTFSS, newpCodeOpBit(aopGet(AOP(right),size,FALSE,FALSE),7,0));
	    //emitpcode(POC_GOTO,  popGetLabel(truelbl->key));

	    if(rFalseIfx.condition)
	      emitpcode(POC_GOTO,  popGetLabel(rFalseIfx.lbl->key));
	    else
	      emitpcode(POC_GOTO,  popGetLabel(truelbl->key));


	  } else {
	    /* Lit is greater than or equal to zero */
	    DEBUGpic14_emitcode(";left lit","line = %d  0x%x is greater than 0",__LINE__,lit);
	    //rFalseIfx.condition ^= 1;
	    //genSkipCond(&rFalseIfx,right,size,7);
	    //rFalseIfx.condition ^= 1;

	    //emitpcode(POC_BTFSC, newpCodeOpBit(aopGet(AOP(right),size,FALSE,FALSE),7,0));
	    //emitpcode(POC_GOTO,  popGetLabel(rFalseIfx.lbl->key));

	    emitpcode(POC_BTFSC, newpCodeOpBit(aopGet(AOP(right),size,FALSE,FALSE),7,0));
	    if(rFalseIfx.condition)
	      emitpcode(POC_GOTO,  popGetLabel(truelbl->key));
	    else
	      emitpcode(POC_GOTO,  popGetLabel(rFalseIfx.lbl->key));

	  }


	  emitpcode(POC_MOVLW, popGetLit((lp1 >> (size*8)) & 0xff));
	  emitpcode(POC_SUBFW, popGet(AOP(right),size));

	  while(size--) {

	    emitpcode(POC_MOVLW, popGetLit((lp1 >> (size*8)) & 0xff));
	    emitSKPNZ;
	    emitpcode(POC_SUBFW, popGet(AOP(right),size));
	  }
	  rFalseIfx.condition ^= 1;
	  //rFalseIfx.condition = 1;
	  genSkipc(&rFalseIfx);

	  emitpLabel(truelbl->key);

	  if(ifx) ifx->generated = 1;
	  return;
	  // end of if (sign)
	} else {

	  /* compare word or long to an unsigned literal on the right.*/


	  size--;
	  if(lit < 0xff) {
	    DEBUGpic14_emitcode ("; ***","%s  %d lit =0x%x < 0xff",__FUNCTION__,__LINE__,lit);
	    switch (lit) {
	    case 0:
	      break; /* handled above */
/*
	    case 0xff:
	      emitpcode(POC_MOVFW, popGet(AOP(right),size));
	      while(size--)
		emitpcode(POC_IORFW, popGet(AOP(right),size));
	      genSkipz2(&rFalseIfx,0);
	      break;
*/
	    default:
	      emitpcode(POC_MOVFW, popGet(AOP(right),size));
	      while(--size)
		emitpcode(POC_IORFW, popGet(AOP(right),size));

	      emitSKPZ;
	      if(rFalseIfx.condition)
		emitpcode(POC_GOTO,  popGetLabel(rFalseIfx.lbl->key));
	      else
		emitpcode(POC_GOTO,  popGetLabel(truelbl->key));


	      emitpcode(POC_MOVLW, popGetLit(lit+1));
	      emitpcode(POC_SUBFW, popGet(AOP(right),0));

	      rFalseIfx.condition ^= 1;
	      genSkipc(&rFalseIfx);
	    }

	    emitpLabel(truelbl->key);

	    if(ifx) ifx->generated = 1;
	    return;
	  }


	  lit++;
	  DEBUGpic14_emitcode ("; ***","%s  %d lit =0x%x",__FUNCTION__,__LINE__,lit);
	  i = (lit >> (size*8)) & 0xff;

	  emitpcode(POC_MOVLW, popGetLit(i));
	  emitpcode(POC_SUBFW, popGet(AOP(right),size));

	  while(size--) {
	    i = (lit >> (size*8)) & 0xff;

	    if(i) {
	      emitpcode(POC_MOVLW, popGetLit(i));
	      emitSKPNZ;
	      emitpcode(POC_SUBFW, popGet(AOP(right),size));
	    } else {
	      /* this byte of the lit is zero, 
	       *if it's not the last then OR in the variable */
	      if(size)
		emitpcode(POC_IORFW, popGet(AOP(right),size));
	    }
	  }


	  emitpLabel(lbl->key);

	  rFalseIfx.condition ^= 1;
	  genSkipc(&rFalseIfx);
	}

	if(sign)
	  emitpLabel(truelbl->key);
	if(ifx) ifx->generated = 1;
	return;
      }
    }
    /* Compare two variables */

    DEBUGpic14_emitcode(";sign","%d",sign);

    size--;
    if(sign) {
      /* Sigh. thus sucks... */
      if(size) {
	emitpcode(POC_MOVFW, popGet(AOP(left),size));
	emitpcode(POC_MOVWF, popRegFromIdx(Gstack_base_addr));
	emitpcode(POC_MOVLW, popGetLit(0x80));
	emitpcode(POC_XORWF, popRegFromIdx(Gstack_base_addr));
	emitpcode(POC_XORFW, popGet(AOP(right),size));
	emitpcode(POC_SUBFW, popRegFromIdx(Gstack_base_addr));
      } else {
	/* Signed char comparison */
	/* Special thanks to Nikolai Golovchenko for this snippet */
	emitpcode(POC_MOVFW, popGet(AOP(right),0));
	emitpcode(POC_SUBFW, popGet(AOP(left),0));
	emitpcode(POC_RRFW,  popGet(AOP(left),0)); /* could be any register */
	emitpcode(POC_XORFW, popGet(AOP(left),0));
	emitpcode(POC_XORFW, popGet(AOP(right),0));
	emitpcode(POC_ADDLW, popGetLit(0x80));

	DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
	genSkipc(&rFalseIfx);
	  
	if(ifx) ifx->generated = 1;
	return;
      }

    } else {

      emitpcode(POC_MOVFW, popGet(AOP(right),size));
      emitpcode(POC_SUBFW, popGet(AOP(left),size));
    }


    /* The rest of the bytes of a multi-byte compare */
    while (size) {

      emitSKPZ;
      emitpcode(POC_GOTO,  popGetLabel(lbl->key));
      size--;

      emitpcode(POC_MOVFW, popGet(AOP(right),size));
      emitpcode(POC_SUBFW, popGet(AOP(left),size));


    }

    emitpLabel(lbl->key);

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    if ((AOP_TYPE(result) == AOP_CRY && AOP_SIZE(result)) || 
	(AOP_TYPE(result) == AOP_REG)) {
      emitpcode(POC_CLRF, popGet(AOP(result),0));
      emitpcode(POC_RLF, popGet(AOP(result),0));
    } else {
      genSkipc(&rFalseIfx);
    }	      
    //genSkipc(&rFalseIfx);
    if(ifx) ifx->generated = 1;

    return;

  }

  // check_carry:
  if (AOP_TYPE(result) == AOP_CRY && AOP_SIZE(result)) {
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    pic14_outBitC(result);
  } else {
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    /* if the result is used in the next
       ifx conditional branch then generate
       code a little differently */
    if (ifx )
      genIfxJump (ifx,"c");
    else
      pic14_outBitC(result);
    /* leave the result in acc */
  }

}

/*-----------------------------------------------------------------*/
/* genCmpGt :- greater than comparison                             */
/*-----------------------------------------------------------------*/
static void genCmpGt (iCode *ic, iCode *ifx)
{
    operand *left, *right, *result;
    sym_link *letype , *retype;
    int sign ;

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    left = IC_LEFT(ic);
    right= IC_RIGHT(ic);
    result = IC_RESULT(ic);

    letype = getSpec(operandType(left));
    retype =getSpec(operandType(right));
    sign =  !(SPEC_USIGN(letype) | SPEC_USIGN(retype));
    /* assign the amsops */
    aopOp (left,ic,FALSE);
    aopOp (right,ic,FALSE);
    aopOp (result,ic,TRUE);

    genCmp(right, left, result, ifx, sign);

    freeAsmop(left,NULL,ic,(RESULTONSTACK(ic) ? FALSE : TRUE));
    freeAsmop(right,NULL,ic,(RESULTONSTACK(ic) ? FALSE : TRUE));
    freeAsmop(result,NULL,ic,TRUE); 
}

/*-----------------------------------------------------------------*/
/* genCmpLt - less than comparisons                                */
/*-----------------------------------------------------------------*/
static void genCmpLt (iCode *ic, iCode *ifx)
{
    operand *left, *right, *result;
    sym_link *letype , *retype;
    int sign ;

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    left = IC_LEFT(ic);
    right= IC_RIGHT(ic);
    result = IC_RESULT(ic);

    letype = getSpec(operandType(left));
    retype =getSpec(operandType(right));
    sign =  !(SPEC_USIGN(letype) | SPEC_USIGN(retype));

    /* assign the amsops */
    aopOp (left,ic,FALSE);
    aopOp (right,ic,FALSE);
    aopOp (result,ic,TRUE);

    genCmp(left, right, result, ifx, sign);

    freeAsmop(left,NULL,ic,(RESULTONSTACK(ic) ? FALSE : TRUE));
    freeAsmop(right,NULL,ic,(RESULTONSTACK(ic) ? FALSE : TRUE));
    freeAsmop(result,NULL,ic,TRUE); 
}

/*-----------------------------------------------------------------*/
/* genc16bit2lit - compare a 16 bit value to a literal             */
/*-----------------------------------------------------------------*/
static void genc16bit2lit(operand *op, int lit, int offset)
{
  int i;

  DEBUGpic14_emitcode ("; ***","%s  %d, lit = %d",__FUNCTION__,__LINE__,lit);
  if( (lit&0xff) == 0) 
    i=1;
  else
    i=0;

  switch( BYTEofLONG(lit,i)) { 
  case 0:
    emitpcode(POC_MOVFW,popGet(AOP(op),offset+i));
    break;
  case 1:
    emitpcode(POC_DECFW,popGet(AOP(op),offset+i));
    break;
  case 0xff:
    emitpcode(POC_INCFW,popGet(AOP(op),offset+i));
    break;
  default:
    emitpcode(POC_MOVFW,popGet(AOP(op),offset+i));
    emitpcode(POC_XORLW,popGetLit(BYTEofLONG(lit,i)));
  }

  i ^= 1;

  switch( BYTEofLONG(lit,i)) { 
  case 0:
    emitpcode(POC_IORFW,popGet(AOP(op),offset+i));
    break;
  case 1:
    emitSKPNZ;
    emitpcode(POC_DECFW,popGet(AOP(op),offset+i));
    break;
  case 0xff:
    emitSKPNZ;
    emitpcode(POC_INCFW,popGet(AOP(op),offset+i));
    break;
  default:
    emitpcode(POC_MOVLW,popGetLit(BYTEofLONG(lit,i)));
    emitSKPNZ;
    emitpcode(POC_XORFW,popGet(AOP(op),offset+i));

  }

}

/*-----------------------------------------------------------------*/
/* gencjneshort - compare and jump if not equal                    */
/*-----------------------------------------------------------------*/
static void gencjne(operand *left, operand *right, operand *result, iCode *ifx)
{
  int size = max(AOP_SIZE(left),AOP_SIZE(right));
  int offset = 0;
  int res_offset = 0;  /* the result may be a different size then left or right */
  int res_size = AOP_SIZE(result);
  resolvedIfx rIfx;
  symbol *lbl;

  unsigned long lit = 0L;
  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
  DEBUGpic14_AopType(__LINE__,left,right,result);
  if(result)
    DEBUGpic14_emitcode ("; ***","%s  %d result is not null",__FUNCTION__,__LINE__);
  resolveIfx(&rIfx,ifx);
  lbl =  newiTempLabel(NULL);


  /* if the left side is a literal or 
     if the right is in a pointer register and left 
     is not */
  if ((AOP_TYPE(left) == AOP_LIT) || 
      (IS_AOP_PREG(right) && !IS_AOP_PREG(left))) {
    operand *t = right;
    right = left;
    left = t;
  }
  if(AOP_TYPE(right) == AOP_LIT)
    lit = (unsigned long)floatFromVal(AOP(right)->aopu.aop_lit);

  /* if the right side is a literal then anything goes */
  if (AOP_TYPE(right) == AOP_LIT &&
      AOP_TYPE(left) != AOP_DIR ) {
    switch(size) {
    case 2:
      genc16bit2lit(left, lit, 0);
      emitSKPNZ;
      emitpcode(POC_GOTO,popGetLabel(lbl->key));
      break;
    default:
      while (size--) {
	if(lit & 0xff) {
	  emitpcode(POC_MOVFW,popGet(AOP(left),offset));
	  emitpcode(POC_XORLW,popGetLit(lit & 0xff));
	} else {
	  emitpcode(POC_MOVF,popGet(AOP(left),offset));
	}

	emitSKPNZ;
	emitpcode(POC_GOTO,popGetLabel(lbl->key));
	offset++;
	if(res_offset < res_size-1)
	  res_offset++;
	lit >>= 8;
      }
      break;
    }
  }

  /* if the right side is in a register or in direct space or
     if the left is a pointer register & right is not */    
  else if (AOP_TYPE(right) == AOP_REG ||
	   AOP_TYPE(right) == AOP_DIR || 
	   (AOP_TYPE(left) == AOP_DIR && AOP_TYPE(right) == AOP_LIT) ||
	   (IS_AOP_PREG(left) && !IS_AOP_PREG(right))) {
    //int lbl_key = (rIfx.condition) ? rIfx.lbl->key : lbl->key;
    int lbl_key = lbl->key;

    if(result) {
      emitpcode(POC_CLRF,popGet(AOP(result),res_offset));
      //emitpcode(POC_INCF,popGet(AOP(result),res_offset));
    }else {
      DEBUGpic14_emitcode ("; ***","%s  %d -- ERROR",__FUNCTION__,__LINE__);
      fprintf(stderr, "%s  %d error - expecting result to be non_null\n",
	      __FUNCTION__,__LINE__);
      return;
    }

/*     switch(size) { */
/*     case 2: */
/*       genc16bit2lit(left, lit, 0); */
/*       emitSKPNZ; */
/*       emitpcode(POC_GOTO,popGetLabel(lbl->key)); */
/*       break; */
/*     default: */
    while (size--) {
      int emit_skip=1;
      if((AOP_TYPE(left) == AOP_DIR) && 
	 ((AOP_TYPE(right) == AOP_REG) || (AOP_TYPE(right) == AOP_DIR))) {

	emitpcode(POC_MOVFW,popGet(AOP(left),offset));
	emitpcode(POC_XORFW,popGet(AOP(right),offset));

      } else if((AOP_TYPE(left) == AOP_DIR) && (AOP_TYPE(right) == AOP_LIT)){
	    
	switch (lit & 0xff) {
	case 0:
	  emitpcode(POC_MOVFW,popGet(AOP(left),offset));
	  break;
	case 1:
	  emitpcode(POC_DECFSZW,popGet(AOP(left),offset));
	  emitpcode(POC_INCF,popGet(AOP(result),res_offset));
	  //emitpcode(POC_GOTO,popGetLabel(lbl->key));
	  emit_skip=0;
	  break;
	case 0xff:
	  emitpcode(POC_INCFSZW,popGet(AOP(left),offset));
	  //emitpcode(POC_INCF,popGet(AOP(result),res_offset));
	  //emitpcode(POC_INCFSZW,popGet(AOP(left),offset));
	  emitpcode(POC_GOTO,popGetLabel(lbl_key));
	  emit_skip=0;
	  break;
	default:
	  emitpcode(POC_MOVFW,popGet(AOP(left),offset));
	  emitpcode(POC_XORLW,popGetLit(lit & 0xff));
	}
	lit >>= 8;

      } else {
	emitpcode(POC_MOVF,popGet(AOP(left),offset));
      }
      if(emit_skip) {
	if(AOP_TYPE(result) == AOP_CRY) {
	  pic14_emitcode(";***","%s  %d",__FUNCTION__,__LINE__);
	  if(rIfx.condition)
	    emitSKPNZ;
	  else
	    emitSKPZ;
	  emitpcode(POC_GOTO,popGetLabel(rIfx.lbl->key));
	} else {
	  /* fix me. probably need to check result size too */
	  //emitpcode(POC_CLRF,popGet(AOP(result),0));
	  if(rIfx.condition)
	    emitSKPZ;
	  else
	    emitSKPNZ;
	  emitpcode(POC_GOTO,popGetLabel(lbl_key));
	  //emitpcode(POC_INCF,popGet(AOP(result),res_offset));
	}
	if(ifx)
	  ifx->generated=1;
      }
      emit_skip++;
      offset++;
      if(res_offset < res_size-1)
	res_offset++;
    }
/*       break; */
/*     } */
  } else if(AOP_TYPE(right) == AOP_REG &&
	    AOP_TYPE(left) != AOP_DIR){

    while(size--) {
      emitpcode(POC_MOVFW,popGet(AOP(left),offset));
      emitpcode(POC_XORFW,popGet(AOP(right),offset));
      pic14_emitcode(";***","%s  %d",__FUNCTION__,__LINE__);
      if(rIfx.condition)
	emitSKPNZ;
      else
	emitSKPZ;
      emitpcode(POC_GOTO,popGetLabel(rIfx.lbl->key));
      offset++;
      if(res_offset < res_size-1)
	res_offset++;
    }
      
  }else{
    /* right is a pointer reg need both a & b */
    while(size--) {
      char *l = aopGet(AOP(left),offset,FALSE,FALSE);
      if(strcmp(l,"b"))
	pic14_emitcode("mov","b,%s",l);
      MOVA(aopGet(AOP(right),offset,FALSE,FALSE));
      pic14_emitcode("cjne","a,b,%05d_DS_",lbl->key+100);    
      offset++;
    }
  }

  emitpcode(POC_INCF,popGet(AOP(result),res_offset));
  if(!rIfx.condition)
    emitpcode(POC_GOTO,popGetLabel(rIfx.lbl->key));

  emitpLabel(lbl->key);

  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

  if(ifx)
    ifx->generated = 1;
}

#if 0
/*-----------------------------------------------------------------*/
/* gencjne - compare and jump if not equal                         */
/*-----------------------------------------------------------------*/
static void gencjne(operand *left, operand *right, iCode *ifx)
{
    symbol *tlbl  = newiTempLabel(NULL);

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    gencjneshort(left, right, lbl);

    pic14_emitcode("mov","a,%s",one);
    pic14_emitcode("sjmp","%05d_DS_",tlbl->key+100);
    pic14_emitcode("","%05d_DS_:",lbl->key+100);
    pic14_emitcode("clr","a");
    pic14_emitcode("","%05d_DS_:",tlbl->key+100);

    emitpLabel(lbl->key);
    emitpLabel(tlbl->key);

}
#endif

/*-----------------------------------------------------------------*/
/* genCmpEq - generates code for equal to                          */
/*-----------------------------------------------------------------*/
static void genCmpEq (iCode *ic, iCode *ifx)
{
    operand *left, *right, *result;
    unsigned long lit = 0L;
    int size,offset=0;

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

    if(ifx)
      DEBUGpic14_emitcode ("; ifx is non-null","");
    else
      DEBUGpic14_emitcode ("; ifx is null","");

    aopOp((left=IC_LEFT(ic)),ic,FALSE);
    aopOp((right=IC_RIGHT(ic)),ic,FALSE);
    aopOp((result=IC_RESULT(ic)),ic,TRUE);

    size = max(AOP_SIZE(left),AOP_SIZE(right));

    DEBUGpic14_AopType(__LINE__,left,right,result);

    /* if literal, literal on the right or 
    if the right is in a pointer register and left 
    is not */
    if ((AOP_TYPE(IC_LEFT(ic)) == AOP_LIT) || 
        (IS_AOP_PREG(right) && !IS_AOP_PREG(left))) {
      operand *tmp = right ;
      right = left;
      left = tmp;
    }


    if(ifx && !AOP_SIZE(result)){
        symbol *tlbl;
        /* if they are both bit variables */
        if (AOP_TYPE(left) == AOP_CRY &&
            ((AOP_TYPE(right) == AOP_CRY) || (AOP_TYPE(right) == AOP_LIT))) {
            if(AOP_TYPE(right) == AOP_LIT){
                unsigned long lit = (unsigned long)floatFromVal(AOP(right)->aopu.aop_lit);
                if(lit == 0L){
                    pic14_emitcode("mov","c,%s",AOP(left)->aopu.aop_dir);
                    pic14_emitcode("cpl","c");
                } else if(lit == 1L) {
                    pic14_emitcode("mov","c,%s",AOP(left)->aopu.aop_dir);
                } else {
                    pic14_emitcode("clr","c");
                }
                /* AOP_TYPE(right) == AOP_CRY */
            } else {
                symbol *lbl = newiTempLabel(NULL);
                pic14_emitcode("mov","c,%s",AOP(left)->aopu.aop_dir);
                pic14_emitcode("jb","%s,%05d_DS_",AOP(right)->aopu.aop_dir,(lbl->key+100));
                pic14_emitcode("cpl","c");
                pic14_emitcode("","%05d_DS_:",(lbl->key+100));
            }
            /* if true label then we jump if condition
            supplied is true */
            tlbl = newiTempLabel(NULL);
            if ( IC_TRUE(ifx) ) {
                pic14_emitcode("jnc","%05d_DS_",tlbl->key+100);
                pic14_emitcode("ljmp","%05d_DS_",IC_TRUE(ifx)->key+100);
            } else {
                pic14_emitcode("jc","%05d_DS_",tlbl->key+100);
                pic14_emitcode("ljmp","%05d_DS_",IC_FALSE(ifx)->key+100);
            }
            pic14_emitcode("","%05d_DS_:",tlbl->key+100+labelOffset);

	    {
	      /* left and right are both bit variables, result is carry */
	      resolvedIfx rIfx;
	      
	      resolveIfx(&rIfx,ifx);

	      emitpcode(POC_MOVLW,popGet(AOP(left),0));
	      emitpcode(POC_ANDFW,popGet(AOP(left),0));
	      emitpcode(POC_BTFSC,popGet(AOP(right),0));
	      emitpcode(POC_ANDLW,popGet(AOP(left),0));
	      genSkipz2(&rIfx,0);
	    }
        } else {

	  /* They're not both bit variables. Is the right a literal? */
	  if(AOP_TYPE(right) == AOP_LIT) {
	    lit = (unsigned long)floatFromVal(AOP(right)->aopu.aop_lit);
	    
	    switch(size) {

	    case 1:
	      switch(lit & 0xff) {
	      case 1:
		if ( IC_TRUE(ifx) ) {
		  emitpcode(POC_DECFW,popGet(AOP(left),offset));
		  emitSKPNZ;
		  emitpcode(POC_GOTO,popGetLabel(IC_TRUE(ifx)->key));
		} else {
		  emitpcode(POC_DECFSZW,popGet(AOP(left),offset));
		  emitpcode(POC_GOTO,popGetLabel(IC_FALSE(ifx)->key));
		}
		break;
	      case 0xff:
		if ( IC_TRUE(ifx) ) {
		  emitpcode(POC_INCFW,popGet(AOP(left),offset));
		  emitSKPNZ;
		  emitpcode(POC_GOTO,popGetLabel(IC_TRUE(ifx)->key));
		} else {
		  emitpcode(POC_INCFSZW,popGet(AOP(left),offset));
		  emitpcode(POC_GOTO,popGetLabel(IC_FALSE(ifx)->key));
		}
		break;
	      default:
		emitpcode(POC_MOVFW,popGet(AOP(left),offset));
		if(lit)
		  emitpcode(POC_XORLW,popGetLit(lit & 0xff));
		genSkip(ifx,'z');
	      }


	      /* end of size == 1 */
	      break;
	      
	    case 2:
	      genc16bit2lit(left,lit,offset);
	      genSkip(ifx,'z');
	      break;
	      /* end of size == 2 */

	    default:
	      /* size is 4 */
	      if(lit==0) {
		emitpcode(POC_MOVFW,popGet(AOP(left),0));
		emitpcode(POC_IORFW,popGet(AOP(left),1));
		emitpcode(POC_IORFW,popGet(AOP(left),2));
		emitpcode(POC_IORFW,popGet(AOP(left),3));

	      } else {

		/* search for patterns that can be optimized */

		genc16bit2lit(left,lit,0);
		lit >>= 16;
		if(lit) {
		  genSkipz(ifx,IC_TRUE(ifx) == NULL);
		  //genSkip(ifx,'z');
		  genc16bit2lit(left,lit,2);
		} else {
		  emitpcode(POC_IORFW,popGet(AOP(left),2));
		  emitpcode(POC_IORFW,popGet(AOP(left),3));

		}
		
	      }

	      genSkip(ifx,'z');
	    }
	  
	    ifx->generated = 1;
	    goto release ;
	    

	  } else if(AOP_TYPE(right) == AOP_CRY ) {
	    /* we know the left is not a bit, but that the right is */
	    emitpcode(POC_MOVFW,popGet(AOP(left),offset));
	    emitpcode( ( (IC_TRUE(ifx)) ? POC_BTFSC : POC_BTFSS),
		      popGet(AOP(right),offset));
	    emitpcode(POC_XORLW,popGetLit(1));

	    /* if the two are equal, then W will be 0 and the Z bit is set
	     * we could test Z now, or go ahead and check the high order bytes if
	     * the variable we're comparing is larger than a byte. */

	    while(--size)
	      emitpcode(POC_IORFW,popGet(AOP(left),offset));

	    if ( IC_TRUE(ifx) ) {
	      emitSKPNZ;
	      emitpcode(POC_GOTO,popGetLabel(IC_TRUE(ifx)->key));
	      pic14_emitcode(" goto","_%05d_DS_",IC_TRUE(ifx)->key+100+labelOffset);
	    } else {
	      emitSKPZ;
	      emitpcode(POC_GOTO,popGetLabel(IC_FALSE(ifx)->key));
	      pic14_emitcode(" goto","_%05d_DS_",IC_FALSE(ifx)->key+100+labelOffset);
	    }

	  } else {
	    /* They're both variables that are larger than bits */
	    int s = size;

	    tlbl = newiTempLabel(NULL);

	    while(size--) {
	      emitpcode(POC_MOVFW,popGet(AOP(left),offset));
	      emitpcode(POC_XORFW,popGet(AOP(right),offset));

	      if ( IC_TRUE(ifx) ) {
		if(size) {
		  emitSKPZ;
		  emitpcode(POC_GOTO,popGetLabel(tlbl->key));
		  pic14_emitcode(" goto","_%05d_DS_",tlbl->key+100+labelOffset);
		} else {
		  emitSKPNZ;
		  emitpcode(POC_GOTO,popGetLabel(IC_TRUE(ifx)->key));
		  pic14_emitcode(" goto","_%05d_DS_",IC_TRUE(ifx)->key+100+labelOffset);
		}
	      } else {
		emitSKPZ;
		emitpcode(POC_GOTO,popGetLabel(IC_FALSE(ifx)->key));
		pic14_emitcode(" goto","_%05d_DS_",IC_FALSE(ifx)->key+100+labelOffset);
	      }
	      offset++;
	    }
	    if(s>1 && IC_TRUE(ifx)) {
	      emitpLabel(tlbl->key);
	      pic14_emitcode("","_%05d_DS_:",tlbl->key+100+labelOffset);                
	    }
	  }
        }
        /* mark the icode as generated */
        ifx->generated = 1;
        goto release ;
    }

    /* if they are both bit variables */
    if (AOP_TYPE(left) == AOP_CRY &&
        ((AOP_TYPE(right) == AOP_CRY) || (AOP_TYPE(right) == AOP_LIT))) {
        if(AOP_TYPE(right) == AOP_LIT){
            unsigned long lit = (unsigned long)floatFromVal(AOP(right)->aopu.aop_lit);
            if(lit == 0L){
                pic14_emitcode("mov","c,%s",AOP(left)->aopu.aop_dir);
                pic14_emitcode("cpl","c");
            } else if(lit == 1L) {
                pic14_emitcode("mov","c,%s",AOP(left)->aopu.aop_dir);
            } else {
                pic14_emitcode("clr","c");
            }
            /* AOP_TYPE(right) == AOP_CRY */
        } else {
            symbol *lbl = newiTempLabel(NULL);
            pic14_emitcode("mov","c,%s",AOP(left)->aopu.aop_dir);
            pic14_emitcode("jb","%s,%05d_DS_",AOP(right)->aopu.aop_dir,(lbl->key+100));
            pic14_emitcode("cpl","c");
            pic14_emitcode("","%05d_DS_:",(lbl->key+100));
        }
        /* c = 1 if egal */
        if (AOP_TYPE(result) == AOP_CRY && AOP_SIZE(result)){
            pic14_outBitC(result);
            goto release ;
        }
        if (ifx) {
            genIfxJump (ifx,"c");
            goto release ;
        }
        /* if the result is used in an arithmetic operation
        then put the result in place */
        pic14_outBitC(result);
    } else {
      
      DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
      gencjne(left,right,result,ifx);
/*
      if(ifx) 
	gencjne(left,right,newiTempLabel(NULL));
      else {
	if(IC_TRUE(ifx)->key)
	  gencjne(left,right,IC_TRUE(ifx)->key);
	else
	  gencjne(left,right,IC_FALSE(ifx)->key);
	ifx->generated = 1;
	goto release ;
      }
      if (AOP_TYPE(result) == AOP_CRY && AOP_SIZE(result)) {
	aopPut(AOP(result),"a",0);
	goto release ;
      }

      if (ifx) {
	genIfxJump (ifx,"a");
	goto release ;
      }
*/
      /* if the result is used in an arithmetic operation
	 then put the result in place */
/*
      if (AOP_TYPE(result) != AOP_CRY) 
	pic14_outAcc(result);
*/
      /* leave the result in acc */
    }

release:
    freeAsmop(left,NULL,ic,(RESULTONSTACK(ic) ? FALSE : TRUE));
    freeAsmop(right,NULL,ic,(RESULTONSTACK(ic) ? FALSE : TRUE));
    freeAsmop(result,NULL,ic,TRUE);
}

/*-----------------------------------------------------------------*/
/* ifxForOp - returns the icode containing the ifx for operand     */
/*-----------------------------------------------------------------*/
static iCode *ifxForOp ( operand *op, iCode *ic )
{
    /* if true symbol then needs to be assigned */
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    if (IS_TRUE_SYMOP(op))
        return NULL ;

    /* if this has register type condition and
    the next instruction is ifx with the same operand
    and live to of the operand is upto the ifx only then */
    if (ic->next &&
        ic->next->op == IFX &&
        IC_COND(ic->next)->key == op->key &&
        OP_SYMBOL(op)->liveTo <= ic->next->seq )
        return ic->next;

    if (ic->next &&
        ic->next->op == IFX &&
        IC_COND(ic->next)->key == op->key) {
      DEBUGpic14_emitcode ("; WARNING ","%d IGNORING liveTo range in %s",__LINE__,__FUNCTION__);
      return ic->next;
    }

    DEBUGpic14_emitcode ("; NULL :(","%d",__LINE__);
    if (ic->next &&
        ic->next->op == IFX)
      DEBUGpic14_emitcode ("; ic-next"," is an IFX");

    if (ic->next &&
        ic->next->op == IFX &&
        IC_COND(ic->next)->key == op->key) {
      DEBUGpic14_emitcode ("; "," key is okay");
      DEBUGpic14_emitcode ("; "," key liveTo %d, next->seq = %d",
			   OP_SYMBOL(op)->liveTo,
			   ic->next->seq);
    }


    return NULL;
}
/*-----------------------------------------------------------------*/
/* genAndOp - for && operation                                     */
/*-----------------------------------------------------------------*/
static void genAndOp (iCode *ic)
{
    operand *left,*right, *result;
/*     symbol *tlbl; */

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    /* note here that && operations that are in an
    if statement are taken away by backPatchLabels
    only those used in arthmetic operations remain */
    aopOp((left=IC_LEFT(ic)),ic,FALSE);
    aopOp((right=IC_RIGHT(ic)),ic,FALSE);
    aopOp((result=IC_RESULT(ic)),ic,FALSE);

    DEBUGpic14_AopType(__LINE__,left,right,result);

    emitpcode(POC_MOVFW,popGet(AOP(left),0));
    emitpcode(POC_ANDFW,popGet(AOP(right),0));
    emitpcode(POC_MOVWF,popGet(AOP(result),0));

    /* if both are bit variables */
/*     if (AOP_TYPE(left) == AOP_CRY && */
/*         AOP_TYPE(right) == AOP_CRY ) { */
/*         pic14_emitcode("mov","c,%s",AOP(left)->aopu.aop_dir); */
/*         pic14_emitcode("anl","c,%s",AOP(right)->aopu.aop_dir); */
/*         pic14_outBitC(result); */
/*     } else { */
/*         tlbl = newiTempLabel(NULL); */
/*         pic14_toBoolean(left);     */
/*         pic14_emitcode("jz","%05d_DS_",tlbl->key+100); */
/*         pic14_toBoolean(right); */
/*         pic14_emitcode("","%05d_DS_:",tlbl->key+100); */
/*         pic14_outBitAcc(result); */
/*     } */

    freeAsmop(left,NULL,ic,(RESULTONSTACK(ic) ? FALSE : TRUE));
    freeAsmop(right,NULL,ic,(RESULTONSTACK(ic) ? FALSE : TRUE));
    freeAsmop(result,NULL,ic,TRUE);
}


/*-----------------------------------------------------------------*/
/* genOrOp - for || operation                                      */
/*-----------------------------------------------------------------*/
/*
  tsd pic port -
  modified this code, but it doesn't appear to ever get called
*/

static void genOrOp (iCode *ic)
{
    operand *left,*right, *result;
    symbol *tlbl;

    /* note here that || operations that are in an
    if statement are taken away by backPatchLabels
    only those used in arthmetic operations remain */
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    aopOp((left=IC_LEFT(ic)),ic,FALSE);
    aopOp((right=IC_RIGHT(ic)),ic,FALSE);
    aopOp((result=IC_RESULT(ic)),ic,FALSE);

    DEBUGpic14_AopType(__LINE__,left,right,result);

    /* if both are bit variables */
    if (AOP_TYPE(left) == AOP_CRY &&
        AOP_TYPE(right) == AOP_CRY ) {
      pic14_emitcode("clrc","");
      pic14_emitcode("btfss","(%s >> 3), (%s & 7)",
	       AOP(left)->aopu.aop_dir,
	       AOP(left)->aopu.aop_dir);
      pic14_emitcode("btfsc","(%s >> 3), (%s & 7)",
	       AOP(right)->aopu.aop_dir,
	       AOP(right)->aopu.aop_dir);
      pic14_emitcode("setc","");

    } else {
        tlbl = newiTempLabel(NULL);
        pic14_toBoolean(left);
	emitSKPZ;
        pic14_emitcode("goto","%05d_DS_",tlbl->key+100+labelOffset);
        pic14_toBoolean(right);
        pic14_emitcode("","%05d_DS_:",tlbl->key+100+labelOffset);

        pic14_outBitAcc(result);
    }

    freeAsmop(left,NULL,ic,(RESULTONSTACK(ic) ? FALSE : TRUE));
    freeAsmop(right,NULL,ic,(RESULTONSTACK(ic) ? FALSE : TRUE));
    freeAsmop(result,NULL,ic,TRUE);            
}

/*-----------------------------------------------------------------*/
/* isLiteralBit - test if lit == 2^n                               */
/*-----------------------------------------------------------------*/
static int isLiteralBit(unsigned long lit)
{
    unsigned long pw[32] = {1L,2L,4L,8L,16L,32L,64L,128L,
    0x100L,0x200L,0x400L,0x800L,
    0x1000L,0x2000L,0x4000L,0x8000L,
    0x10000L,0x20000L,0x40000L,0x80000L,
    0x100000L,0x200000L,0x400000L,0x800000L,
    0x1000000L,0x2000000L,0x4000000L,0x8000000L,
    0x10000000L,0x20000000L,0x40000000L,0x80000000L};
    int idx;
    
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    for(idx = 0; idx < 32; idx++)
        if(lit == pw[idx])
            return idx+1;
    return 0;
}

/*-----------------------------------------------------------------*/
/* continueIfTrue -                                                */
/*-----------------------------------------------------------------*/
static void continueIfTrue (iCode *ic)
{
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    if(IC_TRUE(ic))
        pic14_emitcode("ljmp","%05d_DS_",IC_TRUE(ic)->key+100);
    ic->generated = 1;
}

/*-----------------------------------------------------------------*/
/* jmpIfTrue -                                                     */
/*-----------------------------------------------------------------*/
static void jumpIfTrue (iCode *ic)
{
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    if(!IC_TRUE(ic))
        pic14_emitcode("ljmp","%05d_DS_",IC_FALSE(ic)->key+100);
    ic->generated = 1;
}

/*-----------------------------------------------------------------*/
/* jmpTrueOrFalse -                                                */
/*-----------------------------------------------------------------*/
static void jmpTrueOrFalse (iCode *ic, symbol *tlbl)
{
    // ugly but optimized by peephole
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    if(IC_TRUE(ic)){
        symbol *nlbl = newiTempLabel(NULL);
        pic14_emitcode("sjmp","%05d_DS_",nlbl->key+100);                 
        pic14_emitcode("","%05d_DS_:",tlbl->key+100);
        pic14_emitcode("ljmp","%05d_DS_",IC_TRUE(ic)->key+100);
        pic14_emitcode("","%05d_DS_:",nlbl->key+100);
    }
    else{
        pic14_emitcode("ljmp","%05d_DS_",IC_FALSE(ic)->key+100);
        pic14_emitcode("","%05d_DS_:",tlbl->key+100);
    }
    ic->generated = 1;
}

/*-----------------------------------------------------------------*/
/* genAnd  - code for and                                          */
/*-----------------------------------------------------------------*/
static void genAnd (iCode *ic, iCode *ifx)
{
  operand *left, *right, *result;
  int size, offset=0;  
  unsigned long lit = 0L;
  int bytelit = 0;
  resolvedIfx rIfx;


  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
  aopOp((left = IC_LEFT(ic)),ic,FALSE);
  aopOp((right= IC_RIGHT(ic)),ic,FALSE);
  aopOp((result=IC_RESULT(ic)),ic,TRUE);

  resolveIfx(&rIfx,ifx);

  /* if left is a literal & right is not then exchange them */
  if ((AOP_TYPE(left) == AOP_LIT && AOP_TYPE(right) != AOP_LIT) ||
      AOP_NEEDSACC(left)) {
    operand *tmp = right ;
    right = left;
    left = tmp;
  }

  /* if result = right then exchange them */
  if(pic14_sameRegs(AOP(result),AOP(right))){
    operand *tmp = right ;
    right = left;
    left = tmp;
  }

  /* if right is bit then exchange them */
  if (AOP_TYPE(right) == AOP_CRY &&
      AOP_TYPE(left) != AOP_CRY){
    operand *tmp = right ;
    right = left;
    left = tmp;
  }
  if(AOP_TYPE(right) == AOP_LIT)
    lit = (unsigned long)floatFromVal (AOP(right)->aopu.aop_lit);

  size = AOP_SIZE(result);

  DEBUGpic14_AopType(__LINE__,left,right,result);

  // if(bit & yy)
  // result = bit & yy;
  if (AOP_TYPE(left) == AOP_CRY){
    // c = bit & literal;
    if(AOP_TYPE(right) == AOP_LIT){
      if(lit & 1) {
	if(size && pic14_sameRegs(AOP(result),AOP(left)))
	  // no change
	  goto release;
	pic14_emitcode("mov","c,%s",AOP(left)->aopu.aop_dir);
      } else {
	// bit(result) = 0;
	if(size && (AOP_TYPE(result) == AOP_CRY)){
	  pic14_emitcode("clr","%s",AOP(result)->aopu.aop_dir);
	  goto release;
	}
	if((AOP_TYPE(result) == AOP_CRY) && ifx){
	  jumpIfTrue(ifx);
	  goto release;
	}
	pic14_emitcode("clr","c");
      }
    } else {
      if (AOP_TYPE(right) == AOP_CRY){
	// c = bit & bit;
	pic14_emitcode("mov","c,%s",AOP(right)->aopu.aop_dir);
	pic14_emitcode("anl","c,%s",AOP(left)->aopu.aop_dir);
      } else {
	// c = bit & val;
	MOVA(aopGet(AOP(right),0,FALSE,FALSE));
	// c = lsb
	pic14_emitcode("rrc","a");
	pic14_emitcode("anl","c,%s",AOP(left)->aopu.aop_dir);
      }
    }
    // bit = c
    // val = c
    if(size)
      pic14_outBitC(result);
    // if(bit & ...)
    else if((AOP_TYPE(result) == AOP_CRY) && ifx)
      genIfxJump(ifx, "c");           
    goto release ;
  }

  // if(val & 0xZZ)       - size = 0, ifx != FALSE  -
  // bit = val & 0xZZ     - size = 1, ifx = FALSE -
  if((AOP_TYPE(right) == AOP_LIT) &&
     (AOP_TYPE(result) == AOP_CRY) &&
     (AOP_TYPE(left) != AOP_CRY)){
    int posbit = isLiteralBit(lit);
    /* left &  2^n */
    if(posbit){
      posbit--;
      //MOVA(aopGet(AOP(left),posbit>>3,FALSE,FALSE));
      // bit = left & 2^n
      if(size)
	pic14_emitcode("mov","c,acc.%d",posbit&0x07);
      // if(left &  2^n)
      else{
	if(ifx){
/*
	  if(IC_TRUE(ifx)) {
	    emitpcode(POC_BTFSC,newpCodeOpBit(aopGet(AOP(left),0,FALSE,FALSE),posbit,0));
	    emitpcode(POC_GOTO,popGetLabel(IC_TRUE(ic)->key));
	  } else {
	    emitpcode(POC_BTFSS,newpCodeOpBit(aopGet(AOP(left),0,FALSE,FALSE),posbit,0));
	    emitpcode(POC_GOTO,popGetLabel(IC_FALSE(ic)->key));
	  }
*/
	  emitpcode(((rIfx.condition) ? POC_BTFSC : POC_BTFSS),
		    newpCodeOpBit(aopGet(AOP(left),0,FALSE,FALSE),posbit,0));
	  emitpcode(POC_GOTO,popGetLabel(rIfx.lbl->key));
	  
	  ifx->generated = 1;
	}
	goto release;
      }
    } else {
      symbol *tlbl = newiTempLabel(NULL);
      int sizel = AOP_SIZE(left);
      if(size)
	pic14_emitcode("setb","c");
      while(sizel--){
	if((bytelit = ((lit >> (offset*8)) & 0x0FFL)) != 0x0L){
	  MOVA( aopGet(AOP(left),offset,FALSE,FALSE));
	  // byte ==  2^n ?
	  if((posbit = isLiteralBit(bytelit)) != 0)
	    pic14_emitcode("jb","acc.%d,%05d_DS_",(posbit-1)&0x07,tlbl->key+100);
	  else{
	    if(bytelit != 0x0FFL)
	      pic14_emitcode("anl","a,%s",
			     aopGet(AOP(right),offset,FALSE,TRUE));
	    pic14_emitcode("jnz","%05d_DS_",tlbl->key+100);
	  }
	}
	offset++;
      }
      // bit = left & literal
      if(size){
	pic14_emitcode("clr","c");
	pic14_emitcode("","%05d_DS_:",tlbl->key+100);
      }
      // if(left & literal)
      else{
	if(ifx)
	  jmpTrueOrFalse(ifx, tlbl);
	goto release ;
      }
    }
    pic14_outBitC(result);
    goto release ;
  }

  /* if left is same as result */
  if(pic14_sameRegs(AOP(result),AOP(left))){
    int know_W = -1;
    for(;size--; offset++,lit>>=8) {
      if(AOP_TYPE(right) == AOP_LIT){
	switch(lit & 0xff) {
	case 0x00:
	  /*  and'ing with 0 has clears the result */
	  pic14_emitcode("clrf","%s",aopGet(AOP(result),offset,FALSE,FALSE));
	  emitpcode(POC_CLRF,popGet(AOP(result),offset));
	  break;
	case 0xff:
	  /* and'ing with 0xff is a nop when the result and left are the same */
	  break;

	default:
	  {
	    int p = my_powof2( (~lit) & 0xff );
	    if(p>=0) {
	      /* only one bit is set in the literal, so use a bcf instruction */
	      pic14_emitcode("bcf","%s,%d",aopGet(AOP(left),offset,FALSE,TRUE),p);
	      emitpcode(POC_BCF,newpCodeOpBit(aopGet(AOP(left),offset,FALSE,FALSE),p,0));

	    } else {
	      pic14_emitcode("movlw","0x%x", (lit & 0xff));
	      pic14_emitcode("andwf","%s,f",aopGet(AOP(left),offset,FALSE,TRUE));
	      if(know_W != (int)(lit&0xff))
		emitpcode(POC_MOVLW, popGetLit(lit & 0xff));
	      know_W = lit &0xff;
	      emitpcode(POC_ANDWF,popGet(AOP(left),offset));
	    }
	  }    
	}
      } else {
	if (AOP_TYPE(left) == AOP_ACC) {
	  emitpcode(POC_ANDFW,popGet(AOP(right),offset));
	} else {		    
	  emitpcode(POC_MOVFW,popGet(AOP(right),offset));
	  emitpcode(POC_ANDWF,popGet(AOP(left),offset));

	}
      }
    }

  } else {
    // left & result in different registers
    if(AOP_TYPE(result) == AOP_CRY){
      // result = bit
      // if(size), result in bit
      // if(!size && ifx), conditional oper: if(left & right)
      symbol *tlbl = newiTempLabel(NULL);
      int sizer = min(AOP_SIZE(left),AOP_SIZE(right));
      if(size)
	pic14_emitcode("setb","c");
      while(sizer--){
	MOVA(aopGet(AOP(right),offset,FALSE,FALSE));
	pic14_emitcode("anl","a,%s",
		       aopGet(AOP(left),offset,FALSE,FALSE));
	pic14_emitcode("jnz","%05d_DS_",tlbl->key+100);
	offset++;
      }
      if(size){
	CLRC;
	pic14_emitcode("","%05d_DS_:",tlbl->key+100);
	pic14_outBitC(result);
      } else if(ifx)
	jmpTrueOrFalse(ifx, tlbl);
    } else {
      for(;(size--);offset++) {
	// normal case
	// result = left & right
	if(AOP_TYPE(right) == AOP_LIT){
	  int t = (lit >> (offset*8)) & 0x0FFL;
	  switch(t) { 
	  case 0x00:
	    pic14_emitcode("clrf","%s",
			   aopGet(AOP(result),offset,FALSE,FALSE));
	    emitpcode(POC_CLRF,popGet(AOP(result),offset));
	    break;
	  case 0xff:
	    if(AOP_TYPE(left) != AOP_ACC) {
	      pic14_emitcode("movf","%s,w",
			     aopGet(AOP(left),offset,FALSE,FALSE));
	      pic14_emitcode("movwf","%s",
			     aopGet(AOP(result),offset,FALSE,FALSE));
	      emitpcode(POC_MOVFW,popGet(AOP(left),offset));
	    }
	    emitpcode(POC_MOVWF,popGet(AOP(result),offset));
	    break;
	  default:
	    if(AOP_TYPE(left) == AOP_ACC) {
	      emitpcode(POC_ANDLW, popGetLit(t));
	    } else {
	      pic14_emitcode("movlw","0x%x",t);
	      pic14_emitcode("andwf","%s,w",
			     aopGet(AOP(left),offset,FALSE,FALSE));
	      pic14_emitcode("movwf","%s",
			     aopGet(AOP(result),offset,FALSE,FALSE));
	      
	      emitpcode(POC_MOVLW, popGetLit(t));
	      emitpcode(POC_ANDFW,popGet(AOP(left),offset));
	    }
	    emitpcode(POC_MOVWF,popGet(AOP(result),offset));
	  }
	  continue;
	}

	if (AOP_TYPE(left) == AOP_ACC) {
	  pic14_emitcode("andwf","%s,w",aopGet(AOP(right),offset,FALSE,FALSE));
	  emitpcode(POC_ANDFW,popGet(AOP(right),offset));
	} else {
	  pic14_emitcode("movf","%s,w",aopGet(AOP(right),offset,FALSE,FALSE));
	  pic14_emitcode("andwf","%s,w",
			 aopGet(AOP(left),offset,FALSE,FALSE));
	  emitpcode(POC_MOVFW,popGet(AOP(right),offset));
	  emitpcode(POC_ANDFW,popGet(AOP(left),offset));
	}
	pic14_emitcode("movwf","%s",aopGet(AOP(result),offset,FALSE,FALSE));
	emitpcode(POC_MOVWF,popGet(AOP(result),offset));
      }
    }
  }

  release :
    freeAsmop(left,NULL,ic,(RESULTONSTACK(ic) ? FALSE : TRUE));
  freeAsmop(right,NULL,ic,(RESULTONSTACK(ic) ? FALSE : TRUE));
  freeAsmop(result,NULL,ic,TRUE);     
}

/*-----------------------------------------------------------------*/
/* genOr  - code for or                                            */
/*-----------------------------------------------------------------*/
static void genOr (iCode *ic, iCode *ifx)
{
    operand *left, *right, *result;
    int size, offset=0;
    unsigned long lit = 0L;

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

    aopOp((left = IC_LEFT(ic)),ic,FALSE);
    aopOp((right= IC_RIGHT(ic)),ic,FALSE);
    aopOp((result=IC_RESULT(ic)),ic,TRUE);

    DEBUGpic14_AopType(__LINE__,left,right,result);

    /* if left is a literal & right is not then exchange them */
    if ((AOP_TYPE(left) == AOP_LIT && AOP_TYPE(right) != AOP_LIT) ||
	AOP_NEEDSACC(left)) {
        operand *tmp = right ;
        right = left;
        left = tmp;
    }

    /* if result = right then exchange them */
    if(pic14_sameRegs(AOP(result),AOP(right))){
        operand *tmp = right ;
        right = left;
        left = tmp;
    }

    /* if right is bit then exchange them */
    if (AOP_TYPE(right) == AOP_CRY &&
        AOP_TYPE(left) != AOP_CRY){
        operand *tmp = right ;
        right = left;
        left = tmp;
    }

    DEBUGpic14_AopType(__LINE__,left,right,result);

    if(AOP_TYPE(right) == AOP_LIT)
        lit = (unsigned long)floatFromVal (AOP(right)->aopu.aop_lit);

    size = AOP_SIZE(result);

    // if(bit | yy)
    // xx = bit | yy;
    if (AOP_TYPE(left) == AOP_CRY){
        if(AOP_TYPE(right) == AOP_LIT){
            // c = bit & literal;
            if(lit){
                // lit != 0 => result = 1
                if(AOP_TYPE(result) == AOP_CRY){
		  if(size)
		    emitpcode(POC_BSF, popGet(AOP(result),0));
		  //pic14_emitcode("bsf","(%s >> 3), (%s & 7)",
		  //	 AOP(result)->aopu.aop_dir,
		  //	 AOP(result)->aopu.aop_dir);
                    else if(ifx)
                        continueIfTrue(ifx);
                    goto release;
                }
            } else {
                // lit == 0 => result = left
                if(size && pic14_sameRegs(AOP(result),AOP(left)))
                    goto release;
                pic14_emitcode(";XXX mov","c,%s  %s,%d",AOP(left)->aopu.aop_dir,__FILE__,__LINE__);
            }
        } else {
            if (AOP_TYPE(right) == AOP_CRY){
	      if(pic14_sameRegs(AOP(result),AOP(left))){
                // c = bit | bit;
		emitpcode(POC_BCF,   popGet(AOP(result),0));
		emitpcode(POC_BTFSC, popGet(AOP(right),0));
		emitpcode(POC_BSF,   popGet(AOP(result),0));

		pic14_emitcode("bcf","(%s >> 3), (%s & 7)",
			 AOP(result)->aopu.aop_dir,
			 AOP(result)->aopu.aop_dir);
		pic14_emitcode("btfsc","(%s >> 3), (%s & 7)",
			 AOP(right)->aopu.aop_dir,
			 AOP(right)->aopu.aop_dir);
		pic14_emitcode("bsf","(%s >> 3), (%s & 7)",
			 AOP(result)->aopu.aop_dir,
			 AOP(result)->aopu.aop_dir);
	      } else {
		if( AOP_TYPE(result) == AOP_ACC) {
		  emitpcode(POC_MOVLW, popGetLit(0));
		  emitpcode(POC_BTFSS, popGet(AOP(right),0));
		  emitpcode(POC_BTFSC, popGet(AOP(left),0));
		  emitpcode(POC_MOVLW, popGetLit(1));

		} else {

		  emitpcode(POC_BCF,   popGet(AOP(result),0));
		  emitpcode(POC_BTFSS, popGet(AOP(right),0));
		  emitpcode(POC_BTFSC, popGet(AOP(left),0));
		  emitpcode(POC_BSF,   popGet(AOP(result),0));

		  pic14_emitcode("bcf","(%s >> 3), (%s & 7)",
				 AOP(result)->aopu.aop_dir,
				 AOP(result)->aopu.aop_dir);
		  pic14_emitcode("btfss","(%s >> 3), (%s & 7)",
				 AOP(right)->aopu.aop_dir,
				 AOP(right)->aopu.aop_dir);
		  pic14_emitcode("btfsc","(%s >> 3), (%s & 7)",
				 AOP(left)->aopu.aop_dir,
				 AOP(left)->aopu.aop_dir);
		  pic14_emitcode("bsf","(%s >> 3), (%s & 7)",
				 AOP(result)->aopu.aop_dir,
				 AOP(result)->aopu.aop_dir);
		}
	      }
            } else {
                // c = bit | val;
                symbol *tlbl = newiTempLabel(NULL);
                pic14_emitcode(";XXX "," %s,%d",__FILE__,__LINE__);


		emitpcode(POC_BCF,   popGet(AOP(result),0));
		if( AOP_TYPE(right) == AOP_ACC) {
		  emitpcode(POC_IORLW, popGetLit(0));
		  emitSKPNZ;
		  emitpcode(POC_BTFSC, popGet(AOP(left),0));
		  emitpcode(POC_BSF,   popGet(AOP(result),0));
		}



                if(!((AOP_TYPE(result) == AOP_CRY) && ifx))
                    pic14_emitcode(";XXX setb","c");
                pic14_emitcode(";XXX jb","%s,%05d_DS_",
                         AOP(left)->aopu.aop_dir,tlbl->key+100);
                pic14_toBoolean(right);
                pic14_emitcode(";XXX jnz","%05d_DS_",tlbl->key+100);
                if((AOP_TYPE(result) == AOP_CRY) && ifx){
                    jmpTrueOrFalse(ifx, tlbl);
                    goto release;
                } else {
                    CLRC;
                    pic14_emitcode("","%05d_DS_:",tlbl->key+100);
                }
            }
        }
        // bit = c
        // val = c
        if(size)
            pic14_outBitC(result);
        // if(bit | ...)
        else if((AOP_TYPE(result) == AOP_CRY) && ifx)
            genIfxJump(ifx, "c");           
        goto release ;
    }

    // if(val | 0xZZ)       - size = 0, ifx != FALSE  -
    // bit = val | 0xZZ     - size = 1, ifx = FALSE -
    if((AOP_TYPE(right) == AOP_LIT) &&
       (AOP_TYPE(result) == AOP_CRY) &&
       (AOP_TYPE(left) != AOP_CRY)){
        if(lit){
	  pic14_emitcode(";XXX "," %s,%d",__FILE__,__LINE__);
            // result = 1
            if(size)
                pic14_emitcode(";XXX setb","%s",AOP(result)->aopu.aop_dir);
            else 
                continueIfTrue(ifx);
            goto release;
        } else {
	  pic14_emitcode(";XXX "," %s,%d",__FILE__,__LINE__);
            // lit = 0, result = boolean(left)
            if(size)
                pic14_emitcode(";XXX setb","c");
            pic14_toBoolean(right);
            if(size){
                symbol *tlbl = newiTempLabel(NULL);
                pic14_emitcode(";XXX jnz","%05d_DS_",tlbl->key+100);
                CLRC;
                pic14_emitcode("","%05d_DS_:",tlbl->key+100);
            } else {
                genIfxJump (ifx,"a");
                goto release;
            }
        }
        pic14_outBitC(result);
        goto release ;
    }

    /* if left is same as result */
    if(pic14_sameRegs(AOP(result),AOP(left))){
      int know_W = -1;
      for(;size--; offset++,lit>>=8) {
	if(AOP_TYPE(right) == AOP_LIT){
	  if((lit & 0xff) == 0)
	    /*  or'ing with 0 has no effect */
	    continue;
	  else {
	    int p = my_powof2(lit & 0xff);
	    if(p>=0) {
	      /* only one bit is set in the literal, so use a bsf instruction */
	      emitpcode(POC_BSF,
			newpCodeOpBit(aopGet(AOP(left),offset,FALSE,FALSE),p,0));
	    } else {
	      if(know_W != (int)(lit & 0xff))
		emitpcode(POC_MOVLW, popGetLit(lit & 0xff));
	      know_W = lit & 0xff;
	      emitpcode(POC_IORWF, popGet(AOP(left),offset));
	    }
		    
	  }
	} else {
	  if (AOP_TYPE(left) == AOP_ACC) {
	    emitpcode(POC_IORFW,  popGet(AOP(right),offset));
	    pic14_emitcode("iorwf","%s,w",aopGet(AOP(right),offset,FALSE,FALSE));
	  } else {		    
	    emitpcode(POC_MOVFW,  popGet(AOP(right),offset));
	    emitpcode(POC_IORWF,  popGet(AOP(left),offset));

	    pic14_emitcode("movf","%s,w",aopGet(AOP(right),offset,FALSE,FALSE));
	    pic14_emitcode("iorwf","%s,f",aopGet(AOP(left),offset,FALSE,FALSE));

	  }
	}
      }
    } else {
        // left & result in different registers
        if(AOP_TYPE(result) == AOP_CRY){
            // result = bit
            // if(size), result in bit
            // if(!size && ifx), conditional oper: if(left | right)
            symbol *tlbl = newiTempLabel(NULL);
            int sizer = max(AOP_SIZE(left),AOP_SIZE(right));
	    pic14_emitcode(";XXX "," %s,%d",__FILE__,__LINE__);


            if(size)
                pic14_emitcode(";XXX setb","c");
            while(sizer--){
                MOVA(aopGet(AOP(right),offset,FALSE,FALSE));
                pic14_emitcode(";XXX orl","a,%s",
                         aopGet(AOP(left),offset,FALSE,FALSE));
                pic14_emitcode(";XXX jnz","%05d_DS_",tlbl->key+100);
                offset++;
            }
            if(size){
                CLRC;
                pic14_emitcode("","%05d_DS_:",tlbl->key+100);
                pic14_outBitC(result);
            } else if(ifx)
                jmpTrueOrFalse(ifx, tlbl);
        } else for(;(size--);offset++){
	  // normal case
	  // result = left & right
	  if(AOP_TYPE(right) == AOP_LIT){
	    int t = (lit >> (offset*8)) & 0x0FFL;
	    switch(t) { 
	    case 0x00:
	      if (AOP_TYPE(left) != AOP_ACC) {
		emitpcode(POC_MOVFW,  popGet(AOP(left),offset));
	      }
	      emitpcode(POC_MOVWF,  popGet(AOP(result),offset));

	      break;
	    default:
	      if (AOP_TYPE(left) == AOP_ACC) {
		emitpcode(POC_IORLW,  popGetLit(t));
	      } else {
		emitpcode(POC_MOVLW,  popGetLit(t));
		emitpcode(POC_IORFW,  popGet(AOP(left),offset));
	      }
	      emitpcode(POC_MOVWF,  popGet(AOP(result),offset));	      
	    }
	    continue;
	  }

	  // faster than result <- left, anl result,right
	  // and better if result is SFR
	  if (AOP_TYPE(left) == AOP_ACC) {
	    emitpcode(POC_IORWF,  popGet(AOP(right),offset));
	    pic14_emitcode("iorwf","%s,w",aopGet(AOP(right),offset,FALSE,FALSE));
	  } else {
	    emitpcode(POC_MOVFW,  popGet(AOP(right),offset));
	    emitpcode(POC_IORFW,  popGet(AOP(left),offset));

	    pic14_emitcode("movf","%s,w",aopGet(AOP(right),offset,FALSE,FALSE));
	    pic14_emitcode("iorwf","%s,w",
		     aopGet(AOP(left),offset,FALSE,FALSE));
	  }
	  emitpcode(POC_MOVWF,  popGet(AOP(result),offset));
	  pic14_emitcode("movwf","%s",aopGet(AOP(result),offset,FALSE,FALSE));
	}
    }

release :
    freeAsmop(left,NULL,ic,(RESULTONSTACK(ic) ? FALSE : TRUE));
    freeAsmop(right,NULL,ic,(RESULTONSTACK(ic) ? FALSE : TRUE));
    freeAsmop(result,NULL,ic,TRUE);     
}

/*-----------------------------------------------------------------*/
/* genXor - code for xclusive or                                   */
/*-----------------------------------------------------------------*/
static void genXor (iCode *ic, iCode *ifx)
{
  operand *left, *right, *result;
  int size, offset=0;
  unsigned long lit = 0L;

  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

  aopOp((left = IC_LEFT(ic)),ic,FALSE);
  aopOp((right= IC_RIGHT(ic)),ic,FALSE);
  aopOp((result=IC_RESULT(ic)),ic,TRUE);

  /* if left is a literal & right is not ||
     if left needs acc & right does not */
  if ((AOP_TYPE(left) == AOP_LIT && AOP_TYPE(right) != AOP_LIT) ||
      (AOP_NEEDSACC(left) && !AOP_NEEDSACC(right))) {
    operand *tmp = right ;
    right = left;
    left = tmp;
  }

  /* if result = right then exchange them */
  if(pic14_sameRegs(AOP(result),AOP(right))){
    operand *tmp = right ;
    right = left;
    left = tmp;
  }

  /* if right is bit then exchange them */
  if (AOP_TYPE(right) == AOP_CRY &&
      AOP_TYPE(left) != AOP_CRY){
    operand *tmp = right ;
    right = left;
    left = tmp;
  }
  if(AOP_TYPE(right) == AOP_LIT)
    lit = (unsigned long)floatFromVal (AOP(right)->aopu.aop_lit);

  size = AOP_SIZE(result);

  // if(bit ^ yy)
  // xx = bit ^ yy;
  if (AOP_TYPE(left) == AOP_CRY){
    if(AOP_TYPE(right) == AOP_LIT){
      // c = bit & literal;
      if(lit>>1){
	// lit>>1  != 0 => result = 1
	if(AOP_TYPE(result) == AOP_CRY){
	  if(size)
	    {emitpcode(POC_BSF,  popGet(AOP(result),offset));
	    pic14_emitcode("setb","%s",AOP(result)->aopu.aop_dir);}
	  else if(ifx)
	    continueIfTrue(ifx);
	  goto release;
	}
	pic14_emitcode("setb","c");
      } else{
	// lit == (0 or 1)
	if(lit == 0){
	  // lit == 0, result = left
	  if(size && pic14_sameRegs(AOP(result),AOP(left)))
	    goto release;
	  pic14_emitcode("mov","c,%s",AOP(left)->aopu.aop_dir);
	} else{
	  // lit == 1, result = not(left)
	  if(size && pic14_sameRegs(AOP(result),AOP(left))){
	    emitpcode(POC_MOVLW,  popGet(AOP(result),offset));
	    emitpcode(POC_XORWF,  popGet(AOP(result),offset));
	    pic14_emitcode("cpl","%s",AOP(result)->aopu.aop_dir);
	    goto release;
	  } else {
	    pic14_emitcode("mov","c,%s",AOP(left)->aopu.aop_dir);
	    pic14_emitcode("cpl","c");
	  }
	}
      }

    } else {
      // right != literal
      symbol *tlbl = newiTempLabel(NULL);
      if (AOP_TYPE(right) == AOP_CRY){
	// c = bit ^ bit;
	pic14_emitcode("mov","c,%s",AOP(right)->aopu.aop_dir);
      }
      else{
	int sizer = AOP_SIZE(right);
	// c = bit ^ val
	// if val>>1 != 0, result = 1
	pic14_emitcode("setb","c");
	while(sizer){
	  MOVA(aopGet(AOP(right),sizer-1,FALSE,FALSE));
	  if(sizer == 1)
	    // test the msb of the lsb
	    pic14_emitcode("anl","a,#0xfe");
	  pic14_emitcode("jnz","%05d_DS_",tlbl->key+100);
	  sizer--;
	}
	// val = (0,1)
	pic14_emitcode("rrc","a");
      }
      pic14_emitcode("jnb","%s,%05d_DS_",AOP(left)->aopu.aop_dir,(tlbl->key+100));
      pic14_emitcode("cpl","c");
      pic14_emitcode("","%05d_DS_:",(tlbl->key+100));
    }
    // bit = c
    // val = c
    if(size)
      pic14_outBitC(result);
    // if(bit | ...)
    else if((AOP_TYPE(result) == AOP_CRY) && ifx)
      genIfxJump(ifx, "c");           
    goto release ;
  }

  if(pic14_sameRegs(AOP(result),AOP(left))){
    /* if left is same as result */
    for(;size--; offset++) {
      if(AOP_TYPE(right) == AOP_LIT){
	int t  = (lit >> (offset*8)) & 0x0FFL;
	if(t == 0x00L)
	  continue;
	else
	  if (IS_AOP_PREG(left)) {
	    MOVA(aopGet(AOP(right),offset,FALSE,FALSE));
	    pic14_emitcode("xrl","a,%s",aopGet(AOP(left),offset,FALSE,TRUE));
	    aopPut(AOP(result),"a",offset);
	  } else {
	    emitpcode(POC_MOVLW, popGetLit(t));
	    emitpcode(POC_XORWF,popGet(AOP(left),offset));
	    pic14_emitcode("xrl","%s,%s",
			   aopGet(AOP(left),offset,FALSE,TRUE),
			   aopGet(AOP(right),offset,FALSE,FALSE));
	  }
      } else {
	if (AOP_TYPE(left) == AOP_ACC)
	  pic14_emitcode("xrl","a,%s",aopGet(AOP(right),offset,FALSE,FALSE));
	else {
	  emitpcode(POC_MOVFW,popGet(AOP(right),offset));
	  emitpcode(POC_XORWF,popGet(AOP(left),offset));
/*
	  if (IS_AOP_PREG(left)) {
	    pic14_emitcode("xrl","a,%s",aopGet(AOP(left),offset,FALSE,TRUE));
	    aopPut(AOP(result),"a",offset);
	  } else
	    pic14_emitcode("xrl","%s,a",
			   aopGet(AOP(left),offset,FALSE,TRUE));
*/
	}
      }
    }
  } else {
    // left & result in different registers
    if(AOP_TYPE(result) == AOP_CRY){
      // result = bit
      // if(size), result in bit
      // if(!size && ifx), conditional oper: if(left ^ right)
      symbol *tlbl = newiTempLabel(NULL);
      int sizer = max(AOP_SIZE(left),AOP_SIZE(right));
      if(size)
	pic14_emitcode("setb","c");
      while(sizer--){
	if((AOP_TYPE(right) == AOP_LIT) &&
	   (((lit >> (offset*8)) & 0x0FFL) == 0x00L)){
	  MOVA(aopGet(AOP(left),offset,FALSE,FALSE));
	} else {
	  MOVA(aopGet(AOP(right),offset,FALSE,FALSE));
	  pic14_emitcode("xrl","a,%s",
			 aopGet(AOP(left),offset,FALSE,FALSE));
	}
	pic14_emitcode("jnz","%05d_DS_",tlbl->key+100);
	offset++;
      }
      if(size){
	CLRC;
	pic14_emitcode("","%05d_DS_:",tlbl->key+100);
	pic14_outBitC(result);
      } else if(ifx)
	jmpTrueOrFalse(ifx, tlbl);
    } else for(;(size--);offset++){
      // normal case
      // result = left & right
      if(AOP_TYPE(right) == AOP_LIT){
	int t = (lit >> (offset*8)) & 0x0FFL;
	switch(t) { 
	case 0x00:
	  if (AOP_TYPE(left) != AOP_ACC) {
	    emitpcode(POC_MOVFW,popGet(AOP(left),offset));
	  }
	  emitpcode(POC_MOVWF,popGet(AOP(result),offset));
	  pic14_emitcode("movf","%s,w",
			 aopGet(AOP(left),offset,FALSE,FALSE));
	  pic14_emitcode("movwf","%s",
			 aopGet(AOP(result),offset,FALSE,FALSE));
	  break;
	case 0xff:
	  if (AOP_TYPE(left) == AOP_ACC) {
	    emitpcode(POC_XORLW, popGetLit(t));
	  } else {
	    emitpcode(POC_COMFW,popGet(AOP(left),offset));
	  }
	  emitpcode(POC_MOVWF,popGet(AOP(result),offset));
	  break;
	default:
	  if (AOP_TYPE(left) == AOP_ACC) {
	    emitpcode(POC_XORLW, popGetLit(t));
	  } else {
	    emitpcode(POC_MOVLW, popGetLit(t));
	    emitpcode(POC_XORFW,popGet(AOP(left),offset));
	  }
	  emitpcode(POC_MOVWF,popGet(AOP(result),offset));
	  pic14_emitcode("movlw","0x%x",t);
	  pic14_emitcode("xorwf","%s,w",
			 aopGet(AOP(left),offset,FALSE,FALSE));
	  pic14_emitcode("movwf","%s",
			 aopGet(AOP(result),offset,FALSE,FALSE));

	}
	continue;
      }

      // faster than result <- left, anl result,right
      // and better if result is SFR
      if (AOP_TYPE(left) == AOP_ACC) {
	emitpcode(POC_XORFW,popGet(AOP(right),offset));
	pic14_emitcode("xorwf","%s,w",aopGet(AOP(right),offset,FALSE,FALSE));
      } else {
	emitpcode(POC_MOVFW,popGet(AOP(right),offset));
	emitpcode(POC_XORFW,popGet(AOP(left),offset));
	pic14_emitcode("movf","%s,w",aopGet(AOP(right),offset,FALSE,FALSE));
	pic14_emitcode("xorwf","%s,w",aopGet(AOP(left),offset,FALSE,FALSE));
      }
      if ( AOP_TYPE(result) != AOP_ACC){
	emitpcode(POC_MOVWF,popGet(AOP(result),offset));
	pic14_emitcode("movwf","%s",aopGet(AOP(result),offset,FALSE,FALSE));
      }
    }
  }

  release :
    freeAsmop(left,NULL,ic,(RESULTONSTACK(ic) ? FALSE : TRUE));
  freeAsmop(right,NULL,ic,(RESULTONSTACK(ic) ? FALSE : TRUE));
  freeAsmop(result,NULL,ic,TRUE);     
}

/*-----------------------------------------------------------------*/
/* genInline - write the inline code out                           */
/*-----------------------------------------------------------------*/
static void genInline (iCode *ic)
{
    char *buffer, *bp, *bp1;
    
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

    _G.inLine += (!options.asmpeep);

    buffer = bp = bp1 = Safe_calloc(1, strlen(IC_INLINE(ic))+1);
    strcpy(buffer,IC_INLINE(ic));

    /* emit each line as a code */
    while (*bp) {
        if (*bp == '\n') {
            *bp++ = '\0';

	    if(*bp1)
	      addpCode2pBlock(pb,AssembleLine(bp1));
            bp1 = bp;
        } else {
            if (*bp == ':') {
                bp++;
                *bp = '\0';
                bp++;
                pic14_emitcode(bp1,"");
                bp1 = bp;
            } else
                bp++;
        }
    }
    if ((bp1 != bp) && *bp1)
      addpCode2pBlock(pb,AssembleLine(bp1));

    Safe_free(buffer);

    _G.inLine -= (!options.asmpeep);
}

/*-----------------------------------------------------------------*/
/* genRRC - rotate right with carry                                */
/*-----------------------------------------------------------------*/
static void genRRC (iCode *ic)
{
  operand *left , *result ;
  int size, offset = 0, same;

  /* rotate right with carry */
  left = IC_LEFT(ic);
  result=IC_RESULT(ic);
  aopOp (left,ic,FALSE);
  aopOp (result,ic,FALSE);

  DEBUGpic14_AopType(__LINE__,left,NULL,result);

  same = pic14_sameRegs(AOP(result),AOP(left));

  size = AOP_SIZE(result);    

  /* get the lsb and put it into the carry */
  emitpcode(POC_RRFW, popGet(AOP(left),size-1));

  offset = 0 ;

  while(size--) {

    if(same) {
      emitpcode(POC_RRF, popGet(AOP(left),offset));
    } else {
      emitpcode(POC_RRFW, popGet(AOP(left),offset));
      emitpcode(POC_MOVWF, popGet(AOP(result),offset));
    }

    offset++;
  }

  freeAsmop(left,NULL,ic,TRUE);
  freeAsmop(result,NULL,ic,TRUE);
}

/*-----------------------------------------------------------------*/
/* genRLC - generate code for rotate left with carry               */
/*-----------------------------------------------------------------*/
static void genRLC (iCode *ic)
{    
  operand *left , *result ;
  int size, offset = 0;
  int same;

  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
  /* rotate right with carry */
  left = IC_LEFT(ic);
  result=IC_RESULT(ic);
  aopOp (left,ic,FALSE);
  aopOp (result,ic,FALSE);

  DEBUGpic14_AopType(__LINE__,left,NULL,result);

  same = pic14_sameRegs(AOP(result),AOP(left));

  /* move it to the result */
  size = AOP_SIZE(result);    

  /* get the msb and put it into the carry */
  emitpcode(POC_RLFW, popGet(AOP(left),size-1));

  offset = 0 ;

  while(size--) {

    if(same) {
      emitpcode(POC_RLF, popGet(AOP(left),offset));
    } else {
      emitpcode(POC_RLFW, popGet(AOP(left),offset));
      emitpcode(POC_MOVWF, popGet(AOP(result),offset));
    }

    offset++;
  }


  freeAsmop(left,NULL,ic,TRUE);
  freeAsmop(result,NULL,ic,TRUE);
}

/*-----------------------------------------------------------------*/
/* genGetHbit - generates code get highest order bit               */
/*-----------------------------------------------------------------*/
static void genGetHbit (iCode *ic)
{
    operand *left, *result;
    left = IC_LEFT(ic);
    result=IC_RESULT(ic);
    aopOp (left,ic,FALSE);
    aopOp (result,ic,FALSE);

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    /* get the highest order byte into a */
    MOVA(aopGet(AOP(left),AOP_SIZE(left) - 1,FALSE,FALSE));
    if(AOP_TYPE(result) == AOP_CRY){
        pic14_emitcode("rlc","a");
        pic14_outBitC(result);
    }
    else{
        pic14_emitcode("rl","a");
        pic14_emitcode("anl","a,#0x01");
        pic14_outAcc(result);
    }


    freeAsmop(left,NULL,ic,TRUE);
    freeAsmop(result,NULL,ic,TRUE);
}

/*-----------------------------------------------------------------*/
/* AccRol - rotate left accumulator by known count                 */
/*-----------------------------------------------------------------*/
static void AccRol (int shCount)
{
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    shCount &= 0x0007;              // shCount : 0..7
    switch(shCount){
        case 0 :
            break;
        case 1 :
            pic14_emitcode("rl","a");
            break;
        case 2 :
            pic14_emitcode("rl","a");
            pic14_emitcode("rl","a");
            break;
        case 3 :
            pic14_emitcode("swap","a");
            pic14_emitcode("rr","a");
            break;
        case 4 :
            pic14_emitcode("swap","a");
            break;
        case 5 :
            pic14_emitcode("swap","a");
            pic14_emitcode("rl","a");
            break;
        case 6 :
            pic14_emitcode("rr","a");
            pic14_emitcode("rr","a");
            break;
        case 7 :
            pic14_emitcode("rr","a");
            break;
    }
}

/*-----------------------------------------------------------------*/
/* AccLsh - left shift accumulator by known count                  */
/*-----------------------------------------------------------------*/
static void AccLsh (int shCount)
{
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    if(shCount != 0){
        if(shCount == 1)
            pic14_emitcode("add","a,acc");
        else 
	    if(shCount == 2) {
            pic14_emitcode("add","a,acc");
            pic14_emitcode("add","a,acc");
        } else {
            /* rotate left accumulator */
            AccRol(shCount);
            /* and kill the lower order bits */
            pic14_emitcode("anl","a,#0x%02x", SLMask[shCount]);
        }
    }
}

/*-----------------------------------------------------------------*/
/* AccRsh - right shift accumulator by known count                 */
/*-----------------------------------------------------------------*/
static void AccRsh (int shCount)
{
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    if(shCount != 0){
        if(shCount == 1){
            CLRC;
            pic14_emitcode("rrc","a");
        } else {
            /* rotate right accumulator */
            AccRol(8 - shCount);
            /* and kill the higher order bits */
            pic14_emitcode("anl","a,#0x%02x", SRMask[shCount]);
        }
    }
}

#if 0
/*-----------------------------------------------------------------*/
/* AccSRsh - signed right shift accumulator by known count                 */
/*-----------------------------------------------------------------*/
static void AccSRsh (int shCount)
{
    symbol *tlbl ;
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    if(shCount != 0){
        if(shCount == 1){
            pic14_emitcode("mov","c,acc.7");
            pic14_emitcode("rrc","a");
        } else if(shCount == 2){
            pic14_emitcode("mov","c,acc.7");
            pic14_emitcode("rrc","a");
            pic14_emitcode("mov","c,acc.7");
            pic14_emitcode("rrc","a");
        } else {
            tlbl = newiTempLabel(NULL);
            /* rotate right accumulator */
            AccRol(8 - shCount);
            /* and kill the higher order bits */
            pic14_emitcode("anl","a,#0x%02x", SRMask[shCount]);
            pic14_emitcode("jnb","acc.%d,%05d_DS_",7-shCount,tlbl->key+100);
            pic14_emitcode("orl","a,#0x%02x",
                     (unsigned char)~SRMask[shCount]);
            pic14_emitcode("","%05d_DS_:",tlbl->key+100);
        }
    }
}
#endif
/*-----------------------------------------------------------------*/
/* shiftR1Left2Result - shift right one byte from left to result   */
/*-----------------------------------------------------------------*/
static void shiftR1Left2ResultSigned (operand *left, int offl,
                                operand *result, int offr,
                                int shCount)
{
  int same;

  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

  same = ((left == result) || (AOP(left) == AOP(result))) && (offl == offr);

  switch(shCount) {
  case 1:
    emitpcode(POC_RLFW, popGet(AOP(left),offl));
    if(same) 
      emitpcode(POC_RRF, popGet(AOP(result),offr));
    else {
      emitpcode(POC_RRFW, popGet(AOP(left),offl));
      emitpcode(POC_MOVWF, popGet(AOP(result),offr));
    }

    break;
  case 2:

    emitpcode(POC_RLFW, popGet(AOP(left),offl));
    if(same) 
      emitpcode(POC_RRF, popGet(AOP(result),offr));
    else {
      emitpcode(POC_RRFW, popGet(AOP(left),offl));
      emitpcode(POC_MOVWF, popGet(AOP(result),offr));
    }
    emitpcode(POC_RLFW, popGet(AOP(result),offr));
    emitpcode(POC_RRF,  popGet(AOP(result),offr));

    break;

  case 3:
    if(same)
      emitpcode(POC_SWAPF, popGet(AOP(result),offr));
    else {
      emitpcode(POC_SWAPFW, popGet(AOP(left),offl));
      emitpcode(POC_MOVWF, popGet(AOP(result),offr));
    }

    emitpcode(POC_RLFW,  popGet(AOP(result),offr));
    emitpcode(POC_RLFW,  popGet(AOP(result),offr));
    emitpcode(POC_ANDLW, popGetLit(0x1f));

    emitpcode(POC_BTFSC, newpCodeOpBit(aopGet(AOP(result),offr,FALSE,FALSE),3,0));
    emitpcode(POC_IORLW, popGetLit(0xe0));

    emitpcode(POC_MOVWF, popGet(AOP(result),offr));
    break;

  case 4:
    emitpcode(POC_SWAPFW, popGet(AOP(left),offl));
    emitpcode(POC_ANDLW,  popGetLit(0x0f));
    emitpcode(POC_BTFSC,  newpCodeOpBit(aopGet(AOP(left),offl,FALSE,FALSE),7,0));
    emitpcode(POC_IORLW,  popGetLit(0xf0));
    emitpcode(POC_MOVWF,  popGet(AOP(result),offr));
    break;
  case 5:
    if(same) {
      emitpcode(POC_SWAPF,  popGet(AOP(result),offr));
    } else {
      emitpcode(POC_SWAPFW,  popGet(AOP(left),offl));
      emitpcode(POC_MOVWF,  popGet(AOP(result),offr));
    }
    emitpcode(POC_RRFW,   popGet(AOP(result),offr));
    emitpcode(POC_ANDLW,  popGetLit(0x07));
    emitpcode(POC_BTFSC,  newpCodeOpBit(aopGet(AOP(result),offr,FALSE,FALSE),3,0));
    emitpcode(POC_IORLW,  popGetLit(0xf8));
    emitpcode(POC_MOVWF,  popGet(AOP(result),offr));
    break;

  case 6:
    if(same) {
      emitpcode(POC_MOVLW, popGetLit(0x00));
      emitpcode(POC_BTFSC, newpCodeOpBit(aopGet(AOP(left),offl,FALSE,FALSE),7,0));
      emitpcode(POC_MOVLW, popGetLit(0xfe));
      emitpcode(POC_BTFSC, newpCodeOpBit(aopGet(AOP(left),offl,FALSE,FALSE),6,0));
      emitpcode(POC_IORLW, popGetLit(0x01));
      emitpcode(POC_MOVWF, popGet(AOP(result),offr));
    } else {
      emitpcode(POC_CLRF,  popGet(AOP(result),offr));
      emitpcode(POC_BTFSC, newpCodeOpBit(aopGet(AOP(left),offl,FALSE,FALSE),7,0));
      emitpcode(POC_DECF,  popGet(AOP(result),offr));
      emitpcode(POC_BTFSS, newpCodeOpBit(aopGet(AOP(left),offl,FALSE,FALSE),6,0));
      emitpcode(POC_BCF,   newpCodeOpBit(aopGet(AOP(result),offr,FALSE,FALSE),0,0));
    }
    break;

  case 7:
    if(same) {
      emitpcode(POC_MOVLW, popGetLit(0x00));
      emitpcode(POC_BTFSC, newpCodeOpBit(aopGet(AOP(left),LSB,FALSE,FALSE),7,0));
      emitpcode(POC_MOVLW, popGetLit(0xff));
      emitpcode(POC_MOVWF, popGet(AOP(result),offr));
    } else {
      emitpcode(POC_CLRF,  popGet(AOP(result),offr));
      emitpcode(POC_BTFSC, newpCodeOpBit(aopGet(AOP(left),offl,FALSE,FALSE),7,0));
      emitpcode(POC_DECF,  popGet(AOP(result),offr));
    }

  default:
    break;
  }
}

/*-----------------------------------------------------------------*/
/* shiftR1Left2Result - shift right one byte from left to result   */
/*-----------------------------------------------------------------*/
static void shiftR1Left2Result (operand *left, int offl,
                                operand *result, int offr,
                                int shCount, int sign)
{
  int same;

  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

  same = ((left == result) || (AOP(left) == AOP(result))) && (offl == offr);

  /* Copy the msb into the carry if signed. */
  if(sign) {
    shiftR1Left2ResultSigned(left,offl,result,offr,shCount);
    return;
  }



  switch(shCount) {
  case 1:
    emitCLRC;
    if(same) 
      emitpcode(POC_RRF, popGet(AOP(result),offr));
    else {
      emitpcode(POC_RRFW, popGet(AOP(left),offl));
      emitpcode(POC_MOVWF, popGet(AOP(result),offr));
    }
    break;
  case 2:
    emitCLRC;
    if(same) {
      emitpcode(POC_RRF, popGet(AOP(result),offr));
    } else {
      emitpcode(POC_RRFW, popGet(AOP(left),offl));
      emitpcode(POC_MOVWF, popGet(AOP(result),offr));
    }
    emitCLRC;
    emitpcode(POC_RRF, popGet(AOP(result),offr));

    break;
  case 3:
    if(same)
      emitpcode(POC_SWAPF, popGet(AOP(result),offr));
    else {
      emitpcode(POC_SWAPFW, popGet(AOP(left),offl));
      emitpcode(POC_MOVWF, popGet(AOP(result),offr));
    }

    emitpcode(POC_RLFW,  popGet(AOP(result),offr));
    emitpcode(POC_RLFW,  popGet(AOP(result),offr));
    emitpcode(POC_ANDLW, popGetLit(0x1f));
    emitpcode(POC_MOVWF, popGet(AOP(result),offr));
    break;
      
  case 4:
    emitpcode(POC_SWAPFW, popGet(AOP(left),offl));
    emitpcode(POC_ANDLW, popGetLit(0x0f));
    emitpcode(POC_MOVWF, popGet(AOP(result),offr));
    break;

  case 5:
    emitpcode(POC_SWAPFW, popGet(AOP(left),offl));
    emitpcode(POC_ANDLW, popGetLit(0x0f));
    emitpcode(POC_MOVWF, popGet(AOP(result),offr));
    emitCLRC;
    emitpcode(POC_RRF, popGet(AOP(result),offr));

    break;
  case 6:

    emitpcode(POC_RLFW,  popGet(AOP(left),offl));
    emitpcode(POC_ANDLW, popGetLit(0x80));
    emitpcode(POC_MOVWF, popGet(AOP(result),offr));
    emitpcode(POC_RLF,   popGet(AOP(result),offr));
    emitpcode(POC_RLF,   popGet(AOP(result),offr));
    break;

  case 7:

    emitpcode(POC_RLFW, popGet(AOP(left),offl));
    emitpcode(POC_CLRF, popGet(AOP(result),offr));
    emitpcode(POC_RLF,  popGet(AOP(result),offr));

    break;

  default:
    break;
  }
}

/*-----------------------------------------------------------------*/
/* shiftL1Left2Result - shift left one byte from left to result    */
/*-----------------------------------------------------------------*/
static void shiftL1Left2Result (operand *left, int offl,
                                operand *result, int offr, int shCount)
{
  int same;

  //    char *l;
  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

  same = ((left == result) || (AOP(left) == AOP(result))) && (offl==offr);
  DEBUGpic14_emitcode ("; ***","same =  %d",same);
    //    l = aopGet(AOP(left),offl,FALSE,FALSE);
    //    MOVA(l);
    /* shift left accumulator */
    //AccLsh(shCount); // don't comment out just yet...
  //    aopPut(AOP(result),"a",offr);

  switch(shCount) {
  case 1:
    /* Shift left 1 bit position */
    emitpcode(POC_MOVFW, popGet(AOP(left),offl));
    if(same) {
      emitpcode(POC_ADDWF, popGet(AOP(left),offl));
    } else {
      emitpcode(POC_ADDFW, popGet(AOP(left),offl));
      emitpcode(POC_MOVWF, popGet(AOP(result),offr));
    }
    break;
  case 2:
    emitpcode(POC_RLFW, popGet(AOP(left),offl));
    emitpcode(POC_ANDLW,popGetLit(0x7e));
    emitpcode(POC_MOVWF,popGet(AOP(result),offr));
    emitpcode(POC_ADDWF,popGet(AOP(result),offr));
    break;
  case 3:
    emitpcode(POC_RLFW, popGet(AOP(left),offl));
    emitpcode(POC_ANDLW,popGetLit(0x3e));
    emitpcode(POC_MOVWF,popGet(AOP(result),offr));
    emitpcode(POC_ADDWF,popGet(AOP(result),offr));
    emitpcode(POC_RLF,  popGet(AOP(result),offr));
    break;
  case 4:
    emitpcode(POC_SWAPFW,popGet(AOP(left),offl));
    emitpcode(POC_ANDLW, popGetLit(0xf0));
    emitpcode(POC_MOVWF,popGet(AOP(result),offr));
    break;
  case 5:
    emitpcode(POC_SWAPFW,popGet(AOP(left),offl));
    emitpcode(POC_ANDLW, popGetLit(0xf0));
    emitpcode(POC_MOVWF,popGet(AOP(result),offr));
    emitpcode(POC_ADDWF,popGet(AOP(result),offr));
    break;
  case 6:
    emitpcode(POC_SWAPFW,popGet(AOP(left),offl));
    emitpcode(POC_ANDLW, popGetLit(0x30));
    emitpcode(POC_MOVWF,popGet(AOP(result),offr));
    emitpcode(POC_ADDWF,popGet(AOP(result),offr));
    emitpcode(POC_RLF,  popGet(AOP(result),offr));
    break;
  case 7:
    emitpcode(POC_RRFW, popGet(AOP(left),offl));
    emitpcode(POC_CLRF, popGet(AOP(result),offr));
    emitpcode(POC_RRF,  popGet(AOP(result),offr));
    break;

  default:
    DEBUGpic14_emitcode ("; ***","%s  %d, shift count is %d",__FUNCTION__,__LINE__,shCount);
  }

}

/*-----------------------------------------------------------------*/
/* movLeft2Result - move byte from left to result                  */
/*-----------------------------------------------------------------*/
static void movLeft2Result (operand *left, int offl,
                            operand *result, int offr)
{
  char *l;
  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
  if(!pic14_sameRegs(AOP(left),AOP(result)) || (offl != offr)){
    l = aopGet(AOP(left),offl,FALSE,FALSE);

    if (*l == '@' && (IS_AOP_PREG(result))) {
      pic14_emitcode("mov","a,%s",l);
      aopPut(AOP(result),"a",offr);
    } else {
      emitpcode(POC_MOVFW, popGet(AOP(left),offl));
      emitpcode(POC_MOVWF, popGet(AOP(result),offr));
    }
  }
}

/*-----------------------------------------------------------------*/
/* shiftL2Left2Result - shift left two bytes from left to result   */
/*-----------------------------------------------------------------*/
static void shiftL2Left2Result (operand *left, int offl,
                                operand *result, int offr, int shCount)
{


  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

  if(pic14_sameRegs(AOP(result), AOP(left))) {
    switch(shCount) {
    case 0:
      break;
    case 1:
    case 2:
    case 3:

      emitpcode(POC_MOVFW,popGet(AOP(result),offr));
      emitpcode(POC_ADDWF,popGet(AOP(result),offr));
      emitpcode(POC_RLF,  popGet(AOP(result),offr+MSB16));

      while(--shCount) {
	emitCLRC;
	emitpcode(POC_RLF, popGet(AOP(result),offr));
	emitpcode(POC_RLF, popGet(AOP(result),offr+MSB16));
      }

      break;
    case 4:
    case 5:
      emitpcode(POC_MOVLW, popGetLit(0x0f));
      emitpcode(POC_ANDWF, popGet(AOP(result),offr+MSB16));
      emitpcode(POC_SWAPF, popGet(AOP(result),offr+MSB16));
      emitpcode(POC_SWAPF, popGet(AOP(result),offr));
      emitpcode(POC_ANDFW, popGet(AOP(result),offr));
      emitpcode(POC_XORWF, popGet(AOP(result),offr));
      emitpcode(POC_ADDWF, popGet(AOP(result),offr+MSB16));
      if(shCount >=5) {
	emitpcode(POC_RLF, popGet(AOP(result),offr));
	emitpcode(POC_RLF, popGet(AOP(result),offr+MSB16));
      }
      break;
    case 6:
      emitpcode(POC_RRF,  popGet(AOP(result),offr+MSB16));
      emitpcode(POC_RRF,  popGet(AOP(result),offr));
      emitpcode(POC_RRF,  popGet(AOP(result),offr+MSB16));
      emitpcode(POC_RRF,  popGet(AOP(result),offr));
      emitpcode(POC_RRFW, popGet(AOP(result),offr+MSB16));
      emitpcode(POC_ANDLW,popGetLit(0xc0));
      emitpcode(POC_XORFW,popGet(AOP(result),offr));
      emitpcode(POC_XORWF,popGet(AOP(result),offr));
      emitpcode(POC_XORFW,popGet(AOP(result),offr));
      emitpcode(POC_MOVWF,popGet(AOP(result),offr+MSB16));
      break;
    case 7:
      emitpcode(POC_RRFW, popGet(AOP(result),offr+MSB16));
      emitpcode(POC_RRFW, popGet(AOP(result),offr));
      emitpcode(POC_MOVWF,popGet(AOP(result),offr+MSB16));
      emitpcode(POC_CLRF, popGet(AOP(result),offr));
      emitpcode(POC_RRF,  popGet(AOP(result),offr));
    }

  } else {
    switch(shCount) {
    case 0:
      break;
    case 1:
    case 2:
    case 3:
      /* note, use a mov/add for the shift since the mov has a
	 chance of getting optimized out */
      emitpcode(POC_MOVFW, popGet(AOP(left),offl));
      emitpcode(POC_MOVWF, popGet(AOP(result),offr));
      emitpcode(POC_ADDWF, popGet(AOP(result),offr));
      emitpcode(POC_RLFW,  popGet(AOP(left),offl+MSB16));
      emitpcode(POC_MOVWF, popGet(AOP(result),offr+MSB16));

      while(--shCount) {
	emitCLRC;
	emitpcode(POC_RLF, popGet(AOP(result),offr));
	emitpcode(POC_RLF, popGet(AOP(result),offr+MSB16));
      }
      break;

    case 4:
    case 5:
      emitpcode(POC_SWAPFW,popGet(AOP(left),offl+MSB16));
      emitpcode(POC_ANDLW, popGetLit(0xF0));
      emitpcode(POC_MOVWF, popGet(AOP(result),offr+MSB16));
      emitpcode(POC_SWAPFW,popGet(AOP(left),offl));
      emitpcode(POC_MOVWF, popGet(AOP(result),offr));
      emitpcode(POC_ANDLW, popGetLit(0xF0));
      emitpcode(POC_XORWF, popGet(AOP(result),offr));
      emitpcode(POC_ADDWF, popGet(AOP(result),offr+MSB16));


      if(shCount == 5) {
	emitpcode(POC_RLF, popGet(AOP(result),offr));
	emitpcode(POC_RLF, popGet(AOP(result),offr+MSB16));
      }
      break;
    case 6:
      emitpcode(POC_RRFW, popGet(AOP(left),offl+MSB16));
      emitpcode(POC_MOVWF,popGet(AOP(result),offr+MSB16));
      emitpcode(POC_RRFW, popGet(AOP(result),offl));
      emitpcode(POC_MOVWF,  popGet(AOP(result),offr));

      emitpcode(POC_RRF,  popGet(AOP(result),offr+MSB16));
      emitpcode(POC_RRF,  popGet(AOP(result),offr));
      emitpcode(POC_RRFW, popGet(AOP(result),offr+MSB16));
      emitpcode(POC_ANDLW,popGetLit(0xc0));
      emitpcode(POC_XORFW,popGet(AOP(result),offr));
      emitpcode(POC_XORWF,popGet(AOP(result),offr));
      emitpcode(POC_XORFW,popGet(AOP(result),offr));
      emitpcode(POC_MOVWF,popGet(AOP(result),offr+MSB16));
      break;
    case 7:
      emitpcode(POC_RRFW, popGet(AOP(left),offl+MSB16));
      emitpcode(POC_RRFW, popGet(AOP(left),offl));
      emitpcode(POC_MOVWF,popGet(AOP(result),offr+MSB16));
      emitpcode(POC_CLRF, popGet(AOP(result),offr));
      emitpcode(POC_RRF,  popGet(AOP(result),offr));
    }
  }

}
/*-----------------------------------------------------------------*/
/* shiftR2Left2Result - shift right two bytes from left to result  */
/*-----------------------------------------------------------------*/
static void shiftR2Left2Result (operand *left, int offl,
                                operand *result, int offr,
                                int shCount, int sign)
{
  int same=0;

  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
  same = pic14_sameRegs(AOP(result), AOP(left));

  if(same && ((offl + MSB16) == offr)){
    same=1;
    /* don't crash result[offr] */
    MOVA(aopGet(AOP(left),offl,FALSE,FALSE));
    pic14_emitcode("xch","a,%s", aopGet(AOP(left),offl+MSB16,FALSE,FALSE));
  }
/* else {
    movLeft2Result(left,offl, result, offr);
    MOVA(aopGet(AOP(left),offl+MSB16,FALSE,FALSE));
  }
*/
  /* a:x >> shCount (x = lsb(result))*/
/*
  if(sign)
    AccAXRshS( aopGet(AOP(result),offr,FALSE,FALSE) , shCount);
  else {
    AccAXRsh( aopGet(AOP(result),offr,FALSE,FALSE) , shCount);
*/
  switch(shCount) {
  case 0:
    break;
  case 1:
  case 2:
  case 3:
    if(sign)
      emitpcode(POC_RLFW,popGet(AOP(result),offr+MSB16));
    else
      emitCLRC;

    if(same) {
      emitpcode(POC_RRF,popGet(AOP(result),offr+MSB16));
      emitpcode(POC_RRF,popGet(AOP(result),offr));
    } else {
      emitpcode(POC_RRFW, popGet(AOP(left),offl+MSB16));
      emitpcode(POC_MOVWF,popGet(AOP(result),offr+MSB16));
      emitpcode(POC_RRFW, popGet(AOP(left),offl));
      emitpcode(POC_MOVWF,popGet(AOP(result),offr));
    }

    while(--shCount) {
      if(sign)
	emitpcode(POC_RLFW,popGet(AOP(result),offr+MSB16));
      else
	emitCLRC;
      emitpcode(POC_RRF,popGet(AOP(result),offr+MSB16));
      emitpcode(POC_RRF,popGet(AOP(result),offr));
    }
    break;
  case 4:
  case 5:
    if(same) {

      emitpcode(POC_MOVLW, popGetLit(0xf0));
      emitpcode(POC_ANDWF, popGet(AOP(result),offr));
      emitpcode(POC_SWAPF, popGet(AOP(result),offr));

      emitpcode(POC_SWAPF, popGet(AOP(result),offr+MSB16));
      emitpcode(POC_ANDFW, popGet(AOP(result),offr+MSB16));
      emitpcode(POC_XORWF, popGet(AOP(result),offr+MSB16));
      emitpcode(POC_ADDWF, popGet(AOP(result),offr));
    } else {
      emitpcode(POC_SWAPFW,popGet(AOP(left),offl));
      emitpcode(POC_ANDLW, popGetLit(0x0f));
      emitpcode(POC_MOVWF, popGet(AOP(result),offr));

      emitpcode(POC_SWAPFW,popGet(AOP(left),offl+MSB16));
      emitpcode(POC_MOVWF, popGet(AOP(result),offr+MSB16));
      emitpcode(POC_ANDLW, popGetLit(0xf0));
      emitpcode(POC_XORWF, popGet(AOP(result),offr+MSB16));
      emitpcode(POC_ADDWF, popGet(AOP(result),offr));
    }

    if(shCount >=5) {
      emitpcode(POC_RRF, popGet(AOP(result),offr+MSB16));
      emitpcode(POC_RRF, popGet(AOP(result),offr));
    }

    if(sign) {
      emitpcode(POC_MOVLW, popGetLit(0xf0 + (shCount-4)*8 ));
      emitpcode(POC_BTFSC, 
		newpCodeOpBit(aopGet(AOP(result),offr+MSB16,FALSE,FALSE),7-shCount,0));
      emitpcode(POC_ADDWF, popGet(AOP(result),offr+MSB16));
    }

    break;

  case 6:
    if(same) {

      emitpcode(POC_RLF,  popGet(AOP(result),offr));
      emitpcode(POC_RLF,  popGet(AOP(result),offr+MSB16));

      emitpcode(POC_RLF,  popGet(AOP(result),offr));
      emitpcode(POC_RLF,  popGet(AOP(result),offr+MSB16));
      emitpcode(POC_RLFW, popGet(AOP(result),offr));
      emitpcode(POC_ANDLW,popGetLit(0x03));
      if(sign) {
	emitpcode(POC_BTFSC, 
		  newpCodeOpBit(aopGet(AOP(result),offr+MSB16,FALSE,FALSE),1,0));
	emitpcode(POC_IORLW,popGetLit(0xfc));
      }
      emitpcode(POC_XORFW,popGet(AOP(result),offr+MSB16));
      emitpcode(POC_XORWF,popGet(AOP(result),offr+MSB16));
      emitpcode(POC_XORFW,popGet(AOP(result),offr+MSB16));
      emitpcode(POC_MOVWF,popGet(AOP(result),offr));
    } else {
      emitpcode(POC_RLFW, popGet(AOP(left),offl));
      emitpcode(POC_MOVWF,popGet(AOP(result),offr+MSB16));
      emitpcode(POC_RLFW, popGet(AOP(left),offl+MSB16));
      emitpcode(POC_MOVWF,popGet(AOP(result),offr));
      emitpcode(POC_RLF,  popGet(AOP(result),offr+MSB16));
      emitpcode(POC_RLF,  popGet(AOP(result),offr));
      emitpcode(POC_RLFW, popGet(AOP(result),offr+MSB16));
      emitpcode(POC_ANDLW,popGetLit(0x03));
      if(sign) {
	emitpcode(POC_BTFSC, 
		  newpCodeOpBit(aopGet(AOP(result),offr+MSB16,FALSE,FALSE),0,0));
	emitpcode(POC_IORLW,popGetLit(0xfc));
      }
      emitpcode(POC_MOVWF,popGet(AOP(result),offr+MSB16));
      //emitpcode(POC_RLF,  popGet(AOP(result),offr));

	
    }

    break;
  case 7:
    emitpcode(POC_RLFW, popGet(AOP(left),offl));
    emitpcode(POC_RLFW, popGet(AOP(left),offl+MSB16));
    emitpcode(POC_MOVWF,popGet(AOP(result),offr));
    emitpcode(POC_CLRF, popGet(AOP(result),offr+MSB16));
    if(sign) {
      emitSKPNC;
      emitpcode(POC_DECF, popGet(AOP(result),offr+MSB16));
    } else 
      emitpcode(POC_RLF,  popGet(AOP(result),offr+MSB16));
  }
}


/*-----------------------------------------------------------------*/
/* shiftLLeftOrResult - shift left one byte from left, or to result*/
/*-----------------------------------------------------------------*/
static void shiftLLeftOrResult (operand *left, int offl,
                                operand *result, int offr, int shCount)
{
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    MOVA(aopGet(AOP(left),offl,FALSE,FALSE));
    /* shift left accumulator */
    AccLsh(shCount);
    /* or with result */
    pic14_emitcode("orl","a,%s", aopGet(AOP(result),offr,FALSE,FALSE));
    /* back to result */
    aopPut(AOP(result),"a",offr);
}

/*-----------------------------------------------------------------*/
/* shiftRLeftOrResult - shift right one byte from left,or to result*/
/*-----------------------------------------------------------------*/
static void shiftRLeftOrResult (operand *left, int offl,
                                operand *result, int offr, int shCount)
{
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    MOVA(aopGet(AOP(left),offl,FALSE,FALSE));
    /* shift right accumulator */
    AccRsh(shCount);
    /* or with result */
    pic14_emitcode("orl","a,%s", aopGet(AOP(result),offr,FALSE,FALSE));
    /* back to result */
    aopPut(AOP(result),"a",offr);
}

/*-----------------------------------------------------------------*/
/* genlshOne - left shift a one byte quantity by known count       */
/*-----------------------------------------------------------------*/
static void genlshOne (operand *result, operand *left, int shCount)
{       
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    shiftL1Left2Result(left, LSB, result, LSB, shCount);
}

/*-----------------------------------------------------------------*/
/* genlshTwo - left shift two bytes by known amount != 0           */
/*-----------------------------------------------------------------*/
static void genlshTwo (operand *result,operand *left, int shCount)
{
    int size;
    
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    size = pic14_getDataSize(result);

    /* if shCount >= 8 */
    if (shCount >= 8) {
        shCount -= 8 ;

        if (size > 1){
            if (shCount)
                shiftL1Left2Result(left, LSB, result, MSB16, shCount);
            else 
                movLeft2Result(left, LSB, result, MSB16);
        }
	emitpcode(POC_CLRF,popGet(AOP(result),LSB));
    }

    /*  1 <= shCount <= 7 */
    else {  
        if(size == 1)
            shiftL1Left2Result(left, LSB, result, LSB, shCount); 
        else 
            shiftL2Left2Result(left, LSB, result, LSB, shCount);
    }
}

/*-----------------------------------------------------------------*/
/* shiftLLong - shift left one long from left to result            */
/* offl = LSB or MSB16                                             */
/*-----------------------------------------------------------------*/
static void shiftLLong (operand *left, operand *result, int offr )
{
    char *l;
    int size = AOP_SIZE(result);

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    if(size >= LSB+offr){
        l = aopGet(AOP(left),LSB,FALSE,FALSE);
        MOVA(l);
        pic14_emitcode("add","a,acc");
	if (pic14_sameRegs(AOP(left),AOP(result)) && 
	    size >= MSB16+offr && offr != LSB )
	    pic14_emitcode("xch","a,%s",
		     aopGet(AOP(left),LSB+offr,FALSE,FALSE));
	else	    
	    aopPut(AOP(result),"a",LSB+offr);
    }

    if(size >= MSB16+offr){
	if (!(pic14_sameRegs(AOP(result),AOP(left)) && size >= MSB16+offr && offr != LSB) ) {
	    l = aopGet(AOP(left),MSB16,FALSE,FALSE);
	    MOVA(l);
	}
        pic14_emitcode("rlc","a");
	if (pic14_sameRegs(AOP(left),AOP(result)) && 
	    size >= MSB24+offr && offr != LSB)
	    pic14_emitcode("xch","a,%s",
		     aopGet(AOP(left),MSB16+offr,FALSE,FALSE));
	else	    
	    aopPut(AOP(result),"a",MSB16+offr);
    }

    if(size >= MSB24+offr){
	if (!(pic14_sameRegs(AOP(left),AOP(left)) && size >= MSB24+offr && offr != LSB)) {
	    l = aopGet(AOP(left),MSB24,FALSE,FALSE);
	    MOVA(l);
	}
        pic14_emitcode("rlc","a");
	if (pic14_sameRegs(AOP(left),AOP(result)) && 
	    size >= MSB32+offr && offr != LSB )
	    pic14_emitcode("xch","a,%s",
		     aopGet(AOP(left),MSB24+offr,FALSE,FALSE));
	else	    
	    aopPut(AOP(result),"a",MSB24+offr);
    }

    if(size > MSB32+offr){
	if (!(pic14_sameRegs(AOP(result),AOP(left)) && size >= MSB32+offr && offr != LSB)) {
	    l = aopGet(AOP(left),MSB32,FALSE,FALSE);
	    MOVA(l);	
	}
        pic14_emitcode("rlc","a");
        aopPut(AOP(result),"a",MSB32+offr);
    }
    if(offr != LSB)
        aopPut(AOP(result),zero,LSB);       
}

/*-----------------------------------------------------------------*/
/* genlshFour - shift four byte by a known amount != 0             */
/*-----------------------------------------------------------------*/
static void genlshFour (operand *result, operand *left, int shCount)
{
    int size;

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    size = AOP_SIZE(result);

    /* if shifting more that 3 bytes */
    if (shCount >= 24 ) {
        shCount -= 24;
        if (shCount)
            /* lowest order of left goes to the highest
            order of the destination */
            shiftL1Left2Result(left, LSB, result, MSB32, shCount);
        else
            movLeft2Result(left, LSB, result, MSB32);
        aopPut(AOP(result),zero,LSB);
        aopPut(AOP(result),zero,MSB16);
        aopPut(AOP(result),zero,MSB32);
        return;
    }

    /* more than two bytes */
    else if ( shCount >= 16 ) {
        /* lower order two bytes goes to higher order two bytes */
        shCount -= 16;
        /* if some more remaining */
        if (shCount)
            shiftL2Left2Result(left, LSB, result, MSB24, shCount);
        else {
            movLeft2Result(left, MSB16, result, MSB32);
            movLeft2Result(left, LSB, result, MSB24);
        }
        aopPut(AOP(result),zero,MSB16);
        aopPut(AOP(result),zero,LSB);
        return;
    }    

    /* if more than 1 byte */
    else if ( shCount >= 8 ) {
        /* lower order three bytes goes to higher order  three bytes */
        shCount -= 8;
        if(size == 2){
            if(shCount)
                shiftL1Left2Result(left, LSB, result, MSB16, shCount);
            else
                movLeft2Result(left, LSB, result, MSB16);
        }
        else{   /* size = 4 */
            if(shCount == 0){
                movLeft2Result(left, MSB24, result, MSB32);
                movLeft2Result(left, MSB16, result, MSB24);
                movLeft2Result(left, LSB, result, MSB16);
                aopPut(AOP(result),zero,LSB);
            }
            else if(shCount == 1)
                shiftLLong(left, result, MSB16);
            else{
                shiftL2Left2Result(left, MSB16, result, MSB24, shCount);
                shiftL1Left2Result(left, LSB, result, MSB16, shCount);
                shiftRLeftOrResult(left, LSB, result, MSB24, 8 - shCount);
                aopPut(AOP(result),zero,LSB);
            }
        }
    }

    /* 1 <= shCount <= 7 */
    else if(shCount <= 2){
        shiftLLong(left, result, LSB);
        if(shCount == 2)
            shiftLLong(result, result, LSB);
    }
    /* 3 <= shCount <= 7, optimize */
    else{
        shiftL2Left2Result(left, MSB24, result, MSB24, shCount);
        shiftRLeftOrResult(left, MSB16, result, MSB24, 8 - shCount);
        shiftL2Left2Result(left, LSB, result, LSB, shCount);
    }
}

/*-----------------------------------------------------------------*/
/* genLeftShiftLiteral - left shifting by known count              */
/*-----------------------------------------------------------------*/
static void genLeftShiftLiteral (operand *left,
                                 operand *right,
                                 operand *result,
                                 iCode *ic)
{    
    int shCount = (int) floatFromVal (AOP(right)->aopu.aop_lit);
    int size;

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    freeAsmop(right,NULL,ic,TRUE);

    aopOp(left,ic,FALSE);
    aopOp(result,ic,FALSE);

    size = getSize(operandType(result));

#if VIEW_SIZE
    pic14_emitcode("; shift left ","result %d, left %d",size,
             AOP_SIZE(left));
#endif

    /* I suppose that the left size >= result size */
    if(shCount == 0){
        while(size--){
            movLeft2Result(left, size, result, size);
        }
    }

    else if(shCount >= (size * 8))
        while(size--)
            aopPut(AOP(result),zero,size);
    else{
        switch (size) {
            case 1:
                genlshOne (result,left,shCount);
                break;

            case 2:
            case 3:
                genlshTwo (result,left,shCount);
                break;

            case 4:
                genlshFour (result,left,shCount);
                break;
        }
    }
    freeAsmop(left,NULL,ic,TRUE);
    freeAsmop(result,NULL,ic,TRUE);
}

/*-----------------------------------------------------------------*
 * genMultiAsm - repeat assembly instruction for size of register.
 * if endian == 1, then the high byte (i.e base address + size of 
 * register) is used first else the low byte is used first;
 *-----------------------------------------------------------------*/
static void genMultiAsm( PIC_OPCODE poc, operand *reg, int size, int endian)
{

  int offset = 0;

  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

  if(!reg)
    return;

  if(!endian) {
    endian = 1;
  } else {
    endian = -1;
    offset = size-1;
  }

  while(size--) {
    emitpcode(poc,    popGet(AOP(reg),offset));
    offset += endian;
  }

}
/*-----------------------------------------------------------------*/
/* genLeftShift - generates code for left shifting                 */
/*-----------------------------------------------------------------*/
static void genLeftShift (iCode *ic)
{
  operand *left,*right, *result;
  int size, offset;
  char *l;
  symbol *tlbl , *tlbl1;
  pCodeOp *pctemp;

  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

  right = IC_RIGHT(ic);
  left  = IC_LEFT(ic);
  result = IC_RESULT(ic);

  aopOp(right,ic,FALSE);

  /* if the shift count is known then do it 
     as efficiently as possible */
  if (AOP_TYPE(right) == AOP_LIT) {
    genLeftShiftLiteral (left,right,result,ic);
    return ;
  }

  /* shift count is unknown then we have to form 
     a loop get the loop count in B : Note: we take
     only the lower order byte since shifting
     more that 32 bits make no sense anyway, ( the
     largest size of an object can be only 32 bits ) */  

    
  aopOp(left,ic,FALSE);
  aopOp(result,ic,FALSE);

  /* now move the left to the result if they are not the
     same */
  if (!pic14_sameRegs(AOP(left),AOP(result)) && 
      AOP_SIZE(result) > 1) {

    size = AOP_SIZE(result);
    offset=0;
    while (size--) {
      l = aopGet(AOP(left),offset,FALSE,TRUE);
      if (*l == '@' && (IS_AOP_PREG(result))) {

	pic14_emitcode("mov","a,%s",l);
	aopPut(AOP(result),"a",offset);
      } else {
	emitpcode(POC_MOVFW,  popGet(AOP(left),offset));
	emitpcode(POC_MOVWF,  popGet(AOP(result),offset));
	//aopPut(AOP(result),l,offset);
      }
      offset++;
    }
  }

  size = AOP_SIZE(result);

  /* if it is only one byte then */
  if (size == 1) {
    if(optimized_for_speed) {
      emitpcode(POC_SWAPFW, popGet(AOP(left),0));
      emitpcode(POC_ANDLW,  popGetLit(0xf0));
      emitpcode(POC_BTFSS,  newpCodeOpBit(aopGet(AOP(right),0,FALSE,FALSE),2,0));
      emitpcode(POC_MOVFW,  popGet(AOP(left),0));
      emitpcode(POC_MOVWF,  popGet(AOP(result),0));
      emitpcode(POC_BTFSS,  newpCodeOpBit(aopGet(AOP(right),0,FALSE,FALSE),0,0));
      emitpcode(POC_ADDWF,  popGet(AOP(result),0));
      emitpcode(POC_RLFW,   popGet(AOP(result),0));
      emitpcode(POC_ANDLW,  popGetLit(0xfe));
      emitpcode(POC_ADDFW,  popGet(AOP(result),0));
      emitpcode(POC_BTFSC,  newpCodeOpBit(aopGet(AOP(right),0,FALSE,FALSE),1,0));
      emitpcode(POC_ADDWF,  popGet(AOP(result),0));
    } else {

      tlbl = newiTempLabel(NULL);
      if (!pic14_sameRegs(AOP(left),AOP(result))) {
	emitpcode(POC_MOVFW,  popGet(AOP(left),0));
	emitpcode(POC_MOVWF,  popGet(AOP(result),0));
      }

      emitpcode(POC_COMFW,  popGet(AOP(right),0));
      emitpcode(POC_RRF,    popGet(AOP(result),0));
      emitpLabel(tlbl->key);
      emitpcode(POC_RLF,    popGet(AOP(result),0));
      emitpcode(POC_ADDLW,  popGetLit(1));
      emitSKPC;
      emitpcode(POC_GOTO,popGetLabel(tlbl->key));
    }
    goto release ;
  }
    
  if (pic14_sameRegs(AOP(left),AOP(result))) {

    tlbl = newiTempLabel(NULL);
    emitpcode(POC_COMFW,  popGet(AOP(right),0));
    genMultiAsm(POC_RRF, result, size,1);
    emitpLabel(tlbl->key);
    genMultiAsm(POC_RLF, result, size,0);
    emitpcode(POC_ADDLW,  popGetLit(1));
    emitSKPC;
    emitpcode(POC_GOTO,popGetLabel(tlbl->key));
    goto release;
  }

  //tlbl = newiTempLabel(NULL);
  //offset = 0 ;   
  //tlbl1 = newiTempLabel(NULL);

  //reAdjustPreg(AOP(result));    
    
  //pic14_emitcode("sjmp","%05d_DS_",tlbl1->key+100); 
  //pic14_emitcode("","%05d_DS_:",tlbl->key+100);    
  //l = aopGet(AOP(result),offset,FALSE,FALSE);
  //MOVA(l);
  //pic14_emitcode("add","a,acc");         
  //aopPut(AOP(result),"a",offset++);
  //while (--size) {
  //  l = aopGet(AOP(result),offset,FALSE,FALSE);
  //  MOVA(l);
  //  pic14_emitcode("rlc","a");         
  //  aopPut(AOP(result),"a",offset++);
  //}
  //reAdjustPreg(AOP(result));

  //pic14_emitcode("","%05d_DS_:",tlbl1->key+100);
  //pic14_emitcode("djnz","b,%05d_DS_",tlbl->key+100);


  tlbl = newiTempLabel(NULL);
  tlbl1= newiTempLabel(NULL);

  size = AOP_SIZE(result);
  offset = 1;

  pctemp = popGetTempReg();  /* grab a temporary working register. */

  emitpcode(POC_MOVFW, popGet(AOP(right),0));

  /* offset should be 0, 1 or 3 */
  emitpcode(POC_ANDLW, popGetLit(0x07 + ((offset&3) << 3)));
  emitSKPNZ;
  emitpcode(POC_GOTO,  popGetLabel(tlbl1->key));

  emitpcode(POC_MOVWF, pctemp);


  emitpLabel(tlbl->key);

  emitCLRC;
  emitpcode(POC_RLF,  popGet(AOP(result),0));
  while(--size)
    emitpcode(POC_RLF,   popGet(AOP(result),offset++));

  emitpcode(POC_DECFSZ,  pctemp);
  emitpcode(POC_GOTO,popGetLabel(tlbl->key));
  emitpLabel(tlbl1->key);

  popReleaseTempReg(pctemp);


 release:
  freeAsmop (right,NULL,ic,TRUE);
  freeAsmop(left,NULL,ic,TRUE);
  freeAsmop(result,NULL,ic,TRUE);
}

/*-----------------------------------------------------------------*/
/* genrshOne - right shift a one byte quantity by known count      */
/*-----------------------------------------------------------------*/
static void genrshOne (operand *result, operand *left,
                       int shCount, int sign)
{
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    shiftR1Left2Result(left, LSB, result, LSB, shCount, sign);
}

/*-----------------------------------------------------------------*/
/* genrshTwo - right shift two bytes by known amount != 0          */
/*-----------------------------------------------------------------*/
static void genrshTwo (operand *result,operand *left,
                       int shCount, int sign)
{
  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
  /* if shCount >= 8 */
  if (shCount >= 8) {
    shCount -= 8 ;
    if (shCount)
      shiftR1Left2Result(left, MSB16, result, LSB,
			 shCount, sign);
    else
      movLeft2Result(left, MSB16, result, LSB);

    emitpcode(POC_CLRF,popGet(AOP(result),MSB16));

    if(sign) {
      emitpcode(POC_BTFSC,newpCodeOpBit(aopGet(AOP(left),LSB,FALSE,FALSE),7,0));
      emitpcode(POC_DECF, popGet(AOP(result),MSB16));
    }
  }

  /*  1 <= shCount <= 7 */
  else
    shiftR2Left2Result(left, LSB, result, LSB, shCount, sign); 
}

/*-----------------------------------------------------------------*/
/* shiftRLong - shift right one long from left to result           */
/* offl = LSB or MSB16                                             */
/*-----------------------------------------------------------------*/
static void shiftRLong (operand *left, int offl,
                        operand *result, int sign)
{
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    if(!sign)
        pic14_emitcode("clr","c");
    MOVA(aopGet(AOP(left),MSB32,FALSE,FALSE));
    if(sign)
        pic14_emitcode("mov","c,acc.7");
    pic14_emitcode("rrc","a");
    aopPut(AOP(result),"a",MSB32-offl);
    if(offl == MSB16)
        /* add sign of "a" */
        addSign(result, MSB32, sign);

    MOVA(aopGet(AOP(left),MSB24,FALSE,FALSE));
    pic14_emitcode("rrc","a");
    aopPut(AOP(result),"a",MSB24-offl);

    MOVA(aopGet(AOP(left),MSB16,FALSE,FALSE));
    pic14_emitcode("rrc","a");
    aopPut(AOP(result),"a",MSB16-offl);

    if(offl == LSB){
        MOVA(aopGet(AOP(left),LSB,FALSE,FALSE));
        pic14_emitcode("rrc","a");
        aopPut(AOP(result),"a",LSB);
    }
}

/*-----------------------------------------------------------------*/
/* genrshFour - shift four byte by a known amount != 0             */
/*-----------------------------------------------------------------*/
static void genrshFour (operand *result, operand *left,
                        int shCount, int sign)
{
  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
  /* if shifting more that 3 bytes */
  if(shCount >= 24 ) {
    shCount -= 24;
    if(shCount)
      shiftR1Left2Result(left, MSB32, result, LSB, shCount, sign);
    else
      movLeft2Result(left, MSB32, result, LSB);

    addSign(result, MSB16, sign);
  }
  else if(shCount >= 16){
    shCount -= 16;
    if(shCount)
      shiftR2Left2Result(left, MSB24, result, LSB, shCount, sign);
    else{
      movLeft2Result(left, MSB24, result, LSB);
      movLeft2Result(left, MSB32, result, MSB16);
    }
    addSign(result, MSB24, sign);
  }
  else if(shCount >= 8){
    shCount -= 8;
    if(shCount == 1)
      shiftRLong(left, MSB16, result, sign);
    else if(shCount == 0){
      movLeft2Result(left, MSB16, result, LSB);
      movLeft2Result(left, MSB24, result, MSB16);
      movLeft2Result(left, MSB32, result, MSB24);
      addSign(result, MSB32, sign);
    }
    else{
      shiftR2Left2Result(left, MSB16, result, LSB, shCount, 0);
      shiftLLeftOrResult(left, MSB32, result, MSB16, 8 - shCount);
      /* the last shift is signed */
      shiftR1Left2Result(left, MSB32, result, MSB24, shCount, sign);
      addSign(result, MSB32, sign);
    }
  }
  else{   /* 1 <= shCount <= 7 */
    if(shCount <= 2){
      shiftRLong(left, LSB, result, sign);
      if(shCount == 2)
	shiftRLong(result, LSB, result, sign);
    }
    else{
      shiftR2Left2Result(left, LSB, result, LSB, shCount, 0);
      shiftLLeftOrResult(left, MSB24, result, MSB16, 8 - shCount);
      shiftR2Left2Result(left, MSB24, result, MSB24, shCount, sign);
    }
  }
}

/*-----------------------------------------------------------------*/
/* genRightShiftLiteral - right shifting by known count            */
/*-----------------------------------------------------------------*/
static void genRightShiftLiteral (operand *left,
                                  operand *right,
                                  operand *result,
                                  iCode *ic,
                                  int sign)
{    
  int shCount = (int) floatFromVal (AOP(right)->aopu.aop_lit);
  int lsize,res_size;

  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
  freeAsmop(right,NULL,ic,TRUE);

  aopOp(left,ic,FALSE);
  aopOp(result,ic,FALSE);

#if VIEW_SIZE
  pic14_emitcode("; shift right ","result %d, left %d",AOP_SIZE(result),
		 AOP_SIZE(left));
#endif

  lsize = pic14_getDataSize(left);
  res_size = pic14_getDataSize(result);
  /* test the LEFT size !!! */

  /* I suppose that the left size >= result size */
  if(shCount == 0){
    while(res_size--)
      movLeft2Result(left, lsize, result, res_size);
  }

  else if(shCount >= (lsize * 8)){

    if(res_size == 1) {
      emitpcode(POC_CLRF, popGet(AOP(result),LSB));
      if(sign) {
	emitpcode(POC_BTFSC,newpCodeOpBit(aopGet(AOP(left),lsize-1,FALSE,FALSE),7,0));
	emitpcode(POC_DECF, popGet(AOP(result),LSB));
      }
    } else {

      if(sign) {
	emitpcode(POC_MOVLW, popGetLit(0));
	emitpcode(POC_BTFSC, newpCodeOpBit(aopGet(AOP(left),lsize-1,FALSE,FALSE),7,0));
	emitpcode(POC_MOVLW, popGetLit(0xff));
	while(res_size--)
	  emitpcode(POC_MOVWF, popGet(AOP(result),res_size));

      } else {

	while(res_size--)
	  emitpcode(POC_CLRF, popGet(AOP(result),res_size));
      }
    }
  } else {

    switch (res_size) {
    case 1:
      genrshOne (result,left,shCount,sign);
      break;

    case 2:
      genrshTwo (result,left,shCount,sign);
      break;

    case 4:
      genrshFour (result,left,shCount,sign);
      break;
    default :
      break;
    }

  }

  freeAsmop(left,NULL,ic,TRUE);
  freeAsmop(result,NULL,ic,TRUE);
}

/*-----------------------------------------------------------------*/
/* genSignedRightShift - right shift of signed number              */
/*-----------------------------------------------------------------*/
static void genSignedRightShift (iCode *ic)
{
  operand *right, *left, *result;
  int size, offset;
  //  char *l;
  symbol *tlbl, *tlbl1 ;
  pCodeOp *pctemp;

  //same = ((left == result) || (AOP(left) == AOP(result))) && (offl == offr);

  /* we do it the hard way put the shift count in b
     and loop thru preserving the sign */
  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

  right = IC_RIGHT(ic);
  left  = IC_LEFT(ic);
  result = IC_RESULT(ic);

  aopOp(right,ic,FALSE);  
  aopOp(left,ic,FALSE);
  aopOp(result,ic,FALSE);


  if ( AOP_TYPE(right) == AOP_LIT) {
    genRightShiftLiteral (left,right,result,ic,1);
    return ;
  }
  /* shift count is unknown then we have to form 
     a loop get the loop count in B : Note: we take
     only the lower order byte since shifting
     more that 32 bits make no sense anyway, ( the
     largest size of an object can be only 32 bits ) */  

  //pic14_emitcode("mov","b,%s",aopGet(AOP(right),0,FALSE,FALSE));
  //pic14_emitcode("inc","b");
  //freeAsmop (right,NULL,ic,TRUE);
  //aopOp(left,ic,FALSE);
  //aopOp(result,ic,FALSE);

  /* now move the left to the result if they are not the
     same */
  if (!pic14_sameRegs(AOP(left),AOP(result)) && 
      AOP_SIZE(result) > 1) {

    size = AOP_SIZE(result);
    offset=0;
    while (size--) { 
      /*
	l = aopGet(AOP(left),offset,FALSE,TRUE);
	if (*l == '@' && IS_AOP_PREG(result)) {

	pic14_emitcode("mov","a,%s",l);
	aopPut(AOP(result),"a",offset);
	} else
	aopPut(AOP(result),l,offset);
      */
      emitpcode(POC_MOVFW,  popGet(AOP(left),offset));
      emitpcode(POC_MOVWF,  popGet(AOP(result),offset));

      offset++;
    }
  }

  /* mov the highest order bit to OVR */    
  tlbl = newiTempLabel(NULL);
  tlbl1= newiTempLabel(NULL);

  size = AOP_SIZE(result);
  offset = size - 1;

  pctemp = popGetTempReg();  /* grab a temporary working register. */

  emitpcode(POC_MOVFW, popGet(AOP(right),0));

  /* offset should be 0, 1 or 3 */
  emitpcode(POC_ANDLW, popGetLit(0x07 + ((offset&3) << 3)));
  emitSKPNZ;
  emitpcode(POC_GOTO,  popGetLabel(tlbl1->key));

  emitpcode(POC_MOVWF, pctemp);


  emitpLabel(tlbl->key);

  emitpcode(POC_RLFW,  popGet(AOP(result),offset));
  emitpcode(POC_RRF,   popGet(AOP(result),offset));

  while(--size) {
    emitpcode(POC_RRF,   popGet(AOP(result),--offset));
  }

  emitpcode(POC_DECFSZ,  pctemp);
  emitpcode(POC_GOTO,popGetLabel(tlbl->key));
  emitpLabel(tlbl1->key);

  popReleaseTempReg(pctemp);
#if 0
  size = AOP_SIZE(result);
  offset = size - 1;
  pic14_emitcode("mov","a,%s",aopGet(AOP(left),offset,FALSE,FALSE));
  pic14_emitcode("rlc","a");
  pic14_emitcode("mov","ov,c");
  /* if it is only one byte then */
  if (size == 1) {
    l = aopGet(AOP(left),0,FALSE,FALSE);
    MOVA(l);
    pic14_emitcode("sjmp","%05d_DS_",tlbl1->key+100);
    pic14_emitcode("","%05d_DS_:",tlbl->key+100);
    pic14_emitcode("mov","c,ov");
    pic14_emitcode("rrc","a");
    pic14_emitcode("","%05d_DS_:",tlbl1->key+100);
    pic14_emitcode("djnz","b,%05d_DS_",tlbl->key+100);
    aopPut(AOP(result),"a",0);
    goto release ;
  }

  reAdjustPreg(AOP(result));
  pic14_emitcode("sjmp","%05d_DS_",tlbl1->key+100);
  pic14_emitcode("","%05d_DS_:",tlbl->key+100);    
  pic14_emitcode("mov","c,ov");
  while (size--) {
    l = aopGet(AOP(result),offset,FALSE,FALSE);
    MOVA(l);
    pic14_emitcode("rrc","a");         
    aopPut(AOP(result),"a",offset--);
  }
  reAdjustPreg(AOP(result));
  pic14_emitcode("","%05d_DS_:",tlbl1->key+100);
  pic14_emitcode("djnz","b,%05d_DS_",tlbl->key+100);

 release:
#endif

  freeAsmop(left,NULL,ic,TRUE);
  freeAsmop(result,NULL,ic,TRUE);
  freeAsmop(right,NULL,ic,TRUE);
}

/*-----------------------------------------------------------------*/
/* genRightShift - generate code for right shifting                */
/*-----------------------------------------------------------------*/
static void genRightShift (iCode *ic)
{
    operand *right, *left, *result;
    sym_link *retype ;
    int size, offset;
    char *l;
    symbol *tlbl, *tlbl1 ;

    /* if signed then we do it the hard way preserve the
    sign bit moving it inwards */
    retype = getSpec(operandType(IC_RESULT(ic)));
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

    if (!SPEC_USIGN(retype)) {
        genSignedRightShift (ic);
        return ;
    }

    /* signed & unsigned types are treated the same : i.e. the
    signed is NOT propagated inwards : quoting from the
    ANSI - standard : "for E1 >> E2, is equivalent to division
    by 2**E2 if unsigned or if it has a non-negative value,
    otherwise the result is implementation defined ", MY definition
    is that the sign does not get propagated */

    right = IC_RIGHT(ic);
    left  = IC_LEFT(ic);
    result = IC_RESULT(ic);

    aopOp(right,ic,FALSE);

    /* if the shift count is known then do it 
    as efficiently as possible */
    if (AOP_TYPE(right) == AOP_LIT) {
        genRightShiftLiteral (left,right,result,ic, 0);
        return ;
    }

    /* shift count is unknown then we have to form 
    a loop get the loop count in B : Note: we take
    only the lower order byte since shifting
    more that 32 bits make no sense anyway, ( the
    largest size of an object can be only 32 bits ) */  

    pic14_emitcode("mov","b,%s",aopGet(AOP(right),0,FALSE,FALSE));
    pic14_emitcode("inc","b");
    aopOp(left,ic,FALSE);
    aopOp(result,ic,FALSE);

    /* now move the left to the result if they are not the
    same */
    if (!pic14_sameRegs(AOP(left),AOP(result)) && 
        AOP_SIZE(result) > 1) {

        size = AOP_SIZE(result);
        offset=0;
        while (size--) {
            l = aopGet(AOP(left),offset,FALSE,TRUE);
            if (*l == '@' && IS_AOP_PREG(result)) {

                pic14_emitcode("mov","a,%s",l);
                aopPut(AOP(result),"a",offset);
            } else
                aopPut(AOP(result),l,offset);
            offset++;
        }
    }

    tlbl = newiTempLabel(NULL);
    tlbl1= newiTempLabel(NULL);
    size = AOP_SIZE(result);
    offset = size - 1;

    /* if it is only one byte then */
    if (size == 1) {

      tlbl = newiTempLabel(NULL);
      if (!pic14_sameRegs(AOP(left),AOP(result))) {
	emitpcode(POC_MOVFW,  popGet(AOP(left),0));
	emitpcode(POC_MOVWF,  popGet(AOP(result),0));
      }

      emitpcode(POC_COMFW,  popGet(AOP(right),0));
      emitpcode(POC_RLF,    popGet(AOP(result),0));
      emitpLabel(tlbl->key);
      emitpcode(POC_RRF,    popGet(AOP(result),0));
      emitpcode(POC_ADDLW,  popGetLit(1));
      emitSKPC;
      emitpcode(POC_GOTO,popGetLabel(tlbl->key));

      goto release ;
    }

    reAdjustPreg(AOP(result));
    pic14_emitcode("sjmp","%05d_DS_",tlbl1->key+100);
    pic14_emitcode("","%05d_DS_:",tlbl->key+100);    
    CLRC;
    while (size--) {
        l = aopGet(AOP(result),offset,FALSE,FALSE);
        MOVA(l);
        pic14_emitcode("rrc","a");         
        aopPut(AOP(result),"a",offset--);
    }
    reAdjustPreg(AOP(result));

    pic14_emitcode("","%05d_DS_:",tlbl1->key+100);
    pic14_emitcode("djnz","b,%05d_DS_",tlbl->key+100);

release:
    freeAsmop(left,NULL,ic,TRUE);
    freeAsmop (right,NULL,ic,TRUE);
    freeAsmop(result,NULL,ic,TRUE);
}

/*-----------------------------------------------------------------*/
/* genUnpackBits - generates code for unpacking bits               */
/*-----------------------------------------------------------------*/
static void genUnpackBits (operand *result, char *rname, int ptype)
{    
    int shCnt ;
    int rlen = 0 ;
    sym_link *etype;
    int offset = 0 ;

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    etype = getSpec(operandType(result));

    /* read the first byte  */
    switch (ptype) {

    case POINTER:
    case IPOINTER:
	pic14_emitcode("mov","a,@%s",rname);
	break;
	
    case PPOINTER:
	pic14_emitcode("movx","a,@%s",rname);
	break;
	
    case FPOINTER:
	pic14_emitcode("movx","a,@dptr");
	break;

    case CPOINTER:
	pic14_emitcode("clr","a");
	pic14_emitcode("movc","a","@a+dptr");
	break;

    case GPOINTER:
	pic14_emitcode("lcall","__gptrget");
	break;
    }

    /* if we have bitdisplacement then it fits   */
    /* into this byte completely or if length is */
    /* less than a byte                          */
    if ((shCnt = SPEC_BSTR(etype)) || 
        (SPEC_BLEN(etype) <= 8))  {

        /* shift right acc */
        AccRsh(shCnt);

        pic14_emitcode("anl","a,#0x%02x",
                 ((unsigned char) -1)>>(8 - SPEC_BLEN(etype)));
        aopPut(AOP(result),"a",offset);
        return ;
    }

    /* bit field did not fit in a byte  */
    rlen = SPEC_BLEN(etype) - 8;
    aopPut(AOP(result),"a",offset++);

    while (1)  {

	switch (ptype) {
	case POINTER:
	case IPOINTER:
	    pic14_emitcode("inc","%s",rname);
	    pic14_emitcode("mov","a,@%s",rname);
	    break;
	    
	case PPOINTER:
	    pic14_emitcode("inc","%s",rname);
	    pic14_emitcode("movx","a,@%s",rname);
	    break;

	case FPOINTER:
	    pic14_emitcode("inc","dptr");
	    pic14_emitcode("movx","a,@dptr");
	    break;
	    
	case CPOINTER:
	    pic14_emitcode("clr","a");
	    pic14_emitcode("inc","dptr");
	    pic14_emitcode("movc","a","@a+dptr");
	    break;
	    
	case GPOINTER:
	    pic14_emitcode("inc","dptr");
	    pic14_emitcode("lcall","__gptrget");
	    break;
 	}

	rlen -= 8;            
	/* if we are done */
	if ( rlen <= 0 )
	    break ;
	
	aopPut(AOP(result),"a",offset++);
       			      
    }
    
    if (rlen) {
	pic14_emitcode("anl","a,#0x%02x",((unsigned char)-1)>>(-rlen));
	aopPut(AOP(result),"a",offset);	       
    }
    
    return ;
}

#if 0
/*-----------------------------------------------------------------*/
/* genDataPointerGet - generates code when ptr offset is known     */
/*-----------------------------------------------------------------*/
static void genDataPointerGet (operand *left, 
			       operand *result, 
			       iCode *ic)
{
  int size , offset = 0;


  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);


  /* optimization - most of the time, left and result are the same
   * address, but different types. for the pic code, we could omit
   * the following
   */

  aopOp(result,ic,TRUE);

  DEBUGpic14_AopType(__LINE__,left,NULL,result);

  emitpcode(POC_MOVFW, popGet(AOP(left),0));

  size = AOP_SIZE(result);

  while (size--) {
    emitpcode(POC_MOVWF, popGet(AOP(result),offset));
    offset++;
  }

  freeAsmop(left,NULL,ic,TRUE);
  freeAsmop(result,NULL,ic,TRUE);
}
#endif
/*-----------------------------------------------------------------*/
/* genNearPointerGet - pic14_emitcode for near pointer fetch             */
/*-----------------------------------------------------------------*/
static void genNearPointerGet (operand *left, 
			       operand *result, 
			       iCode *ic)
{
    asmop *aop = NULL;
    //regs *preg = NULL ;
    char *rname ;
    sym_link *rtype, *retype;
    sym_link *ltype = operandType(left);    
    //char buffer[80];

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

    rtype = operandType(result);
    retype= getSpec(rtype);
    
    aopOp(left,ic,FALSE);
    
    /* if left is rematerialisable and
       result is not bit variable type and
       the left is pointer to data space i.e
       lower 128 bytes of space */
    if (AOP_TYPE(left) == AOP_PCODE &&  //AOP_TYPE(left) == AOP_IMMD &&
	!IS_BITVAR(retype)         &&
	DCL_TYPE(ltype) == POINTER) {
      //genDataPointerGet (left,result,ic);
	return ;
    }
    
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

	/* if the value is already in a pointer register
       then don't need anything more */
    if (!AOP_INPREG(AOP(left))) {
	/* otherwise get a free pointer register */
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
/*
	aop = newAsmop(0);
	preg = getFreePtr(ic,&aop,FALSE);
	pic14_emitcode("mov","%s,%s",
		preg->name,
		aopGet(AOP(left),0,FALSE,TRUE));
	rname = preg->name ;
*/
    rname ="BAD";
    } else
	rname = aopGet(AOP(left),0,FALSE,FALSE);
    
    aopOp (result,ic,FALSE);
    
      /* if bitfield then unpack the bits */
    if (IS_BITVAR(retype)) 
	genUnpackBits (result,rname,POINTER);
    else {
	/* we have can just get the values */
      int size = AOP_SIZE(result);
      int offset = 0 ;	
	
      DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

      emitpcode(POC_MOVFW,popGet(AOP(left),0));
      emitpcode(POC_MOVWF,popCopyReg(&pc_fsr));
      while(size--) {
	emitpcode(POC_MOVFW,popCopyReg(&pc_indf));
	emitpcode(POC_MOVWF,popGet(AOP(result),offset++));
	if(size)
	  emitpcode(POC_INCF,popCopyReg(&pc_fsr));
      }
/*
	while (size--) {
	    if (IS_AOP_PREG(result) || AOP_TYPE(result) == AOP_STK ) {

		pic14_emitcode("mov","a,@%s",rname);
		aopPut(AOP(result),"a",offset);
	    } else {
		sprintf(buffer,"@%s",rname);
		aopPut(AOP(result),buffer,offset);
	    }
	    offset++ ;
	    if (size)
		pic14_emitcode("inc","%s",rname);
	}
*/
    }

    /* now some housekeeping stuff */
    if (aop) {
	/* we had to allocate for this iCode */
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
	freeAsmop(NULL,aop,ic,TRUE);
    } else { 
	/* we did not allocate which means left
	   already in a pointer register, then
	   if size > 0 && this could be used again
	   we have to point it back to where it 
	   belongs */
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
	if (AOP_SIZE(result) > 1 &&
	    !OP_SYMBOL(left)->remat &&
	    ( OP_SYMBOL(left)->liveTo > ic->seq ||
	      ic->depth )) {
	    int size = AOP_SIZE(result) - 1;
	    while (size--)
		pic14_emitcode("dec","%s",rname);
	}
    }

    /* done */
    freeAsmop(left,NULL,ic,TRUE);
    freeAsmop(result,NULL,ic,TRUE);
     
}

/*-----------------------------------------------------------------*/
/* genPagedPointerGet - pic14_emitcode for paged pointer fetch           */
/*-----------------------------------------------------------------*/
static void genPagedPointerGet (operand *left, 
			       operand *result, 
			       iCode *ic)
{
    asmop *aop = NULL;
    regs *preg = NULL ;
    char *rname ;
    sym_link *rtype, *retype;    

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

    rtype = operandType(result);
    retype= getSpec(rtype);
    
    aopOp(left,ic,FALSE);

  /* if the value is already in a pointer register
       then don't need anything more */
    if (!AOP_INPREG(AOP(left))) {
	/* otherwise get a free pointer register */
	aop = newAsmop(0);
	preg = getFreePtr(ic,&aop,FALSE);
	pic14_emitcode("mov","%s,%s",
		preg->name,
		aopGet(AOP(left),0,FALSE,TRUE));
	rname = preg->name ;
    } else
	rname = aopGet(AOP(left),0,FALSE,FALSE);
    
    freeAsmop(left,NULL,ic,TRUE);
    aopOp (result,ic,FALSE);

    /* if bitfield then unpack the bits */
    if (IS_BITVAR(retype)) 
	genUnpackBits (result,rname,PPOINTER);
    else {
	/* we have can just get the values */
	int size = AOP_SIZE(result);
	int offset = 0 ;	
	
	while (size--) {
	    
	    pic14_emitcode("movx","a,@%s",rname);
	    aopPut(AOP(result),"a",offset);
	    
	    offset++ ;
	    
	    if (size)
		pic14_emitcode("inc","%s",rname);
	}
    }

    /* now some housekeeping stuff */
    if (aop) {
	/* we had to allocate for this iCode */
	freeAsmop(NULL,aop,ic,TRUE);
    } else { 
	/* we did not allocate which means left
	   already in a pointer register, then
	   if size > 0 && this could be used again
	   we have to point it back to where it 
	   belongs */
	if (AOP_SIZE(result) > 1 &&
	    !OP_SYMBOL(left)->remat &&
	    ( OP_SYMBOL(left)->liveTo > ic->seq ||
	      ic->depth )) {
	    int size = AOP_SIZE(result) - 1;
	    while (size--)
		pic14_emitcode("dec","%s",rname);
	}
    }

    /* done */
    freeAsmop(result,NULL,ic,TRUE);
    
	
}

/*-----------------------------------------------------------------*/
/* genFarPointerGet - gget value from far space                    */
/*-----------------------------------------------------------------*/
static void genFarPointerGet (operand *left,
                              operand *result, iCode *ic)
{
    int size, offset ;
    sym_link *retype = getSpec(operandType(result));

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

    aopOp(left,ic,FALSE);

    /* if the operand is already in dptr 
    then we do nothing else we move the value to dptr */
    if (AOP_TYPE(left) != AOP_STR) {
        /* if this is remateriazable */
        if (AOP_TYPE(left) == AOP_IMMD)
            pic14_emitcode("mov","dptr,%s",aopGet(AOP(left),0,TRUE,FALSE));
        else { /* we need to get it byte by byte */
            pic14_emitcode("mov","dpl,%s",aopGet(AOP(left),0,FALSE,FALSE));
            pic14_emitcode("mov","dph,%s",aopGet(AOP(left),1,FALSE,FALSE));
            if (options.model == MODEL_FLAT24)
            {
               pic14_emitcode("mov", "dpx,%s",aopGet(AOP(left),2,FALSE,FALSE));
            }
        }
    }
    /* so dptr know contains the address */
    freeAsmop(left,NULL,ic,TRUE);
    aopOp(result,ic,FALSE);

    /* if bit then unpack */
    if (IS_BITVAR(retype)) 
        genUnpackBits(result,"dptr",FPOINTER);
    else {
        size = AOP_SIZE(result);
        offset = 0 ;

        while (size--) {
            pic14_emitcode("movx","a,@dptr");
            aopPut(AOP(result),"a",offset++);
            if (size)
                pic14_emitcode("inc","dptr");
        }
    }

    freeAsmop(result,NULL,ic,TRUE);
}
#if 0
/*-----------------------------------------------------------------*/
/* genCodePointerGet - get value from code space                  */
/*-----------------------------------------------------------------*/
static void genCodePointerGet (operand *left,
                                operand *result, iCode *ic)
{
    int size, offset ;
    sym_link *retype = getSpec(operandType(result));

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

    aopOp(left,ic,FALSE);

    /* if the operand is already in dptr 
    then we do nothing else we move the value to dptr */
    if (AOP_TYPE(left) != AOP_STR) {
        /* if this is remateriazable */
        if (AOP_TYPE(left) == AOP_IMMD)
            pic14_emitcode("mov","dptr,%s",aopGet(AOP(left),0,TRUE,FALSE));
        else { /* we need to get it byte by byte */
            pic14_emitcode("mov","dpl,%s",aopGet(AOP(left),0,FALSE,FALSE));
            pic14_emitcode("mov","dph,%s",aopGet(AOP(left),1,FALSE,FALSE));
            if (options.model == MODEL_FLAT24)
            {
               pic14_emitcode("mov", "dpx,%s",aopGet(AOP(left),2,FALSE,FALSE));
            }
        }
    }
    /* so dptr know contains the address */
    freeAsmop(left,NULL,ic,TRUE);
    aopOp(result,ic,FALSE);

    /* if bit then unpack */
    if (IS_BITVAR(retype)) 
        genUnpackBits(result,"dptr",CPOINTER);
    else {
        size = AOP_SIZE(result);
        offset = 0 ;

        while (size--) {
            pic14_emitcode("clr","a");
            pic14_emitcode("movc","a,@a+dptr");
            aopPut(AOP(result),"a",offset++);
            if (size)
                pic14_emitcode("inc","dptr");
        }
    }

    freeAsmop(result,NULL,ic,TRUE);
}
#endif
/*-----------------------------------------------------------------*/
/* genGenPointerGet - gget value from generic pointer space        */
/*-----------------------------------------------------------------*/
static void genGenPointerGet (operand *left,
                              operand *result, iCode *ic)
{
  int size, offset ;
  sym_link *retype = getSpec(operandType(result));

  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
  aopOp(left,ic,FALSE);
  aopOp(result,ic,FALSE);


  DEBUGpic14_AopType(__LINE__,left,NULL,result);

  /* if the operand is already in dptr 
     then we do nothing else we move the value to dptr */
  //  if (AOP_TYPE(left) != AOP_STR) {
    /* if this is remateriazable */
    if (AOP_TYPE(left) == AOP_IMMD) {
      pic14_emitcode("mov","dptr,%s",aopGet(AOP(left),0,TRUE,FALSE));
      pic14_emitcode("mov","b,#%d",pointerCode(retype));
    }
    else { /* we need to get it byte by byte */

      emitpcode(POC_MOVFW,popGet(AOP(left),0));
      emitpcode(POC_MOVWF,popCopyReg(&pc_fsr));

      size = AOP_SIZE(result);
      offset = 0 ;

      while(size--) {
	emitpcode(POC_MOVFW,popCopyReg(&pc_indf));
	emitpcode(POC_MOVWF,popGet(AOP(result),offset++));
	if(size)
	  emitpcode(POC_INCF,popCopyReg(&pc_fsr));
      }
      goto release;
    }
    //}
  /* so dptr know contains the address */

  /* if bit then unpack */
  //if (IS_BITVAR(retype)) 
  //  genUnpackBits(result,"dptr",GPOINTER);

 release:
  freeAsmop(left,NULL,ic,TRUE);
  freeAsmop(result,NULL,ic,TRUE);

}

/*-----------------------------------------------------------------*/
/* genConstPointerGet - get value from const generic pointer space */
/*-----------------------------------------------------------------*/
static void genConstPointerGet (operand *left,
				operand *result, iCode *ic)
{
  //sym_link *retype = getSpec(operandType(result));
  symbol *albl = newiTempLabel(NULL);
  symbol *blbl = newiTempLabel(NULL);
  PIC_OPCODE poc;

  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
  aopOp(left,ic,FALSE);
  aopOp(result,ic,FALSE);


  DEBUGpic14_AopType(__LINE__,left,NULL,result);

  DEBUGpic14_emitcode ("; "," %d getting const pointer",__LINE__);

  emitpcode(POC_CALL,popGetLabel(albl->key));
  emitpcodePagesel(popGetLabel(blbl->key)->name); /* Must restore PCLATH before goto, without destroying W */
  emitpcode(POC_GOTO,popGetLabel(blbl->key));
  emitpLabel(albl->key);

  poc = ( (AOP_TYPE(left) == AOP_PCODE) ? POC_MOVLW : POC_MOVFW);
    
  emitpcode(poc,popGet(AOP(left),1));
  emitpcode(POC_MOVWF,popCopyReg(&pc_pclath));
  emitpcode(poc,popGet(AOP(left),0));
  emitpcode(POC_MOVWF,popCopyReg(&pc_pcl));

  emitpLabel(blbl->key);

  emitpcode(POC_MOVWF,popGet(AOP(result),0));


  freeAsmop(left,NULL,ic,TRUE);
  freeAsmop(result,NULL,ic,TRUE);

}
/*-----------------------------------------------------------------*/
/* genPointerGet - generate code for pointer get                   */
/*-----------------------------------------------------------------*/
static void genPointerGet (iCode *ic)
{
    operand *left, *result ;
    sym_link *type, *etype;
    int p_type;

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

    left = IC_LEFT(ic);
    result = IC_RESULT(ic) ;

    /* depending on the type of pointer we need to
    move it to the correct pointer register */
    type = operandType(left);
    etype = getSpec(type);

    if (IS_PTR_CONST(type))
      DEBUGpic14_emitcode ("; ***","%d - const pointer",__LINE__);

    /* if left is of type of pointer then it is simple */
    if (IS_PTR(type) && !IS_FUNC(type->next)) 
        p_type = DCL_TYPE(type);
    else {
	/* we have to go by the storage class */
	p_type = PTR_TYPE(SPEC_OCLS(etype));

	DEBUGpic14_emitcode ("; ***","%d - resolve pointer by storage class",__LINE__);

	if (SPEC_OCLS(etype)->codesp ) {
	  DEBUGpic14_emitcode ("; ***","%d - cpointer",__LINE__);
	  //p_type = CPOINTER ;	
	}
	else
	    if (SPEC_OCLS(etype)->fmap && !SPEC_OCLS(etype)->paged)
	      DEBUGpic14_emitcode ("; ***","%d - fpointer",__LINE__);
	       /*p_type = FPOINTER ;*/ 
	    else
		if (SPEC_OCLS(etype)->fmap && SPEC_OCLS(etype)->paged)
		  DEBUGpic14_emitcode ("; ***","%d - ppointer",__LINE__);
/* 		    p_type = PPOINTER; */
		else
		    if (SPEC_OCLS(etype) == idata )
		      DEBUGpic14_emitcode ("; ***","%d - ipointer",__LINE__);
/* 			p_type = IPOINTER; */
		    else
		      DEBUGpic14_emitcode ("; ***","%d - pointer",__LINE__);
/* 			p_type = POINTER ; */
    }

    /* now that we have the pointer type we assign
    the pointer values */
    switch (p_type) {

    case POINTER:	
    case IPOINTER:
	genNearPointerGet (left,result,ic);
	break;

    case PPOINTER:
	genPagedPointerGet(left,result,ic);
	break;

    case FPOINTER:
	genFarPointerGet (left,result,ic);
	break;

    case CPOINTER:
	genConstPointerGet (left,result,ic);
	//pic14_emitcodePointerGet (left,result,ic);
	break;

    case GPOINTER:
      if (IS_PTR_CONST(type))
	genConstPointerGet (left,result,ic);
      else
	genGenPointerGet (left,result,ic);
      break;
    }

}

/*-----------------------------------------------------------------*/
/* genPackBits - generates code for packed bit storage             */
/*-----------------------------------------------------------------*/
static void genPackBits (sym_link    *etype ,
                         operand *right ,
                         char *rname, int p_type)
{
    int shCount = 0 ;
    int offset = 0  ;
    int rLen = 0 ;
    int blen, bstr ;   
    char *l ;

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    blen = SPEC_BLEN(etype);
    bstr = SPEC_BSTR(etype);

    l = aopGet(AOP(right),offset++,FALSE,FALSE);
    MOVA(l);   

    /* if the bit lenth is less than or    */
    /* it exactly fits a byte then         */
    if (SPEC_BLEN(etype) <= 8 )  {
        shCount = SPEC_BSTR(etype) ;

        /* shift left acc */
        AccLsh(shCount);

        if (SPEC_BLEN(etype) < 8 ) { /* if smaller than a byte */


            switch (p_type) {
                case POINTER:
                    pic14_emitcode ("mov","b,a");
                    pic14_emitcode("mov","a,@%s",rname);
                    break;

                case FPOINTER:
                    pic14_emitcode ("mov","b,a");
                    pic14_emitcode("movx","a,@dptr");
                    break;

                case GPOINTER:
                    pic14_emitcode ("push","b");
                    pic14_emitcode ("push","acc");
                    pic14_emitcode ("lcall","__gptrget");
                    pic14_emitcode ("pop","b");
                    break;
            }

            pic14_emitcode ("anl","a,#0x%02x",(unsigned char)
                      ((unsigned char)(0xFF << (blen+bstr)) | 
                       (unsigned char)(0xFF >> (8-bstr)) ) );
            pic14_emitcode ("orl","a,b");
            if (p_type == GPOINTER)
                pic14_emitcode("pop","b");
        }
    }

    switch (p_type) {
        case POINTER:
            pic14_emitcode("mov","@%s,a",rname);
            break;

        case FPOINTER:
            pic14_emitcode("movx","@dptr,a");
            break;

        case GPOINTER:
            DEBUGpic14_emitcode(";lcall","__gptrput");
            break;
    }

    /* if we r done */
    if ( SPEC_BLEN(etype) <= 8 )
        return ;

    pic14_emitcode("inc","%s",rname);
    rLen = SPEC_BLEN(etype) ;     

    /* now generate for lengths greater than one byte */
    while (1) {

        l = aopGet(AOP(right),offset++,FALSE,TRUE);

        rLen -= 8 ;
        if (rLen <= 0 )
            break ;

        switch (p_type) {
            case POINTER:
                if (*l == '@') {
                    MOVA(l);
                    pic14_emitcode("mov","@%s,a",rname);
                } else
                    pic14_emitcode("mov","@%s,%s",rname,l);
                break;

            case FPOINTER:
                MOVA(l);
                pic14_emitcode("movx","@dptr,a");
                break;

            case GPOINTER:
                MOVA(l);
                DEBUGpic14_emitcode(";lcall","__gptrput");
                break;  
        }   
        pic14_emitcode ("inc","%s",rname);
    }

    MOVA(l);

    /* last last was not complete */
    if (rLen)   {
        /* save the byte & read byte */
        switch (p_type) {
            case POINTER:
                pic14_emitcode ("mov","b,a");
                pic14_emitcode("mov","a,@%s",rname);
                break;

            case FPOINTER:
                pic14_emitcode ("mov","b,a");
                pic14_emitcode("movx","a,@dptr");
                break;

            case GPOINTER:
                pic14_emitcode ("push","b");
                pic14_emitcode ("push","acc");
                pic14_emitcode ("lcall","__gptrget");
                pic14_emitcode ("pop","b");
                break;
        }

        pic14_emitcode ("anl","a,#0x%02x",((unsigned char)-1 << -rLen) );
        pic14_emitcode ("orl","a,b");
    }

    if (p_type == GPOINTER)
        pic14_emitcode("pop","b");

    switch (p_type) {

    case POINTER:
	pic14_emitcode("mov","@%s,a",rname);
	break;
	
    case FPOINTER:
	pic14_emitcode("movx","@dptr,a");
	break;
	
    case GPOINTER:
	DEBUGpic14_emitcode(";lcall","__gptrput");
	break;   		
    }
}
/*-----------------------------------------------------------------*/
/* genDataPointerSet - remat pointer to data space                 */
/*-----------------------------------------------------------------*/
static void genDataPointerSet(operand *right,
			      operand *result,
			      iCode *ic)
{
    int size, offset = 0 ;
    char *l, buffer[256];

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    aopOp(right,ic,FALSE);
    
    l = aopGet(AOP(result),0,FALSE,TRUE);
    size = AOP_SIZE(right);
/*
    if ( AOP_TYPE(result) == AOP_PCODE) {
      fprintf(stderr,"genDataPointerSet   %s, %d\n",
	      AOP(result)->aopu.pcop->name,
	      PCOI(AOP(result)->aopu.pcop)->offset);
    }
*/

    // tsd, was l+1 - the underline `_' prefix was being stripped
    while (size--) {
      if (offset) {
	sprintf(buffer,"(%s + %d)",l,offset);
	fprintf(stderr,"oops  %s\n",buffer);
      } else
	sprintf(buffer,"%s",l);

	if (AOP_TYPE(right) == AOP_LIT) {
	  unsigned int lit = (unsigned int) floatFromVal (AOP(IC_RIGHT(ic))->aopu.aop_lit);
	  lit = lit >> (8*offset);
	  if(lit&0xff) {
	    pic14_emitcode("movlw","%d",lit);
	    pic14_emitcode("movwf","%s",buffer);

	    emitpcode(POC_MOVLW, popGetLit(lit&0xff));
	    //emitpcode(POC_MOVWF, popRegFromString(buffer));
	    emitpcode(POC_MOVWF, popGet(AOP(result),0));

	  } else {
	    pic14_emitcode("clrf","%s",buffer);
	    //emitpcode(POC_CLRF, popRegFromString(buffer));
	    emitpcode(POC_CLRF, popGet(AOP(result),0));
	  }
	}else {
	  pic14_emitcode("movf","%s,w",aopGet(AOP(right),offset,FALSE,FALSE));
	  pic14_emitcode("movwf","%s",buffer);

	  emitpcode(POC_MOVFW, popGet(AOP(right),offset));
	  //emitpcode(POC_MOVWF, popRegFromString(buffer));
	  emitpcode(POC_MOVWF, popGet(AOP(result),0));

	}

	offset++;
    }

    freeAsmop(right,NULL,ic,TRUE);
    freeAsmop(result,NULL,ic,TRUE);
}

/*-----------------------------------------------------------------*/
/* genNearPointerSet - pic14_emitcode for near pointer put                */
/*-----------------------------------------------------------------*/
static void genNearPointerSet (operand *right,
                               operand *result, 
                               iCode *ic)
{
  asmop *aop = NULL;
  char *l;
  sym_link *retype;
  sym_link *ptype = operandType(result);

    
  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
  retype= getSpec(operandType(right));

  aopOp(result,ic,FALSE);

    
  /* if the result is rematerializable &
     in data space & not a bit variable */
  //if (AOP_TYPE(result) == AOP_IMMD &&
  if (AOP_TYPE(result) == AOP_PCODE &&  //AOP_TYPE(result) == AOP_IMMD &&
      DCL_TYPE(ptype) == POINTER   &&
      !IS_BITVAR(retype)) {
    genDataPointerSet (right,result,ic);
    freeAsmop(result,NULL,ic,TRUE);
    return;
  }

  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
  aopOp(right,ic,FALSE);
  DEBUGpic14_AopType(__LINE__,NULL,right,result);

  /* if the value is already in a pointer register
     then don't need anything more */
  if (!AOP_INPREG(AOP(result))) {
    /* otherwise get a free pointer register */
    //aop = newAsmop(0);
    //preg = getFreePtr(ic,&aop,FALSE);
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    //pic14_emitcode("mov","%s,%s",
    //         preg->name,
    //         aopGet(AOP(result),0,FALSE,TRUE));
    //rname = preg->name ;
    //pic14_emitcode("movwf","fsr");
    emitpcode(POC_MOVFW, popGet(AOP(result),0));
    emitpcode(POC_MOVWF, popCopyReg(&pc_fsr));
    emitpcode(POC_MOVFW, popGet(AOP(right),0));
    emitpcode(POC_MOVWF, popCopyReg(&pc_indf));
    goto release;

  }// else
  //   rname = aopGet(AOP(result),0,FALSE,FALSE);


  /* if bitfield then unpack the bits */
  if (IS_BITVAR(retype)) {
    werror(E_INTERNAL_ERROR,__FILE__,__LINE__,
	   "The programmer is obviously confused");
    //genPackBits (retype,right,rname,POINTER);
    exit(1);
  }
  else {
    /* we have can just get the values */
    int size = AOP_SIZE(right);
    int offset = 0 ;    

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    while (size--) {
      l = aopGet(AOP(right),offset,FALSE,TRUE);
      if (*l == '@' ) {
	//MOVA(l);
	//pic14_emitcode("mov","@%s,a",rname);
	pic14_emitcode("movf","indf,w ;1");
      } else {

	if (AOP_TYPE(right) == AOP_LIT) {
	  unsigned int lit = (unsigned int) floatFromVal (AOP(IC_RIGHT(ic))->aopu.aop_lit);
	  if(lit) {
	    pic14_emitcode("movlw","%s",l);
	    pic14_emitcode("movwf","indf ;2");
	  } else 
	    pic14_emitcode("clrf","indf");
	}else {
	  pic14_emitcode("movf","%s,w",l);
	  pic14_emitcode("movwf","indf ;2");
	}
	//pic14_emitcode("mov","@%s,%s",rname,l);
      }
      if (size)
	pic14_emitcode("incf","fsr,f ;3");
      //pic14_emitcode("inc","%s",rname);
      offset++;
    }
  }

  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
  /* now some housekeeping stuff */
  if (aop) {
    /* we had to allocate for this iCode */
    freeAsmop(NULL,aop,ic,TRUE);
  } else { 
    /* we did not allocate which means left
       already in a pointer register, then
       if size > 0 && this could be used again
       we have to point it back to where it 
       belongs */
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    if (AOP_SIZE(right) > 1 &&
	!OP_SYMBOL(result)->remat &&
	( OP_SYMBOL(result)->liveTo > ic->seq ||
	  ic->depth )) {
      int size = AOP_SIZE(right) - 1;
      while (size--)
	pic14_emitcode("decf","fsr,f");
      //pic14_emitcode("dec","%s",rname);
    }
  }

  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
  /* done */
 release:
  freeAsmop(right,NULL,ic,TRUE);
  freeAsmop(result,NULL,ic,TRUE);
}

/*-----------------------------------------------------------------*/
/* genPagedPointerSet - pic14_emitcode for Paged pointer put             */
/*-----------------------------------------------------------------*/
static void genPagedPointerSet (operand *right,
			       operand *result, 
			       iCode *ic)
{
    asmop *aop = NULL;
    regs *preg = NULL ;
    char *rname , *l;
    sym_link *retype;
       
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

    retype= getSpec(operandType(right));
    
    aopOp(result,ic,FALSE);
    
    /* if the value is already in a pointer register
       then don't need anything more */
    if (!AOP_INPREG(AOP(result))) {
	/* otherwise get a free pointer register */
	aop = newAsmop(0);
	preg = getFreePtr(ic,&aop,FALSE);
	pic14_emitcode("mov","%s,%s",
		preg->name,
		aopGet(AOP(result),0,FALSE,TRUE));
	rname = preg->name ;
    } else
	rname = aopGet(AOP(result),0,FALSE,FALSE);
    
    freeAsmop(result,NULL,ic,TRUE);
    aopOp (right,ic,FALSE);

    /* if bitfield then unpack the bits */
    if (IS_BITVAR(retype)) 
	genPackBits (retype,right,rname,PPOINTER);
    else {
	/* we have can just get the values */
	int size = AOP_SIZE(right);
	int offset = 0 ;	
	
	while (size--) {
	    l = aopGet(AOP(right),offset,FALSE,TRUE);
	    
	    MOVA(l);
	    pic14_emitcode("movx","@%s,a",rname);

	    if (size)
		pic14_emitcode("inc","%s",rname);

	    offset++;
	}
    }
    
    /* now some housekeeping stuff */
    if (aop) {
	/* we had to allocate for this iCode */
	freeAsmop(NULL,aop,ic,TRUE);
    } else { 
	/* we did not allocate which means left
	   already in a pointer register, then
	   if size > 0 && this could be used again
	   we have to point it back to where it 
	   belongs */
	if (AOP_SIZE(right) > 1 &&
	    !OP_SYMBOL(result)->remat &&
	    ( OP_SYMBOL(result)->liveTo > ic->seq ||
	      ic->depth )) {
	    int size = AOP_SIZE(right) - 1;
	    while (size--)
		pic14_emitcode("dec","%s",rname);
	}
    }

    /* done */
    freeAsmop(right,NULL,ic,TRUE);
    
	
}

/*-----------------------------------------------------------------*/
/* genFarPointerSet - set value from far space                     */
/*-----------------------------------------------------------------*/
static void genFarPointerSet (operand *right,
                              operand *result, iCode *ic)
{
    int size, offset ;
    sym_link *retype = getSpec(operandType(right));

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    aopOp(result,ic,FALSE);

    /* if the operand is already in dptr 
    then we do nothing else we move the value to dptr */
    if (AOP_TYPE(result) != AOP_STR) {
        /* if this is remateriazable */
        if (AOP_TYPE(result) == AOP_IMMD)
            pic14_emitcode("mov","dptr,%s",aopGet(AOP(result),0,TRUE,FALSE));
        else { /* we need to get it byte by byte */
            pic14_emitcode("mov","dpl,%s",aopGet(AOP(result),0,FALSE,FALSE));
            pic14_emitcode("mov","dph,%s",aopGet(AOP(result),1,FALSE,FALSE));
            if (options.model == MODEL_FLAT24)
            {
               pic14_emitcode("mov", "dpx,%s",aopGet(AOP(result),2,FALSE,FALSE));
            }
        }
    }
    /* so dptr know contains the address */
    freeAsmop(result,NULL,ic,TRUE);
    aopOp(right,ic,FALSE);

    /* if bit then unpack */
    if (IS_BITVAR(retype)) 
        genPackBits(retype,right,"dptr",FPOINTER);
    else {
        size = AOP_SIZE(right);
        offset = 0 ;

        while (size--) {
            char *l = aopGet(AOP(right),offset++,FALSE,FALSE);
            MOVA(l);
            pic14_emitcode("movx","@dptr,a");
            if (size)
                pic14_emitcode("inc","dptr");
        }
    }

    freeAsmop(right,NULL,ic,TRUE);
}

/*-----------------------------------------------------------------*/
/* genGenPointerSet - set value from generic pointer space         */
/*-----------------------------------------------------------------*/
static void genGenPointerSet (operand *right,
                              operand *result, iCode *ic)
{
  int size, offset ;
  sym_link *retype = getSpec(operandType(right));

  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

  aopOp(result,ic,FALSE);
  aopOp(right,ic,FALSE);
  size = AOP_SIZE(right);

  DEBUGpic14_AopType(__LINE__,NULL,right,result);

  /* if the operand is already in dptr 
     then we do nothing else we move the value to dptr */
  if (AOP_TYPE(result) != AOP_STR) {
    /* if this is remateriazable */
    if (AOP_TYPE(result) == AOP_IMMD) {
      pic14_emitcode("mov","dptr,%s",aopGet(AOP(result),0,TRUE,FALSE));
      pic14_emitcode("mov","b,%s + 1",aopGet(AOP(result),0,TRUE,FALSE));
    }
    else { /* we need to get it byte by byte */
      //char *l = aopGet(AOP(result),0,FALSE,FALSE);
      size = AOP_SIZE(right);
      offset = 0 ;

      /* hack hack! see if this the FSR. If so don't load W */
      if(AOP_TYPE(right) != AOP_ACC) {


	emitpcode(POC_MOVFW,popGet(AOP(result),0));
	emitpcode(POC_MOVWF,popCopyReg(&pc_fsr));

	if(AOP_SIZE(result) > 1) {
	  emitpcode(POC_BCF,  popCopyGPR2Bit(PCOP(&pc_status),PIC_IRP_BIT));
	  emitpcode(POC_BTFSC,newpCodeOpBit(aopGet(AOP(result),1,FALSE,FALSE),0,0));
	  emitpcode(POC_BSF,  popCopyGPR2Bit(PCOP(&pc_status),PIC_IRP_BIT));

	}

	//if(size==2)
	//emitpcode(POC_DECF,popCopyReg(&pc_fsr));
	//if(size==4) {
	//  emitpcode(POC_MOVLW,popGetLit(0xfd));
	//  emitpcode(POC_ADDWF,popCopyReg(&pc_fsr));
	//}

	while(size--) {
	  emitpcode(POC_MOVFW,popGet(AOP(right),offset++));
	  emitpcode(POC_MOVWF,popCopyReg(&pc_indf));
	  
	  if(size)
	    emitpcode(POC_INCF,popCopyReg(&pc_fsr));
	}


	goto release;
      } 

      if(aopIdx(AOP(result),0) != 4) {

	emitpcode(POC_MOVWF,popCopyReg(&pc_indf));
	goto release;
      }

      emitpcode(POC_MOVWF,popCopyReg(&pc_indf));
      goto release;

    }
  }
  /* so dptr know contains the address */


  /* if bit then unpack */
  if (IS_BITVAR(retype)) 
    genPackBits(retype,right,"dptr",GPOINTER);
  else {
    size = AOP_SIZE(right);
    offset = 0 ;

  DEBUGpic14_emitcode ("; ***","%s  %d size=%d",__FUNCTION__,__LINE__,size);

    while (size--) {

      emitpcode(POC_MOVFW,popGet(AOP(result),offset));
      emitpcode(POC_MOVWF,popCopyReg(&pc_fsr));

      if (AOP_TYPE(right) == AOP_LIT) 
	emitpcode(POC_MOVLW, popGet(AOP(right),offset));
      else
	emitpcode(POC_MOVFW, popGet(AOP(right),offset));

      emitpcode(POC_MOVWF,popCopyReg(&pc_indf));

      offset++;
    }
  }

 release:
  freeAsmop(right,NULL,ic,TRUE);
  freeAsmop(result,NULL,ic,TRUE);
}

/*-----------------------------------------------------------------*/
/* genPointerSet - stores the value into a pointer location        */
/*-----------------------------------------------------------------*/
static void genPointerSet (iCode *ic)
{    
    operand *right, *result ;
    sym_link *type, *etype;
    int p_type;

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

    right = IC_RIGHT(ic);
    result = IC_RESULT(ic) ;

    /* depending on the type of pointer we need to
    move it to the correct pointer register */
    type = operandType(result);
    etype = getSpec(type);
    /* if left is of type of pointer then it is simple */
    if (IS_PTR(type) && !IS_FUNC(type->next)) {
        p_type = DCL_TYPE(type);
    }
    else {
	/* we have to go by the storage class */
	p_type = PTR_TYPE(SPEC_OCLS(etype));

/* 	if (SPEC_OCLS(etype)->codesp ) { */
/* 	    p_type = CPOINTER ;	 */
/* 	} */
/* 	else */
/* 	    if (SPEC_OCLS(etype)->fmap && !SPEC_OCLS(etype)->paged) */
/* 		p_type = FPOINTER ; */
/* 	    else */
/* 		if (SPEC_OCLS(etype)->fmap && SPEC_OCLS(etype)->paged) */
/* 		    p_type = PPOINTER ; */
/* 		else */
/* 		    if (SPEC_OCLS(etype) == idata ) */
/* 			p_type = IPOINTER ; */
/* 		    else */
/* 			p_type = POINTER ; */
    }

    /* now that we have the pointer type we assign
    the pointer values */
    switch (p_type) {

    case POINTER:
    case IPOINTER:
	genNearPointerSet (right,result,ic);
	break;

    case PPOINTER:
	genPagedPointerSet (right,result,ic);
	break;

    case FPOINTER:
	genFarPointerSet (right,result,ic);
	break;

    case GPOINTER:
	genGenPointerSet (right,result,ic);
	break;

    default:
      werror (E_INTERNAL_ERROR, __FILE__, __LINE__, 
	      "genPointerSet: illegal pointer type");
    }
}

/*-----------------------------------------------------------------*/
/* genIfx - generate code for Ifx statement                        */
/*-----------------------------------------------------------------*/
static void genIfx (iCode *ic, iCode *popIc)
{
  operand *cond = IC_COND(ic);
  int isbit =0;

  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

  aopOp(cond,ic,FALSE);

  /* get the value into acc */
  if (AOP_TYPE(cond) != AOP_CRY)
    pic14_toBoolean(cond);
  else
    isbit = 1;
  /* the result is now in the accumulator */
  freeAsmop(cond,NULL,ic,TRUE);

  /* if there was something to be popped then do it */
  if (popIc)
    genIpop(popIc);

  /* if the condition is  a bit variable */
  if (isbit && IS_ITEMP(cond) && 
      SPIL_LOC(cond)) {
    genIfxJump(ic,SPIL_LOC(cond)->rname);
    DEBUGpic14_emitcode ("; isbit  SPIL_LOC","%s",SPIL_LOC(cond)->rname);
  }
  else {
    if (isbit && !IS_ITEMP(cond))
      genIfxJump(ic,OP_SYMBOL(cond)->rname);
    else
      genIfxJump(ic,"a");
  }
  ic->generated = 1;

}

/*-----------------------------------------------------------------*/
/* genAddrOf - generates code for address of                       */
/*-----------------------------------------------------------------*/
static void genAddrOf (iCode *ic)
{
  operand *right, *result, *left;
  int size, offset ;

  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);


  //aopOp(IC_RESULT(ic),ic,FALSE);

  aopOp((left=IC_LEFT(ic)),ic,FALSE);
  aopOp((right=IC_RIGHT(ic)),ic,FALSE);
  aopOp((result=IC_RESULT(ic)),ic,TRUE);

  DEBUGpic14_AopType(__LINE__,left,right,result);

  size = AOP_SIZE(IC_RESULT(ic));
  offset = 0;

  while (size--) {
    emitpcode(POC_MOVLW, popGet(AOP(left),offset));
    emitpcode(POC_MOVWF, popGet(AOP(result),offset));
    offset++;
  }

  freeAsmop(left,NULL,ic,FALSE);
  freeAsmop(result,NULL,ic,TRUE);

}

#if 0
/*-----------------------------------------------------------------*/
/* genFarFarAssign - assignment when both are in far space         */
/*-----------------------------------------------------------------*/
static void genFarFarAssign (operand *result, operand *right, iCode *ic)
{
    int size = AOP_SIZE(right);
    int offset = 0;
    char *l ;
    /* first push the right side on to the stack */
    while (size--) {
	l = aopGet(AOP(right),offset++,FALSE,FALSE);
	MOVA(l);
	pic14_emitcode ("push","acc");
    }
    
    freeAsmop(right,NULL,ic,FALSE);
    /* now assign DPTR to result */
    aopOp(result,ic,FALSE);
    size = AOP_SIZE(result);
    while (size--) {
	pic14_emitcode ("pop","acc");
	aopPut(AOP(result),"a",--offset);
    }
    freeAsmop(result,NULL,ic,FALSE);
	
}
#endif

/*-----------------------------------------------------------------*/
/* genAssign - generate code for assignment                        */
/*-----------------------------------------------------------------*/
static void genAssign (iCode *ic)
{
  operand *result, *right;
  int size, offset,know_W;
  unsigned long lit = 0L;

  result = IC_RESULT(ic);
  right  = IC_RIGHT(ic) ;

  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

  /* if they are the same */
  if (operandsEqu (IC_RESULT(ic),IC_RIGHT(ic)))
    return ;

  aopOp(right,ic,FALSE);
  aopOp(result,ic,TRUE);

  DEBUGpic14_AopType(__LINE__,NULL,right,result);

  /* if they are the same registers */
  if (pic14_sameRegs(AOP(right),AOP(result)))
    goto release;

  /* if the result is a bit */
  if (AOP_TYPE(result) == AOP_CRY) {

    /* if the right size is a literal then
       we know what the value is */
    if (AOP_TYPE(right) == AOP_LIT) {
	  
      emitpcode(  ( ((int) operandLitValue(right)) ? POC_BSF : POC_BCF),
		  popGet(AOP(result),0));

      if (((int) operandLitValue(right))) 
	pic14_emitcode("bsf","(%s >> 3),(%s & 7)",
		       AOP(result)->aopu.aop_dir,
		       AOP(result)->aopu.aop_dir);
      else
	pic14_emitcode("bcf","(%s >> 3),(%s & 7)",
		       AOP(result)->aopu.aop_dir,
		       AOP(result)->aopu.aop_dir);
      goto release;
    }

    /* the right is also a bit variable */
    if (AOP_TYPE(right) == AOP_CRY) {
      emitpcode(POC_BCF,    popGet(AOP(result),0));
      emitpcode(POC_BTFSC,  popGet(AOP(right),0));
      emitpcode(POC_BSF,    popGet(AOP(result),0));

      pic14_emitcode("bcf","(%s >> 3),(%s & 7)",
		     AOP(result)->aopu.aop_dir,
		     AOP(result)->aopu.aop_dir);
      pic14_emitcode("btfsc","(%s >> 3),(%s & 7)",
		     AOP(right)->aopu.aop_dir,
		     AOP(right)->aopu.aop_dir);
      pic14_emitcode("bsf","(%s >> 3),(%s & 7)",
		     AOP(result)->aopu.aop_dir,
		     AOP(result)->aopu.aop_dir);
      goto release ;
    }

    /* we need to or */
    emitpcode(POC_BCF,    popGet(AOP(result),0));
    pic14_toBoolean(right);
    emitSKPZ;
    emitpcode(POC_BSF,    popGet(AOP(result),0));
    //aopPut(AOP(result),"a",0);
    goto release ;
  }

  /* bit variables done */
  /* general case */
  size = AOP_SIZE(result);
  offset = 0 ;
  if(AOP_TYPE(right) == AOP_LIT)
    lit = (unsigned long)floatFromVal(AOP(right)->aopu.aop_lit);

  if( AOP_TYPE(right) == AOP_DIR  && (AOP_TYPE(result) == AOP_REG) && size==1)  {
  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    if(aopIdx(AOP(result),0) == 4) {
  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
      emitpcode(POC_MOVFW, popGet(AOP(right),offset));
      emitpcode(POC_MOVWF, popGet(AOP(result),offset));
      goto release;
    } else
      DEBUGpic14_emitcode ("; WARNING","%s  %d ignoring register storage",__FUNCTION__,__LINE__);
  }

  know_W=-1;
  while (size--) {
  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);
    if(AOP_TYPE(right) == AOP_LIT) {
      if(lit&0xff) {
	if(know_W != (int)(lit&0xff))
	  emitpcode(POC_MOVLW,popGetLit(lit&0xff));
	know_W = lit&0xff;
	emitpcode(POC_MOVWF, popGet(AOP(result),offset));
      } else
	emitpcode(POC_CLRF, popGet(AOP(result),offset));

      lit >>= 8;

    } else if (AOP_TYPE(right) == AOP_CRY) {
      emitpcode(POC_CLRF, popGet(AOP(result),offset));
      if(offset == 0) {
	emitpcode(POC_BTFSS, popGet(AOP(right),0));
	emitpcode(POC_INCF, popGet(AOP(result),0));
      }
    } else {
      mov2w (AOP(right), offset);
      emitpcode(POC_MOVWF, popGet(AOP(result),offset));
    }
	    
    offset++;
  }

    
 release:
  freeAsmop (right,NULL,ic,FALSE);
  freeAsmop (result,NULL,ic,TRUE);
}   

/*-----------------------------------------------------------------*/
/* genJumpTab - genrates code for jump table                       */
/*-----------------------------------------------------------------*/
static void genJumpTab (iCode *ic)
{
    symbol *jtab;
    char *l;

    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

    aopOp(IC_JTCOND(ic),ic,FALSE);
    /* get the condition into accumulator */
    l = aopGet(AOP(IC_JTCOND(ic)),0,FALSE,FALSE);
    MOVA(l);
    /* multiply by three */
    pic14_emitcode("add","a,acc");
    pic14_emitcode("add","a,%s",aopGet(AOP(IC_JTCOND(ic)),0,FALSE,FALSE));

    jtab = newiTempLabel(NULL);
    pic14_emitcode("mov","dptr,#%05d_DS_",jtab->key+100);
    pic14_emitcode("jmp","@a+dptr");
    pic14_emitcode("","%05d_DS_:",jtab->key+100);

    emitpcode(POC_MOVLW, popGetHighLabel(jtab->key));
    emitpcode(POC_MOVWF, popCopyReg(&pc_pclath));
    emitpcode(POC_MOVLW, popGetLabel(jtab->key));
    emitpcode(POC_ADDFW, popGet(AOP(IC_JTCOND(ic)),0));
    emitSKPNC;
    emitpcode(POC_INCF, popCopyReg(&pc_pclath));
    emitpcode(POC_MOVWF, popCopyReg(&pc_pcl));
    emitpLabel(jtab->key);

    freeAsmop(IC_JTCOND(ic),NULL,ic,TRUE);

    /* now generate the jump labels */
    for (jtab = setFirstItem(IC_JTLABELS(ic)) ; jtab;
         jtab = setNextItem(IC_JTLABELS(ic))) {
        pic14_emitcode("ljmp","%05d_DS_",jtab->key+100);
	emitpcode(POC_GOTO,popGetLabel(jtab->key));
	
    }

}

/*-----------------------------------------------------------------*/
/* genMixedOperation - gen code for operators between mixed types  */
/*-----------------------------------------------------------------*/
/*
  TSD - Written for the PIC port - but this unfortunately is buggy.
  This routine is good in that it is able to efficiently promote 
  types to different (larger) sizes. Unfortunately, the temporary
  variables that are optimized out by this routine are sometimes
  used in other places. So until I know how to really parse the 
  iCode tree, I'm going to not be using this routine :(.
*/
static int genMixedOperation (iCode *ic)
{
#if 0
  operand *result = IC_RESULT(ic);
  sym_link *ctype = operandType(IC_LEFT(ic));
  operand *right = IC_RIGHT(ic);
  int ret = 0;
  int big,small;
  int offset;

  iCode *nextic;
  operand *nextright=NULL,*nextleft=NULL,*nextresult=NULL;

  pic14_emitcode("; ***","%s  %d",__FUNCTION__,__LINE__);

  nextic = ic->next;
  if(!nextic)
    return 0;

  nextright = IC_RIGHT(nextic);
  nextleft  = IC_LEFT(nextic);
  nextresult = IC_RESULT(nextic);

  aopOp(right,ic,FALSE);
  aopOp(result,ic,FALSE);
  aopOp(nextright,  nextic, FALSE);
  aopOp(nextleft,   nextic, FALSE);
  aopOp(nextresult, nextic, FALSE);

  if (pic14_sameRegs(AOP(IC_RESULT(ic)), AOP(IC_RIGHT(nextic)))) {

    operand *t = right;
    right = nextright;
    nextright = t; 

    pic14_emitcode(";remove right +","");

  } else   if (pic14_sameRegs(AOP(IC_RESULT(ic)), AOP(IC_LEFT(nextic)))) {
/*
    operand *t = right;
    right = nextleft;
    nextleft = t; 
*/
    pic14_emitcode(";remove left +","");
  } else
    return 0;

  big = AOP_SIZE(nextleft);
  small = AOP_SIZE(nextright);

  switch(nextic->op) {

  case '+':
    pic14_emitcode(";optimize a +","");
    /* if unsigned or not an integral type */
    if (AOP_TYPE(IC_LEFT(nextic)) == AOP_CRY) {
      pic14_emitcode(";add a bit to something","");
    } else {

      pic14_emitcode("movf","%s,w",AOP(nextright)->aopu.aop_dir);

      if (!pic14_sameRegs(AOP(IC_LEFT(nextic)), AOP(IC_RESULT(nextic))) ) {
	pic14_emitcode("addwf","%s,w",AOP(nextleft)->aopu.aop_dir);
	pic14_emitcode("movwf","%s",aopGet(AOP(IC_RESULT(nextic)),0,FALSE,FALSE));
      } else
	pic14_emitcode("addwf","%s,f",AOP(nextleft)->aopu.aop_dir);

      offset = 0;
      while(--big) {

	offset++;

	if(--small) {
	  if (!pic14_sameRegs(AOP(IC_LEFT(nextic)), AOP(IC_RESULT(nextic))) ){
	    pic14_emitcode("movf","%s,w",aopGet(AOP(IC_LEFT(nextic)),offset,FALSE,FALSE));
	    pic14_emitcode("movwf","%s,f",aopGet(AOP(IC_RESULT(nextic)),offset,FALSE,FALSE) );
	  }

	  pic14_emitcode("movf","%s,w", aopGet(AOP(IC_LEFT(nextic)),offset,FALSE,FALSE));
	  emitSKPNC;
	  pic14_emitcode("btfsc","(%s >> 3), (%s & 7)",
		   AOP(IC_RIGHT(nextic))->aopu.aop_dir,
		   AOP(IC_RIGHT(nextic))->aopu.aop_dir);
	  pic14_emitcode(" incf","%s,w", aopGet(AOP(IC_LEFT(nextic)),offset,FALSE,FALSE));
	  pic14_emitcode("movwf","%s", aopGet(AOP(IC_RESULT(nextic)),offset,FALSE,FALSE));

	} else {
	  pic14_emitcode("rlf","known_zero,w");

	  /*
	    if right is signed
	      btfsc  right,7
               addlw ff
	  */
	  if (!pic14_sameRegs(AOP(IC_LEFT(nextic)), AOP(IC_RESULT(nextic))) ){
	    pic14_emitcode("addwf","%s,w",aopGet(AOP(IC_LEFT(nextic)),offset,FALSE,FALSE));
	    pic14_emitcode("movwf","%s,f",aopGet(AOP(IC_RESULT(nextic)),offset,FALSE,FALSE) );
	  } else {
	    pic14_emitcode("addwf","%s,f",aopGet(AOP(IC_RESULT(nextic)),offset,FALSE,FALSE) );
	  }
	}
      }
      ret = 1;
    }
  }
  ret = 1;

release:
  freeAsmop(right,NULL,ic,TRUE);
  freeAsmop(result,NULL,ic,TRUE);
  freeAsmop(nextright,NULL,ic,TRUE);
  freeAsmop(nextleft,NULL,ic,TRUE);
  if(ret)
    nextic->generated = 1;

  return ret;
#else
  return 0;
#endif
}
/*-----------------------------------------------------------------*/
/* genCast - gen code for casting                                  */
/*-----------------------------------------------------------------*/
static void genCast (iCode *ic)
{
    operand *result = IC_RESULT(ic);
    sym_link *ctype = operandType(IC_LEFT(ic));
    sym_link *rtype = operandType(IC_RIGHT(ic));
    operand *right = IC_RIGHT(ic);
    int size, offset ;

    DEBUGpic14_emitcode("; ***","%s  %d",__FUNCTION__,__LINE__);
    /* if they are equivalent then do nothing */
    if (operandsEqu(IC_RESULT(ic),IC_RIGHT(ic)))
        return ;

    aopOp(right,ic,FALSE) ;
    aopOp(result,ic,FALSE);

    DEBUGpic14_AopType(__LINE__,NULL,right,result);

    /* if the result is a bit */
    if (AOP_TYPE(result) == AOP_CRY) {
        /* if the right size is a literal then
        we know what the value is */
      DEBUGpic14_emitcode("; ***","%s  %d",__FUNCTION__,__LINE__);
        if (AOP_TYPE(right) == AOP_LIT) {

	  emitpcode(  ( ((int) operandLitValue(right)) ? POC_BSF : POC_BCF),
		      popGet(AOP(result),0));

            if (((int) operandLitValue(right))) 
	      pic14_emitcode("bsf","(%s >> 3), (%s & 7)",
		       AOP(result)->aopu.aop_dir,
		       AOP(result)->aopu.aop_dir);
            else
	      pic14_emitcode("bcf","(%s >> 3), (%s & 7)",
		       AOP(result)->aopu.aop_dir,
		       AOP(result)->aopu.aop_dir);

            goto release;
        }

        /* the right is also a bit variable */
        if (AOP_TYPE(right) == AOP_CRY) {

	  emitCLRC;
	  emitpcode(POC_BTFSC,  popGet(AOP(right),0));

	  pic14_emitcode("clrc","");
	  pic14_emitcode("btfsc","(%s >> 3), (%s & 7)",
		   AOP(right)->aopu.aop_dir,
		   AOP(right)->aopu.aop_dir);
            aopPut(AOP(result),"c",0);
            goto release ;
        }

        /* we need to or */
	if (AOP_TYPE(right) == AOP_REG) {
	  emitpcode(POC_BCF,    popGet(AOP(result),0));
	  emitpcode(POC_BTFSC,  newpCodeOpBit(aopGet(AOP(right),0,FALSE,FALSE),0,0));
	  emitpcode(POC_BSF,    popGet(AOP(result),0));
	}
	pic14_toBoolean(right);
	aopPut(AOP(result),"a",0);
        goto release ;
    }

    if ((AOP_TYPE(right) == AOP_CRY) && (AOP_TYPE(result) == AOP_REG)) {
      int offset = 1;
      size = AOP_SIZE(result);

      DEBUGpic14_emitcode("; ***","%s  %d",__FUNCTION__,__LINE__);

      emitpcode(POC_CLRF,   popGet(AOP(result),0));
      emitpcode(POC_BTFSC,  popGet(AOP(right),0));
      emitpcode(POC_INCF,   popGet(AOP(result),0));

      while (size--)
	emitpcode(POC_CLRF,   popGet(AOP(result),offset++));

      goto release;
    }

    /* if they are the same size : or less */
    if (AOP_SIZE(result) <= AOP_SIZE(right)) {

        /* if they are in the same place */
      if (pic14_sameRegs(AOP(right),AOP(result)))
	goto release;

      DEBUGpic14_emitcode("; ***","%s  %d",__FUNCTION__,__LINE__);
      if (IS_PTR_CONST(rtype))
	DEBUGpic14_emitcode ("; ***","%d - right is const pointer",__LINE__);
      if (IS_PTR_CONST(operandType(IC_RESULT(ic))))
	DEBUGpic14_emitcode ("; ***","%d - result is const pointer",__LINE__);

      if ((AOP_TYPE(right) == AOP_PCODE) && AOP(right)->aopu.pcop->type == PO_IMMEDIATE) {
	emitpcode(POC_MOVLW, popGet(AOP(right),0));
	emitpcode(POC_MOVWF, popGet(AOP(result),0));
	emitpcode(POC_MOVLW, popGet(AOP(right),1));
	emitpcode(POC_MOVWF, popGet(AOP(result),1));
        if(AOP_SIZE(result) <2)
	  fprintf(stderr,"%d -- result is not big enough to hold a ptr\n",__LINE__);

      } else {

        /* if they in different places then copy */
        size = AOP_SIZE(result);
        offset = 0 ;
        while (size--) {
	  emitpcode(POC_MOVFW, popGet(AOP(right),offset));
	  emitpcode(POC_MOVWF, popGet(AOP(result),offset));

	  //aopPut(AOP(result),
	  // aopGet(AOP(right),offset,FALSE,FALSE),
	  // offset);

	  offset++;
        }
      }
      goto release;
    }


    /* if the result is of type pointer */
    if (IS_PTR(ctype)) {

	int p_type;
	sym_link *type = operandType(right);
	sym_link *etype = getSpec(type);
      DEBUGpic14_emitcode("; ***","%s  %d - pointer cast",__FUNCTION__,__LINE__);

	/* pointer to generic pointer */
	if (IS_GENPTR(ctype)) {
	    char *l = zero;
	    
	    if (IS_PTR(type)) 
		p_type = DCL_TYPE(type);
	    else {
		/* we have to go by the storage class */
		p_type = PTR_TYPE(SPEC_OCLS(etype));

/* 		if (SPEC_OCLS(etype)->codesp )  */
/* 		    p_type = CPOINTER ;	 */
/* 		else */
/* 		    if (SPEC_OCLS(etype)->fmap && !SPEC_OCLS(etype)->paged) */
/* 			p_type = FPOINTER ; */
/* 		    else */
/* 			if (SPEC_OCLS(etype)->fmap && SPEC_OCLS(etype)->paged) */
/* 			    p_type = PPOINTER; */
/* 			else */
/* 			    if (SPEC_OCLS(etype) == idata ) */
/* 				p_type = IPOINTER ; */
/* 			    else */
/* 				p_type = POINTER ; */
	    }
		
	    /* the first two bytes are known */
      DEBUGpic14_emitcode("; ***","%s  %d - pointer cast2",__FUNCTION__,__LINE__);
	    size = GPTRSIZE - 1; 
	    offset = 0 ;
	    while (size--) {
	      if(offset < AOP_SIZE(right)) {
      DEBUGpic14_emitcode("; ***","%s  %d - pointer cast3",__FUNCTION__,__LINE__);
		if ((AOP_TYPE(right) == AOP_PCODE) && 
		    AOP(right)->aopu.pcop->type == PO_IMMEDIATE) {
		  emitpcode(POC_MOVLW, popGet(AOP(right),offset));
		  emitpcode(POC_MOVWF, popGet(AOP(result),offset));
		} else { 
		  aopPut(AOP(result),
			 aopGet(AOP(right),offset,FALSE,FALSE),
			 offset);
		}
	      } else 
		emitpcode(POC_CLRF,popGet(AOP(result),offset));
	      offset++;
	    }
	    /* the last byte depending on type */
	    switch (p_type) {
	    case IPOINTER:
	    case POINTER:
		emitpcode(POC_CLRF,popGet(AOP(result),GPTRSIZE - 1));
		break;
	    case FPOINTER:
	      pic14_emitcode(";BUG!? ","%d",__LINE__);
		l = one;
		break;
	    case CPOINTER:
	      pic14_emitcode(";BUG!? ","%d",__LINE__);
		l = "#0x02";
		break;				
	    case PPOINTER:
	      pic14_emitcode(";BUG!? ","%d",__LINE__);
		l = "#0x03";
		break;
		
	    default:
		/* this should never happen */
		werror(E_INTERNAL_ERROR,__FILE__,__LINE__,
		       "got unknown pointer type");
		exit(1);
	    }
	    //aopPut(AOP(result),l, GPTRSIZE - 1);	    
	    goto release ;
	}
	
	/* just copy the pointers */
	size = AOP_SIZE(result);
	offset = 0 ;
	while (size--) {
	    aopPut(AOP(result),
		   aopGet(AOP(right),offset,FALSE,FALSE),
		   offset);
	    offset++;
	}
	goto release ;
    }
    


    /* so we now know that the size of destination is greater
    than the size of the source.
    Now, if the next iCode is an operator then we might be
    able to optimize the operation without performing a cast.
    */
    if(genMixedOperation(ic))
      goto release;

    DEBUGpic14_emitcode("; ***","%s  %d",__FUNCTION__,__LINE__);
    
    /* we move to result for the size of source */
    size = AOP_SIZE(right);
    offset = 0 ;
    while (size--) {
      emitpcode(POC_MOVFW,   popGet(AOP(right),offset));
      emitpcode(POC_MOVWF,   popGet(AOP(result),offset));
      offset++;
    }

    /* now depending on the sign of the destination */
    size = AOP_SIZE(result) - AOP_SIZE(right);
    /* if unsigned or not an integral type */
    if (SPEC_USIGN(rtype) || !IS_SPEC(rtype)) {
      while (size--)
	emitpcode(POC_CLRF,   popGet(AOP(result),offset++));
    } else {
      /* we need to extend the sign :{ */

      if(size == 1) {
	/* Save one instruction of casting char to int */
	emitpcode(POC_CLRF,   popGet(AOP(result),offset));
	emitpcode(POC_BTFSC,  newpCodeOpBit(aopGet(AOP(right),offset-1,FALSE,FALSE),7,0));
	emitpcode(POC_DECF,   popGet(AOP(result),offset));
      } else {
	emitpcodeNULLop(POC_CLRW);

	if(offset)
	  emitpcode(POC_BTFSC,   newpCodeOpBit(aopGet(AOP(right),offset-1,FALSE,FALSE),7,0));
	else
	  emitpcode(POC_BTFSC,   newpCodeOpBit(aopGet(AOP(right),offset,FALSE,FALSE),7,0));
	
	emitpcode(POC_MOVLW,   popGetLit(0xff));

        while (size--)
	  emitpcode(POC_MOVWF,   popGet(AOP(result),offset++));
      }
    }

release:
    freeAsmop(right,NULL,ic,TRUE);
    freeAsmop(result,NULL,ic,TRUE);

}

/*-----------------------------------------------------------------*/
/* genDjnz - generate decrement & jump if not zero instrucion      */
/*-----------------------------------------------------------------*/
static int genDjnz (iCode *ic, iCode *ifx)
{
    symbol *lbl, *lbl1;
    DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

    if (!ifx)
	return 0;
    
    /* if the if condition has a false label
       then we cannot save */
    if (IC_FALSE(ifx))
	return 0;

    /* if the minus is not of the form 
       a = a - 1 */
    if (!isOperandEqual(IC_RESULT(ic),IC_LEFT(ic)) ||
	!IS_OP_LITERAL(IC_RIGHT(ic)))
	return 0;

    if (operandLitValue(IC_RIGHT(ic)) != 1)
	return 0;

    /* if the size of this greater than one then no
       saving */
    if (getSize(operandType(IC_RESULT(ic))) > 1)
	return 0;

    /* otherwise we can save BIG */
    lbl = newiTempLabel(NULL);
    lbl1= newiTempLabel(NULL);

    aopOp(IC_RESULT(ic),ic,FALSE);
    
    if (IS_AOP_PREG(IC_RESULT(ic))) {
	pic14_emitcode("dec","%s",
		 aopGet(AOP(IC_RESULT(ic)),0,FALSE,FALSE));
	pic14_emitcode("mov","a,%s",aopGet(AOP(IC_RESULT(ic)),0,FALSE,FALSE));
	pic14_emitcode("jnz","%05d_DS_",lbl->key+100);
    } else {	


      emitpcode(POC_DECFSZ,popGet(AOP(IC_RESULT(ic)),0));
      emitpcode(POC_GOTO,popGetLabel(IC_TRUE(ifx)->key));

      pic14_emitcode("decfsz","%s,f",aopGet(AOP(IC_RESULT(ic)),0,FALSE,FALSE));
      pic14_emitcode ("goto","_%05d_DS_",IC_TRUE(ifx)->key+100 + labelOffset);

    }
/*     pic14_emitcode ("sjmp","%05d_DS_",lbl1->key+100); */
/*     pic14_emitcode ("","%05d_DS_:",lbl->key+100); */
/*     pic14_emitcode ("ljmp","%05d_DS_",IC_TRUE(ifx)->key+100); */
/*     pic14_emitcode ("","%05d_DS_:",lbl1->key+100); */

    
    freeAsmop(IC_RESULT(ic),NULL,ic,TRUE);
    ifx->generated = 1;
    return 1;
}

/*-----------------------------------------------------------------*/
/* genReceive - generate code for a receive iCode                  */
/*-----------------------------------------------------------------*/
static void genReceive (iCode *ic)
{
  DEBUGpic14_emitcode ("; ***","%s  %d",__FUNCTION__,__LINE__);

  if (isOperandInFarSpace(IC_RESULT(ic)) &&
      ( OP_SYMBOL(IC_RESULT(ic))->isspilt ||
	IS_TRUE_SYMOP(IC_RESULT(ic))) ) {

    int size = getSize(operandType(IC_RESULT(ic)));
    int offset =  fReturnSizePic - size;
    while (size--) {
      pic14_emitcode ("push","%s", (strcmp(fReturn[fReturnSizePic - offset - 1],"a") ?
				    fReturn[fReturnSizePic - offset - 1] : "acc"));
      offset++;
    }
    aopOp(IC_RESULT(ic),ic,FALSE);
    size = AOP_SIZE(IC_RESULT(ic));
    offset = 0;
    while (size--) {
      pic14_emitcode ("pop","acc");
      aopPut (AOP(IC_RESULT(ic)),"a",offset++);
    }

  } else {
    _G.accInUse++;
    aopOp(IC_RESULT(ic),ic,FALSE);
    _G.accInUse--;
    assignResultValue(IC_RESULT(ic));
  }

  freeAsmop(IC_RESULT(ic),NULL,ic,TRUE);
}

/*-----------------------------------------------------------------*/
/* genDummyRead - generate code for dummy read of volatiles        */
/*-----------------------------------------------------------------*/
static void
genDummyRead (iCode * ic)
{
  pic14_emitcode ("; genDummyRead","");
  pic14_emitcode ("; not implemented","");

  ic = ic;
}

/*-----------------------------------------------------------------*/
/* genpic14Code - generate code for pic14 based controllers        */
/*-----------------------------------------------------------------*/
/*
 * At this point, ralloc.c has gone through the iCode and attempted
 * to optimize in a way suitable for a PIC. Now we've got to generate
 * PIC instructions that correspond to the iCode.
 *
 * Once the instructions are generated, we'll pass through both the
 * peep hole optimizer and the pCode optimizer.
 *-----------------------------------------------------------------*/

void genpic14Code (iCode *lic)
{
    iCode *ic;
    int cln = 0;

    lineHead = lineCurr = NULL;

    pb = newpCodeChain(GcurMemmap,0,newpCodeCharP("; Starting pCode block"));
    addpBlock(pb);

    /* if debug information required */
    if (options.debug && currFunc) { 
      if (currFunc) {
	debugFile->writeFunction(currFunc);
	_G.debugLine = 1;
	if (IS_STATIC(currFunc->etype)) {
	  pic14_emitcode("",";F%s$%s$0$0     %d",moduleName,currFunc->name,__LINE__);
	  //addpCode2pBlock(pb,newpCodeLabel(moduleName,currFunc->name));
	} else {
	  pic14_emitcode("",";G$%s$0$0   %d",currFunc->name,__LINE__);
	  //addpCode2pBlock(pb,newpCodeLabel(NULL,currFunc->name));
	}
	_G.debugLine = 0;
      }
    }


    for (ic = lic ; ic ; ic = ic->next ) {

      DEBUGpic14_emitcode(";ic","");
	if ( cln != ic->lineno ) {
	    if ( options.debug ) {
		_G.debugLine = 1;
		pic14_emitcode("",";C$%s$%d$%d$%d ==.",
			 FileBaseName(ic->filename),ic->lineno,
			 ic->level,ic->block);
		_G.debugLine = 0;
	    }
	    /*
	      pic14_emitcode("#CSRC","%s %d",FileBaseName(ic->filename),ic->lineno);
	      pic14_emitcode ("", ";\t%s:%d: %s", ic->filename, ic->lineno, 
	      printCLine(ic->filename, ic->lineno));
	    */
	    if (!options.noCcodeInAsm) {
	      addpCode2pBlock(pb,
			      newpCodeCSource(ic->lineno, 
					      ic->filename, 
					      printCLine(ic->filename, ic->lineno)));
	    }

	    cln = ic->lineno ;
	}

	// if you want printILine too, look at ../mcs51/gen.c, i don't understand this :)

	/* if the result is marked as
	   spilt and rematerializable or code for
	   this has already been generated then
	   do nothing */
	if (resultRemat(ic) || ic->generated ) 
	    continue ;
	
	/* depending on the operation */
	switch (ic->op) {
	case '!' :
	    genNot(ic);
	    break;
	    
	case '~' :
	    genCpl(ic);
	    break;
	    
	case UNARYMINUS:
	    genUminus (ic);
	    break;
	    
	case IPUSH:
	    genIpush (ic);
	    break;
	    
	case IPOP:
	    /* IPOP happens only when trying to restore a 
	       spilt live range, if there is an ifx statement
	       following this pop then the if statement might
	       be using some of the registers being popped which
	       would destory the contents of the register so
	       we need to check for this condition and handle it */
	    if (ic->next            && 
		ic->next->op == IFX &&
		regsInCommon(IC_LEFT(ic),IC_COND(ic->next))) 
		genIfx (ic->next,ic);
	    else
		genIpop (ic);
	    break; 
	    
	case CALL:
	    genCall (ic);
	    break;
	    
	case PCALL:
	    genPcall (ic);
	    break;
	    
	case FUNCTION:
	    genFunction (ic);
	    break;
	    
	case ENDFUNCTION:
	    genEndFunction (ic);
	    break;
	    
	case RETURN:
	    genRet (ic);
	    break;
	    
	case LABEL:
	    genLabel (ic);
	    break;
	    
	case GOTO:
	    genGoto (ic);
	    break;
	    
	case '+' :
	    genPlus (ic) ;
	    break;
	    
	case '-' :
	    if ( ! genDjnz (ic,ifxForOp(IC_RESULT(ic),ic)))
		genMinus (ic);
	    break;
	    
	case '*' :
	    genMult (ic);
	    break;
	    
	case '/' :
	    genDiv (ic) ;
	    break;
	    
	case '%' :
	    genMod (ic);
	    break;
	    
	case '>' :
	    genCmpGt (ic,ifxForOp(IC_RESULT(ic),ic));		      
	    break;
	    
	case '<' :
	    genCmpLt (ic,ifxForOp(IC_RESULT(ic),ic));
	    break;
	    
	case LE_OP:
	case GE_OP:
	case NE_OP:
	    
	    /* note these two are xlated by algebraic equivalence
	       during parsing SDCC.y */
	    werror(E_INTERNAL_ERROR,__FILE__,__LINE__,
		   "got '>=' or '<=' shouldn't have come here");
	    break;	
	    
	case EQ_OP:
	    genCmpEq (ic,ifxForOp(IC_RESULT(ic),ic));
	    break;	    
	    
	case AND_OP:
	    genAndOp (ic);
	    break;
	    
	case OR_OP:
	    genOrOp (ic);
	    break;
	    
	case '^' :
	    genXor (ic,ifxForOp(IC_RESULT(ic),ic));
	    break;
	    
	case '|' :
		genOr (ic,ifxForOp(IC_RESULT(ic),ic));
	    break;
	    
	case BITWISEAND:
            genAnd (ic,ifxForOp(IC_RESULT(ic),ic));
	    break;
	    
	case INLINEASM:
	    genInline (ic);
	    break;
	    
	case RRC:
	    genRRC (ic);
	    break;
	    
	case RLC:
	    genRLC (ic);
	    break;
	    
	case GETHBIT:
	    genGetHbit (ic);
	    break;
	    
	case LEFT_OP:
	    genLeftShift (ic);
	    break;
	    
	case RIGHT_OP:
	    genRightShift (ic);
	    break;
	    
	case GET_VALUE_AT_ADDRESS:
	    genPointerGet(ic);
	    break;
	    
	case '=' :
	    if (POINTER_SET(ic))
		genPointerSet(ic);
	    else
		genAssign(ic);
	    break;
	    
	case IFX:
	    genIfx (ic,NULL);
	    break;
	    
	case ADDRESS_OF:
	    genAddrOf (ic);
	    break;
	    
	case JUMPTABLE:
	    genJumpTab (ic);
	    break;
	    
	case CAST:
	    genCast (ic);
	    break;
	    
	case RECEIVE:
	    genReceive(ic);
	    break;
	    
	case SEND:
	    addSet(&_G.sendSet,ic);
	    break;

	case DUMMY_READ_VOLATILE:
	  genDummyRead (ic);
	  break;

	default :
	    ic = ic;
        }
    }


    /* now we are ready to call the
       peep hole optimizer */
    if (!options.nopeep) {
      peepHole (&lineHead);
    }
    /* now do the actual printing */
    printLine (lineHead,codeOutFile);

#ifdef PCODE_DEBUG
    DFPRINTF((stderr,"printing pBlock\n\n"));
    printpBlock(stdout,pb);
#endif

    return;
}
