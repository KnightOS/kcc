/*------------------------------------------------------------------------

  ralloc.c - source file for register allocation. PIC16 specific

                Written By -  Sandeep Dutta . sandeep.dutta@usa.net (1998)
		Added Pic Port T.scott Dattalo scott@dattalo.com (2000)
		Added Pic16 Port Martin Dubuc m.dubuc@rogers.com (2002)

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
#include "pcode.h"
#include "gen.h"

#if defined(__BORLANDC__) || defined(_MSC_VER)
#define STRCASECMP stricmp
#else
#define STRCASECMP strcasecmp
#endif

/*-----------------------------------------------------------------*/
/* At this point we start getting processor specific although      */
/* some routines are non-processor specific & can be reused when   */
/* targetting other processors. The decision for this will have    */
/* to be made on a routine by routine basis                        */
/* routines used to pack registers are most definitely not reusable */
/* since the pack the registers depending strictly on the MCU      */
/*-----------------------------------------------------------------*/

static regs *typeRegWithIdx (int idx, int type, int fixed);
extern void genpic16Code (iCode *);
extern void pic16_assignConfigWordValue(int address, int value);

/* Global data */
static struct
  {
    bitVect *spiltSet;
    set *stackSpil;
    bitVect *regAssigned;
    short blockSpil;
    int slocNum;
    bitVect *funcrUsed;		/* registers used in a function */
    int stackExtend;
    int dataExtend;
  }
_G;

/* Shared with gen.c */
int pic16_ptrRegReq;		/* one byte pointer register required */


set *pic16_dynAllocRegs=NULL;
set *pic16_dynStackRegs=NULL;
set *pic16_dynProcessorRegs=NULL;
set *pic16_dynDirectRegs=NULL;
set *pic16_dynDirectBitRegs=NULL;
set *pic16_dynInternalRegs=NULL;

static hTab  *dynDirectRegNames= NULL;
//static hTab  *regHash = NULL;    /* a hash table containing ALL registers */

static int dynrIdx=0x20;
static int rDirectIdx=0;

int pic16_nRegs = 128;   // = sizeof (regspic16) / sizeof (regs);

int pic16_Gstack_base_addr=0; /* The starting address of registers that
			 * are used to pass and return parameters */




static void spillThis (symbol *);
static int debug = 1;
static FILE *debugF = NULL;
/*-----------------------------------------------------------------*/
/* debugLog - open a file for debugging information                */
/*-----------------------------------------------------------------*/
//static void debugLog(char *inst,char *fmt, ...)
static void
debugLog (char *fmt,...)
{
  static int append = 0;	// First time through, open the file without append.

  char buffer[256];
  //char *bufferP=buffer;
  va_list ap;

  if (!debug || !dstFileName)
    return;


  if (!debugF)
    {
      /* create the file name */
      strcpy (buffer, dstFileName);
      strcat (buffer, ".d");

      if (!(debugF = fopen (buffer, (append ? "a+" : "w"))))
	{
	  werror (E_FILE_OPEN_ERR, buffer);
	  exit (1);
	}
      append = 1;		// Next time debubLog is called, we'll append the debug info

    }

  va_start (ap, fmt);

  vsprintf (buffer, fmt, ap);

  fprintf (debugF, "%s", buffer);
/*
   while (isspace(*bufferP)) bufferP++;

   if (bufferP && *bufferP) 
   lineCurr = (lineCurr ?
   connectLine(lineCurr,newLineNode(lb)) :
   (lineHead = newLineNode(lb)));
   lineCurr->isInline = _G.inLine;
   lineCurr->isDebug  = _G.debugLine;
 */
  va_end (ap);

}

static void
debugNewLine (void)
{
  if (debugF)
    fputc ('\n', debugF);
}
/*-----------------------------------------------------------------*/
/* debugLogClose - closes the debug log file (if opened)           */
/*-----------------------------------------------------------------*/
static void
debugLogClose (void)
{
  if (debugF)
    {
      fclose (debugF);
      debugF = NULL;
    }
}
#define AOP(op) op->aop

static char *
debugAopGet (char *str, operand * op)
{
  if (str)
    debugLog (str);

  printOperand (op, debugF);
  debugNewLine ();

  return NULL;

}

static char *
decodeOp (unsigned int op)
{

  if (op < 128 && op > ' ')
    {
      buffer[0] = (op & 0xff);
      buffer[1] = 0;
      return buffer;
    }

  switch (op)
    {
    case IDENTIFIER:
      return "IDENTIFIER";
    case TYPE_NAME:
      return "TYPE_NAME";
    case CONSTANT:
      return "CONSTANT";
    case STRING_LITERAL:
      return "STRING_LITERAL";
    case SIZEOF:
      return "SIZEOF";
    case PTR_OP:
      return "PTR_OP";
    case INC_OP:
      return "INC_OP";
    case DEC_OP:
      return "DEC_OP";
    case LEFT_OP:
      return "LEFT_OP";
    case RIGHT_OP:
      return "RIGHT_OP";
    case LE_OP:
      return "LE_OP";
    case GE_OP:
      return "GE_OP";
    case EQ_OP:
      return "EQ_OP";
    case NE_OP:
      return "NE_OP";
    case AND_OP:
      return "AND_OP";
    case OR_OP:
      return "OR_OP";
    case MUL_ASSIGN:
      return "MUL_ASSIGN";
    case DIV_ASSIGN:
      return "DIV_ASSIGN";
    case MOD_ASSIGN:
      return "MOD_ASSIGN";
    case ADD_ASSIGN:
      return "ADD_ASSIGN";
    case SUB_ASSIGN:
      return "SUB_ASSIGN";
    case LEFT_ASSIGN:
      return "LEFT_ASSIGN";
    case RIGHT_ASSIGN:
      return "RIGHT_ASSIGN";
    case AND_ASSIGN:
      return "AND_ASSIGN";
    case XOR_ASSIGN:
      return "XOR_ASSIGN";
    case OR_ASSIGN:
      return "OR_ASSIGN";
    case TYPEDEF:
      return "TYPEDEF";
    case EXTERN:
      return "EXTERN";
    case STATIC:
      return "STATIC";
    case AUTO:
      return "AUTO";
    case REGISTER:
      return "REGISTER";
    case CODE:
      return "CODE";
    case EEPROM:
      return "EEPROM";
    case INTERRUPT:
      return "INTERRUPT";
    case SFR:
      return "SFR";
    case AT:
      return "AT";
    case SBIT:
      return "SBIT";
    case REENTRANT:
      return "REENTRANT";
    case USING:
      return "USING";
    case XDATA:
      return "XDATA";
    case DATA:
      return "DATA";
    case IDATA:
      return "IDATA";
    case PDATA:
      return "PDATA";
    case VAR_ARGS:
      return "VAR_ARGS";
    case CRITICAL:
      return "CRITICAL";
    case NONBANKED:
      return "NONBANKED";
    case BANKED:
      return "BANKED";
    case CHAR:
      return "CHAR";
    case SHORT:
      return "SHORT";
    case INT:
      return "INT";
    case LONG:
      return "LONG";
    case SIGNED:
      return "SIGNED";
    case UNSIGNED:
      return "UNSIGNED";
    case FLOAT:
      return "FLOAT";
    case DOUBLE:
      return "DOUBLE";
    case CONST:
      return "CONST";
    case VOLATILE:
      return "VOLATILE";
    case VOID:
      return "VOID";
    case BIT:
      return "BIT";
    case STRUCT:
      return "STRUCT";
    case UNION:
      return "UNION";
    case ENUM:
      return "ENUM";
    case ELIPSIS:
      return "ELIPSIS";
    case RANGE:
      return "RANGE";
    case FAR:
      return "FAR";
    case CASE:
      return "CASE";
    case DEFAULT:
      return "DEFAULT";
    case IF:
      return "IF";
    case ELSE:
      return "ELSE";
    case SWITCH:
      return "SWITCH";
    case WHILE:
      return "WHILE";
    case DO:
      return "DO";
    case FOR:
      return "FOR";
    case GOTO:
      return "GOTO";
    case CONTINUE:
      return "CONTINUE";
    case BREAK:
      return "BREAK";
    case RETURN:
      return "RETURN";
    case INLINEASM:
      return "INLINEASM";
    case IFX:
      return "IFX";
    case ADDRESS_OF:
      return "ADDRESS_OF";
    case GET_VALUE_AT_ADDRESS:
      return "GET_VALUE_AT_ADDRESS";
    case SPIL:
      return "SPIL";
    case UNSPIL:
      return "UNSPIL";
    case GETHBIT:
      return "GETHBIT";
    case BITWISEAND:
      return "BITWISEAND";
    case UNARYMINUS:
      return "UNARYMINUS";
    case IPUSH:
      return "IPUSH";
    case IPOP:
      return "IPOP";
    case PCALL:
      return "PCALL";
    case ENDFUNCTION:
      return "ENDFUNCTION";
    case JUMPTABLE:
      return "JUMPTABLE";
    case RRC:
      return "RRC";
    case RLC:
      return "RLC";
    case CAST:
      return "CAST";
    case CALL:
      return "CALL";
    case PARAM:
      return "PARAM  ";
    case NULLOP:
      return "NULLOP";
    case BLOCK:
      return "BLOCK";
    case LABEL:
      return "LABEL";
    case RECEIVE:
      return "RECEIVE";
    case SEND:
      return "SEND";
    }
  sprintf (buffer, "unkown op %d %c", op, op & 0xff);
  return buffer;
}
/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
static char *
debugLogRegType (short type)
{

  switch (type)
    {
    case REG_GPR:
      return "REG_GPR";
    case REG_PTR:
      return "REG_PTR";
    case REG_CND:
      return "REG_CND";
    }

  sprintf (buffer, "unknown reg type %d", type);
  return buffer;
}

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
static int regname2key(char const *name)
{
  int key = 0;

  if(!name)
    return 0;

  while(*name) {

    key += (*name++) + 1;

  }

  return ( (key + (key >> 4) + (key>>8)) & 0x3f);

}

/*-----------------------------------------------------------------*/
/* newReg - allocate and init memory for a new register            */
/*-----------------------------------------------------------------*/
static regs* newReg(short type, short pc_type, int rIdx, char *name, int size, int alias)
{

  regs *dReg;

  dReg = Safe_calloc(1,sizeof(regs));
  dReg->type = type;
  dReg->pc_type = pc_type;
  dReg->rIdx = rIdx;
  if(name) 
    dReg->name = Safe_strdup(name);
  else {
    sprintf(buffer,"r0x%02X", dReg->rIdx);
    if(type == REG_STK)
      *buffer = 's';
    dReg->name = Safe_strdup(buffer);
  }
  //fprintf(stderr,"newReg: %s, rIdx = 0x%02x\n",dReg->name,rIdx);
  dReg->isFree = 0;
  dReg->wasUsed = 1;
  if(type == REG_SFR) {
    dReg->isFixed = 1;
    dReg->address = rIdx;
  } else {
    dReg->isFixed = 0;
    dReg->address = 0;
  }

  dReg->isMapped = 0;
  dReg->isEmitted = 0;
  dReg->size = size;
  dReg->alias = alias;
  dReg->reg_alias = NULL;
  dReg->reglives.usedpFlows = newSet();
  dReg->reglives.assignedpFlows = newSet();

  hTabAddItem(&dynDirectRegNames, regname2key(name), dReg);

  return dReg;
}

/*-----------------------------------------------------------------*/
/* regWithIdx - Search through a set of registers that matches idx */
/*-----------------------------------------------------------------*/
static regs *
regWithIdx (set *dRegs, int idx, int fixed)
{
  regs *dReg;

  for (dReg = setFirstItem(dRegs) ; dReg ; 
       dReg = setNextItem(dRegs)) {

    if(idx == dReg->rIdx && (fixed == dReg->isFixed)) {
      return dReg;
    }
  }

  return NULL;
}

/*-----------------------------------------------------------------*/
/* regFindFree - Search for a free register in a set of registers  */
/*-----------------------------------------------------------------*/
static regs *
regFindFree (set *dRegs)
{
  regs *dReg;

  for (dReg = setFirstItem(dRegs) ; dReg ; 
       dReg = setNextItem(dRegs)) {

    if(dReg->isFree)
      return dReg;
  }

  return NULL;
}
/*-----------------------------------------------------------------*/
/* pic16_initStack - allocate registers for a pseudo stack               */
/*-----------------------------------------------------------------*/
void pic16_initStack(int base_address, int size)
{

  int i;

  pic16_Gstack_base_addr = base_address;
  //fprintf(stderr,"initStack");

  for(i = 0; i<size; i++)
    addSet(&pic16_dynStackRegs,newReg(REG_STK, PO_GPR_TEMP,base_address++,NULL,1,0));
}

/*-----------------------------------------------------------------*
 *-----------------------------------------------------------------*/
regs *
pic16_allocProcessorRegister(int rIdx, char * name, short po_type, int alias)
{
  regs * reg = newReg(REG_SFR, po_type, rIdx, name, 1, alias);

	if(reg)reg->wasUsed = 0;
	
  //fprintf(stderr,"pic16_allocProcessorRegister %s addr =0x%x\n",name,rIdx);
  return addSet(&pic16_dynProcessorRegs, reg);
}

/*-----------------------------------------------------------------*
 *-----------------------------------------------------------------*/

regs *
pic16_allocInternalRegister(int rIdx, char * name, short po_type, int alias)
{
  regs * reg = newReg(REG_GPR, po_type, rIdx, name,1,alias);

  //fprintf(stderr,"pic16_allocInternalRegister %s addr =0x%x\n",name,rIdx);
  if(reg) {
    reg->wasUsed = 0;
    return addSet(&pic16_dynInternalRegs,reg);
  }

  return NULL;
}
/*-----------------------------------------------------------------*/
/* allocReg - allocates register of given type                     */
/*-----------------------------------------------------------------*/
static regs *
allocReg (short type)
{

  debugLog ("%s of type %s\n", __FUNCTION__, debugLogRegType (type));
  //fprintf(stderr,"allocReg\n");


  return addSet(&pic16_dynAllocRegs,newReg(REG_GPR, PO_GPR_TEMP,dynrIdx++,NULL,1,0));

}


/*-----------------------------------------------------------------*/
/* pic16_dirregWithName - search for register by name                    */
/*-----------------------------------------------------------------*/
regs *
pic16_dirregWithName (char *name)
{
  int hkey;
  regs *reg;

  if(!name)
    return NULL;

  /* hash the name to get a key */

  hkey = regname2key(name);

//	fprintf(stderr, "%s:%d: name = %s\thash = %d\n", __FUNCTION__, __LINE__, name, hkey);

  reg = hTabFirstItemWK(dynDirectRegNames, hkey);

  while(reg) {

    if(STRCASECMP(reg->name, name) == 0) {
      return(reg);
    }

    reg = hTabNextItemWK (dynDirectRegNames);
  
  }

  return NULL; // name wasn't found in the hash table
}

static int IS_CONFIG_ADDRESS(int address)
{

  return address >= 0x300000 && address <= 0x300000d;
}

/*-----------------------------------------------------------------*/
/* pic16_allocDirReg - allocates register of given type                  */
/*-----------------------------------------------------------------*/
regs *
pic16_allocDirReg (operand *op )
{

  regs *reg;
  char *name;

  if(!IS_SYMOP(op)) {
    debugLog ("%s BAD, op is NULL\n", __FUNCTION__);
    return NULL;
  }

  name = OP_SYMBOL (op)->rname[0] ? OP_SYMBOL (op)->rname : OP_SYMBOL (op)->name;

  /* If the symbol is at a fixed address, then remove the leading underscore
   * from the name. This is hack to allow the .asm include file named registers
   * to match the .c declared register names */

  //if (SPEC_ABSA ( OP_SYM_ETYPE(op)) && (*name == '_'))
  //name++;

  debugLog ("%s symbol name %s\n", __FUNCTION__,name);
  {
    if(SPEC_CONST ( OP_SYM_ETYPE(op)) && (IS_CHAR ( OP_SYM_ETYPE(op)) )) {
      debugLog(" %d  const char\n",__LINE__);
      debugLog(" value = %s \n",SPEC_CVAL( OP_SYM_ETYPE(op)));
    }

    debugLog("  %d  storage class %d \n",__LINE__,SPEC_SCLS( OP_SYM_ETYPE(op)));
    if (IS_CODE ( OP_SYM_ETYPE(op)) )
      debugLog(" %d  code space\n",__LINE__);

    if (IS_INTEGRAL ( OP_SYM_ETYPE(op)) )
      debugLog(" %d  integral\n",__LINE__);
    if (IS_LITERAL ( OP_SYM_ETYPE(op)) )
      debugLog(" %d  literal\n",__LINE__);
    if (IS_SPEC ( OP_SYM_ETYPE(op)) )
      debugLog(" %d  specifier\n",__LINE__);
    debugAopGet(NULL, op);
  }

  if (IS_CODE ( OP_SYM_ETYPE(op)) )
    return NULL;

  /* First, search the hash table to see if there is a register with this name */
  if (SPEC_ABSA ( OP_SYM_ETYPE(op)) && !(IS_BITVAR (OP_SYM_ETYPE(op))) ) {
    reg = regWithIdx (pic16_dynProcessorRegs, SPEC_ADDR ( OP_SYM_ETYPE(op)), 1);

#if 0
    if(!reg) 
      fprintf(stderr,"%s:%d: ralloc %s is at fixed address but not a processor reg, addr=0x%x\n",
      		__FUNCTION__, __LINE__, name, SPEC_ADDR ( OP_SYM_ETYPE(op)));
    else
      fprintf(stderr,"%s:%d: ralloc %s at fixed address has already been declared, addr=0x%x\n",
		__FUNCTION__, __LINE__, name, SPEC_ADDR ( OP_SYM_ETYPE(op)));
#endif
  } else {
//  	fprintf(stderr,"ralloc:%d %s \n", __LINE__,name);
    
    reg = pic16_dirregWithName(name);
  }

  if(!reg) {
    int address = 0;

    /* if this is at an absolute address, then get the address. */
    if (SPEC_ABSA ( OP_SYM_ETYPE(op)) ) {
      address = SPEC_ADDR ( OP_SYM_ETYPE(op));
      //fprintf(stderr,"reg %s is at an absolute address: 0x%03x\n",name,address);
    }

    /* Register wasn't found in hash, so let's create
     * a new one and put it in the hash table AND in the 
     * dynDirectRegNames set */
    if(!IS_CONFIG_ADDRESS(address)) {
      //fprintf(stderr,"allocating new reg %s\n",name);

      reg = newReg(REG_GPR, PO_DIR, rDirectIdx++, name,getSize (OP_SYMBOL (op)->type),0 );
      debugLog ("  -- added %s to hash, size = %d\n", name,reg->size);

      //hTabAddItem(&dynDirectRegNames, regname2key(name), reg);

      if (SPEC_ABSA ( OP_SYM_ETYPE(op)) ) {

	//fprintf(stderr, " ralloc.c at fixed address: %s - changing to REG_SFR\n",name);
	reg->type = REG_SFR;
      }

      if (IS_BITVAR (OP_SYM_ETYPE(op))) {
	addSet(&pic16_dynDirectBitRegs, reg);
	reg->isBitField = 1;
      } else
	addSet(&pic16_dynDirectRegs, reg);

    } else {
      debugLog ("  -- %s is declared at address 0x30000x\n",name);

    }
  }

  if (SPEC_ABSA ( OP_SYM_ETYPE(op)) ) {
    reg->isFixed = 1;
    reg->address = SPEC_ADDR ( OP_SYM_ETYPE(op));
    debugLog ("  -- and it is at a fixed address 0x%02x\n",reg->address);
  }

  return reg;
}

/*-----------------------------------------------------------------*/
/* pic16_allocRegByName - allocates register of given type                  */
/*-----------------------------------------------------------------*/
regs *
pic16_allocRegByName (char *name, int size)
{

  regs *reg;

  if(!name) {
    fprintf(stderr, "%s - allocating a NULL register\n",__FUNCTION__);
    exit(1);
  }

  /* First, search the hash table to see if there is a register with this name */
  reg = pic16_dirregWithName(name);

  if(!reg) {

    /* Register wasn't found in hash, so let's create
     * a new one and put it in the hash table AND in the 
     * dynDirectRegNames set */
    //fprintf (stderr,"%s symbol name %s\n", __FUNCTION__,name);
    reg = newReg(REG_GPR, PO_DIR, rDirectIdx++, name,size,0 );

    debugLog ("  -- added %s to hash, size = %d\n", name,reg->size);

    //hTabAddItem(&dynDirectRegNames, regname2key(name), reg);
    addSet(&pic16_dynDirectRegs, reg);
  }

  return reg;
}

/*-----------------------------------------------------------------*/
/* RegWithIdx - returns pointer to register with index number       */
/*-----------------------------------------------------------------*/
static regs *
typeRegWithIdx (int idx, int type, int fixed)
{

  regs *dReg;

  debugLog ("%s - requesting index = 0x%x\n", __FUNCTION__,idx);

  switch (type) {

  case REG_GPR:
    if( (dReg = regWithIdx ( pic16_dynAllocRegs, idx, fixed)) != NULL) {

      debugLog ("Found a Dynamic Register!\n");
      return dReg;
    }
    if( (dReg = regWithIdx ( pic16_dynDirectRegs, idx, fixed)) != NULL ) {
      debugLog ("Found a Direct Register!\n");
      return dReg;
    }

    break;
  case REG_STK:
    if( (dReg = regWithIdx ( pic16_dynStackRegs, idx, fixed)) != NULL ) {
      debugLog ("Found a Stack Register!\n");
      return dReg;
    }
    break;
  case REG_SFR:
    if( (dReg = regWithIdx ( pic16_dynProcessorRegs, idx, fixed)) != NULL ) {
      debugLog ("Found a Processor Register!\n");
      return dReg;
    }

  case REG_CND:
  case REG_PTR:
  default:
    break;
  }


  return NULL;
}

/*-----------------------------------------------------------------*/
/* pic16_regWithIdx - returns pointer to register with index number*/
/*-----------------------------------------------------------------*/
regs *
pic16_regWithIdx (int idx)
{
  regs *dReg;

  if( (dReg = typeRegWithIdx(idx,REG_GPR,0)) != NULL)
    return dReg;

  if( (dReg = typeRegWithIdx(idx,REG_SFR,0)) != NULL)
    return dReg;

  if( (dReg = typeRegWithIdx(idx,REG_STK,0)) != NULL)
    return dReg;

  return NULL;
}

/*-----------------------------------------------------------------*/
/* pic16_regWithIdx - returns pointer to register with index number       */
/*-----------------------------------------------------------------*/
regs *
pic16_allocWithIdx (int idx)
{

  regs *dReg;

  debugLog ("%s - allocating with index = 0x%x\n", __FUNCTION__,idx);

  if( (dReg = regWithIdx ( pic16_dynAllocRegs, idx,0)) != NULL) {

    debugLog ("Found a Dynamic Register!\n");
  } else if( (dReg = regWithIdx ( pic16_dynStackRegs, idx,0)) != NULL ) {
    debugLog ("Found a Stack Register!\n");
  } else if( (dReg = regWithIdx ( pic16_dynProcessorRegs, idx,0)) != NULL ) {
    debugLog ("Found a Processor Register!\n");
  } else if( (dReg = regWithIdx ( pic16_dynInternalRegs, idx,0)) != NULL ) {
    debugLog ("Found an Internal Register!\n");
  } else {
    
    debugLog ("Dynamic Register not found\n");


    //fprintf(stderr,"%s %d - requested register: 0x%x\n",__FUNCTION__,__LINE__,idx);
    werror (E_INTERNAL_ERROR, __FILE__, __LINE__,
	    "regWithIdx not found");
    exit (1);

  }

  dReg->wasUsed = 1;
  dReg->isFree = 0;

  return dReg;
}
/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
regs *
pic16_findFreeReg(short type)
{
  //  int i;
  regs* dReg;

  switch (type) {
  case REG_GPR:
    if((dReg = regFindFree(pic16_dynAllocRegs)) != NULL)
      return dReg;
    return addSet(&pic16_dynAllocRegs,newReg(REG_GPR, PO_GPR_TEMP,dynrIdx++,NULL,1,0));

  case REG_STK:

    if((dReg = regFindFree(pic16_dynStackRegs)) != NULL)
      return dReg;

    return NULL;

  case REG_PTR:
  case REG_CND:
  case REG_SFR:
  default:
    return NULL;
  }
}
/*-----------------------------------------------------------------*/
/* freeReg - frees a register                                      */
/*-----------------------------------------------------------------*/
static void
freeReg (regs * reg)
{
  debugLog ("%s\n", __FUNCTION__);
  reg->isFree = 1;
}


/*-----------------------------------------------------------------*/
/* nFreeRegs - returns number of free registers                    */
/*-----------------------------------------------------------------*/
static int
nFreeRegs (int type)
{
  /* dynamically allocate as many as we need and worry about
   * fitting them into a PIC later */

  return 100;
#if 0
  int i;
  int nfr = 0;

  debugLog ("%s\n", __FUNCTION__);
  for (i = 0; i < pic16_nRegs; i++)
    if (regspic16[i].isFree && regspic16[i].type == type)
      nfr++;
  return nfr;
#endif
}

/*-----------------------------------------------------------------*/
/* nfreeRegsType - free registers with type                         */
/*-----------------------------------------------------------------*/
static int
nfreeRegsType (int type)
{
  int nfr;
  debugLog ("%s\n", __FUNCTION__);
  if (type == REG_PTR)
    {
      if ((nfr = nFreeRegs (type)) == 0)
	return nFreeRegs (REG_GPR);
    }

  return nFreeRegs (type);
}
#if 0
static void writeSetUsedRegs(FILE *of, set *dRegs)
{

  regs *dReg;

  for (dReg = setFirstItem(dRegs) ; dReg ; 
       dReg = setNextItem(dRegs)) {

    if(dReg->wasUsed)
      fprintf (of, "\t%s\n",dReg->name);
  }

}
#endif

extern void pic16_assignFixedRegisters(set *regset);
extern void pic16_assignRelocatableRegisters(set *regset,int used);
extern void pic16_dump_map(void);
extern void pic16_dump_cblock(FILE *of);


static void packBits(set *bregs)
{
  set *regset;
  regs *breg;
  regs *bitfield=NULL;
  regs *relocbitfield=NULL;
  int bit_no=0;
  int byte_no=-1;
  char buffer[20];


  for (regset = bregs ; regset ;
       regset = regset->next) {

    breg = regset->item;
    breg->isBitField = 1;
    //fprintf(stderr,"bit reg: %s\n",breg->name);

    if(breg->isFixed) {
      //fprintf(stderr,"packing bit at fixed address = 0x%03x\n",breg->address);

      bitfield = typeRegWithIdx (breg->address >> 3, -1 , 1);
      breg->rIdx = breg->address & 7;
      breg->address >>= 3;

      if(!bitfield) {
	sprintf (buffer, "fbitfield%02x", breg->address);
	//fprintf(stderr,"new bit field\n");
	bitfield = newReg(REG_SFR, PO_GPR_BIT,breg->address,buffer,1,0);
	bitfield->isBitField = 1;
	bitfield->isFixed = 1;
	bitfield->address = breg->address;
	addSet(&pic16_dynDirectRegs,bitfield);
	//hTabAddItem(&dynDirectRegNames, regname2key(buffer), bitfield);
      } else {
	//fprintf(stderr,"  which is occupied by %s (addr = %d)\n",bitfield->name,bitfield->address);
	;
      }
      breg->reg_alias = bitfield;
      bitfield = NULL;

    } else {
      if(!relocbitfield || bit_no >7) {
	byte_no++;
	bit_no=0;
	sprintf (buffer, "bitfield%d", byte_no);
	//fprintf(stderr,"new relocatable bit field\n");
	relocbitfield = newReg(REG_GPR, PO_GPR_BIT,rDirectIdx++,buffer,1,0);
	relocbitfield->isBitField = 1;
	addSet(&pic16_dynDirectRegs,relocbitfield);
	//hTabAddItem(&dynDirectRegNames, regname2key(buffer), relocbitfield);

      }

      breg->reg_alias = relocbitfield;
      breg->address = rDirectIdx;   /* byte_no; */
      breg->rIdx = bit_no++;
    }
  }
      
}



#if 0
static void bitEQUs(FILE *of, set *bregs)
{
  regs *breg,*bytereg;
  int bit_no=0;

  //fprintf(stderr," %s\n",__FUNCTION__);
  for (breg = setFirstItem(bregs) ; breg ;
       breg = setNextItem(bregs)) {

    //fprintf(stderr,"bit reg: %s\n",breg->name);

    bytereg = breg->reg_alias;
    if(bytereg)
      fprintf (of, "%s\tEQU\t( (%s<<3)+%d)\n",
	       breg->name,
	       bytereg->name,
	       breg->rIdx & 0x0007);

    else {
      fprintf(stderr, "bit field is not assigned to a register\n");
      fprintf (of, "%s\tEQU\t( (bitfield%d<<3)+%d)\n",
	       breg->name,
	       bit_no>>3,
	       bit_no & 0x0007);

      bit_no++;
    }
  }
      
}

static void aliasEQUs(FILE *of, set *fregs, int use_rIdx)
{
  regs *reg;


  for (reg = setFirstItem(fregs) ; reg ;
       reg = setNextItem(fregs)) {

    if(!reg->isEmitted && reg->wasUsed) {
      if(use_rIdx) {
        if (reg->type != REG_SFR) {
	  fprintf (of, "%s\tEQU\t0x%03x\n",
		   reg->name,
		   reg->rIdx);
        }
      }
      else
	fprintf (of, "%s\tEQU\t0x%03x\n",
		 reg->name,
		 reg->address);
    }
  }
      
}
#endif

void pic16_writeUsedRegs(FILE *of) 
{
  packBits(pic16_dynDirectBitRegs);


  pic16_assignFixedRegisters(pic16_dynAllocRegs);
  pic16_assignFixedRegisters(pic16_dynStackRegs);
  pic16_assignFixedRegisters(pic16_dynDirectRegs);
  pic16_assignFixedRegisters(pic16_dynProcessorRegs);

  pic16_assignRelocatableRegisters(pic16_dynInternalRegs,0);
  pic16_assignRelocatableRegisters(pic16_dynAllocRegs,0);
  pic16_assignRelocatableRegisters(pic16_dynStackRegs,0);
  pic16_assignRelocatableRegisters(pic16_dynDirectRegs,0);

	//pic16_dump_map();

  pic16_dump_cblock(of);

#if 0
  bitEQUs(of,pic16_dynDirectBitRegs);
  aliasEQUs(of,pic16_dynAllocRegs,0);
  aliasEQUs(of,pic16_dynDirectRegs,0);
  aliasEQUs(of,pic16_dynStackRegs,0);
  aliasEQUs(of,pic16_dynProcessorRegs,1);
#endif

}

#if 0
/*-----------------------------------------------------------------*/
/* allDefsOutOfRange - all definitions are out of a range          */
/*-----------------------------------------------------------------*/
static bool
allDefsOutOfRange (bitVect * defs, int fseq, int toseq)
{
  int i;

  debugLog ("%s\n", __FUNCTION__);
  if (!defs)
    return TRUE;

  for (i = 0; i < defs->size; i++)
    {
      iCode *ic;

      if (bitVectBitValue (defs, i) &&
	  (ic = hTabItemWithKey (iCodehTab, i)) &&
	  (ic->seq >= fseq && ic->seq <= toseq))

	return FALSE;

    }

  return TRUE;
}
#endif

/*-----------------------------------------------------------------*/
/* computeSpillable - given a point find the spillable live ranges */
/*-----------------------------------------------------------------*/
static bitVect *
computeSpillable (iCode * ic)
{
  bitVect *spillable;

  debugLog ("%s\n", __FUNCTION__);
  /* spillable live ranges are those that are live at this 
     point . the following categories need to be subtracted
     from this set. 
     a) - those that are already spilt
     b) - if being used by this one
     c) - defined by this one */

  spillable = bitVectCopy (ic->rlive);
  spillable =
    bitVectCplAnd (spillable, _G.spiltSet);	/* those already spilt */
  spillable =
    bitVectCplAnd (spillable, ic->uses);	/* used in this one */
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
  debugLog ("%s\n", __FUNCTION__);
  return (sym->usl.spillLoc ? 0 : 1);
}

/*-----------------------------------------------------------------*/
/* hasSpilLoc - will return 1 if the symbol has spil location      */
/*-----------------------------------------------------------------*/
static int
hasSpilLoc (symbol * sym, eBBlock * ebp, iCode * ic)
{
  debugLog ("%s\n", __FUNCTION__);
  return (sym->usl.spillLoc ? 1 : 0);
}

/*-----------------------------------------------------------------*/
/* directSpilLoc - will return 1 if the splilocation is in direct  */
/*-----------------------------------------------------------------*/
static int
directSpilLoc (symbol * sym, eBBlock * ebp, iCode * ic)
{
  debugLog ("%s\n", __FUNCTION__);
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
  debugLog ("%s\n", __FUNCTION__);
  return ((sym->usl.spillLoc && !sym->uptr) ? 1 : 0);
}

/*-----------------------------------------------------------------*/
/* rematable - will return 1 if the remat flag is set              */
/*-----------------------------------------------------------------*/
static int
rematable (symbol * sym, eBBlock * ebp, iCode * ic)
{
  debugLog ("%s\n", __FUNCTION__);
  return sym->remat;
}

/*-----------------------------------------------------------------*/
/* notUsedInRemaining - not used or defined in remain of the block */
/*-----------------------------------------------------------------*/
static int
notUsedInRemaining (symbol * sym, eBBlock * ebp, iCode * ic)
{
  debugLog ("%s\n", __FUNCTION__);
  return ((usedInRemaining (operandFromSymbol (sym), ic) ? 0 : 1) &&
	  allDefsOutOfRange (sym->defs, ebp->fSeq, ebp->lSeq));
}

/*-----------------------------------------------------------------*/
/* allLRs - return true for all                                    */
/*-----------------------------------------------------------------*/
static int
allLRs (symbol * sym, eBBlock * ebp, iCode * ic)
{
  debugLog ("%s\n", __FUNCTION__);
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

  debugLog ("%s\n", __FUNCTION__);
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

  debugLog ("%s\n", __FUNCTION__);
  sym = lsym = setFirstItem (sset);

  if (!lsym)
    return NULL;

  for (; lsym; lsym = setNextItem (sset))
    {

      /* if usage is the same then prefer
         the spill the smaller of the two */
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
  debugLog ("%s\n", __FUNCTION__);


  for (sym = setFirstItem (itmpStack); sym;
       sym = setNextItem (itmpStack))
    {
      if (sym->liveTo > fsym->liveFrom)
	return 0;

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

  debugLog ("%s\n", __FUNCTION__);
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
  regs *r0, *r1;
  int k;

  debugLog ("%s\n", __FUNCTION__);
  if (!_G.regAssigned ||
      bitVectIsZero (_G.regAssigned))
    return;

  r0 = pic16_regWithIdx (R0_IDX);
  r1 = pic16_regWithIdx (R1_IDX);

  /* for all live ranges */
  for (lrsym = hTabFirstItem (liveRanges, &k); lrsym;
       lrsym = hTabNextItem (liveRanges, &k))
    {
      int j;

      /* if no registers assigned to it or
         spilt */
      /* if it does not overlap with this then 
         not need to spill it */

      if (lrsym->isspilt || !lrsym->nRegs ||
	  (lrsym->liveTo < forSym->liveFrom))
	continue;

      /* go thru the registers : if it is either
         r0 or r1 then spil it */
      for (j = 0; j < lrsym->nRegs; j++)
	if (lrsym->regs[j] == r0 ||
	    lrsym->regs[j] == r1)
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
  int useXstack, model, noOverlay;

  char slocBuffer[30];
  debugLog ("%s\n", __FUNCTION__);

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
      fprintf (stderr, "kkkInternal error: slocBuffer overflowed: %s:%d\n",
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

  /* we don't allow it to be allocated`
     onto the external stack since : so we
     temporarily turn it off ; we also
     turn off memory model to prevent
     the spil from going to the external storage
     and turn off overlaying 
   */

  useXstack = options.useXstack;
  model = options.model;
  noOverlay = options.noOverlay;
  options.noOverlay = 1;
  options.model = options.useXstack = 0;

  allocLocal (sloc);

  options.useXstack = useXstack;
  options.model = model;
  options.noOverlay = noOverlay;
  sloc->isref = 1;		/* to prevent compiler warning */

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

  debugLog ("%s\n", __FUNCTION__);
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
  debugLog ("%s : %s\n", __FUNCTION__, sym->rname);

  /* if this is rematerializable or has a spillLocation
     we are okay, else we need to create a spillLocation
     for it */
  if (!(sym->remat || sym->usl.spillLoc))
    createStackSpil (sym);


  /* mark it has spilt & put it in the spilt set */
  sym->isspilt = 1;
  _G.spiltSet = bitVectSetBit (_G.spiltSet, sym->key);

  bitVectUnSetBit (_G.regAssigned, sym->key);

  for (i = 0; i < sym->nRegs; i++)

    if (sym->regs[i])
      {
	freeReg (sym->regs[i]);
	sym->regs[i] = NULL;
      }

  /* if spilt on stack then free up r0 & r1 
     if they could have been assigned to some
     LIVE ranges */
  if (!pic16_ptrRegReq && isSpiltOnStack (sym))
    {
      pic16_ptrRegReq++;
      spillLRWithPtrReg (sym);
    }

  if (sym->usl.spillLoc && !sym->remat)
    sym->usl.spillLoc->allocreq = 1;
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

  debugLog ("%s\n", __FUNCTION__);
  /* get the spillable live ranges */
  lrcs = computeSpillable (ic);

  /* get all live ranges that are rematerizable */
  if ((selectS = liveRangesWith (lrcs, rematable, ebp, ic)))
    {

      /* return the least used of these */
      return leastUsedLR (selectS);
    }

  /* get live ranges with spillLocations in direct space */
  if ((selectS = liveRangesWith (lrcs, directSpilLoc, ebp, ic)))
    {
      sym = leastUsedLR (selectS);
      strcpy (sym->rname, (sym->usl.spillLoc->rname[0] ?
			   sym->usl.spillLoc->rname :
			   sym->usl.spillLoc->name));
      sym->spildir = 1;
      /* mark it as allocation required */
      sym->usl.spillLoc->allocreq = 1;
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

      /* check if there are any live ranges that not
         used in the remainder of the block */
      if (!_G.blockSpil && (selectS = liveRangesWith (lrcs, notUsedInRemaining, ebp, ic)))
	{
	  sym = leastUsedLR (selectS);
	  if (!sym->remat)
	    {
	      sym->remainSpil = 1;
	      _G.blockSpil++;
	    }
	  return sym;
	}
    }

  /* find live ranges with spillocation && not used as pointers */
  if ((selectS = liveRangesWith (lrcs, hasSpilLocnoUptr, ebp, ic)))
    {

      sym = leastUsedLR (selectS);
      /* mark this as allocation required */
      sym->usl.spillLoc->allocreq = 1;
      return sym;
    }

  /* find live ranges with spillocation */
  if ((selectS = liveRangesWith (lrcs, hasSpilLoc, ebp, ic)))
    {

      sym = leastUsedLR (selectS);
      sym->usl.spillLoc->allocreq = 1;
      return sym;
    }

  /* couldn't find then we need to create a spil
     location on the stack , for which one? the least
     used ofcourse */
  if ((selectS = liveRangesWith (lrcs, noSpilLoc, ebp, ic)))
    {

      /* return a created spil location */
      sym = createStackSpil (leastUsedLR (selectS));
      sym->usl.spillLoc->allocreq = 1;
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

  debugLog ("%s\n", __FUNCTION__);
  /* get something we can spil */
  ssym = selectSpil (ic, ebp, forSym);

  /* mark it as spilt */
  ssym->isspilt = 1;
  _G.spiltSet = bitVectSetBit (_G.spiltSet, ssym->key);

  /* mark it as not register assigned &
     take it away from the set */
  bitVectUnSetBit (_G.regAssigned, ssym->key);

  /* mark the registers as free */
  for (i = 0; i < ssym->nRegs; i++)
    if (ssym->regs[i])
      freeReg (ssym->regs[i]);

  /* if spilt on stack then free up r0 & r1 
     if they could have been assigned to as gprs */
  if (!pic16_ptrRegReq && isSpiltOnStack (ssym))
    {
      pic16_ptrRegReq++;
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
static regs *
getRegPtr (iCode * ic, eBBlock * ebp, symbol * sym)
{
  regs *reg;

  debugLog ("%s\n", __FUNCTION__);
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
static regs *
getRegGpr (iCode * ic, eBBlock * ebp, symbol * sym)
{
  regs *reg;

  debugLog ("%s\n", __FUNCTION__);
tryAgain:
  /* try for gpr type */
  if ((reg = allocReg (REG_GPR)))
    return reg;

  if (!pic16_ptrRegReq)
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
/* symHasReg - symbol has a given register                         */
/*-----------------------------------------------------------------*/
static bool
symHasReg (symbol * sym, regs * reg)
{
  int i;

  debugLog ("%s\n", __FUNCTION__);
  for (i = 0; i < sym->nRegs; i++)
    if (sym->regs[i] == reg)
      return TRUE;

  return FALSE;
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

  debugLog ("%s\n", __FUNCTION__);
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
	      !(SKIP_IC2 (ic) ||	/* not a special icode */
		ic->op == JUMPTABLE ||
		ic->op == IFX ||
		ic->op == IPUSH ||
		ic->op == IPOP ||
		ic->op == RETURN ||
		POINTER_SET (ic)) &&
	      (result = OP_SYMBOL (IC_RESULT (ic))) &&	/* has a result */
	      result->liveTo > ic->seq &&	/* and will live beyond this */
	      result->liveTo <= ebp->lSeq &&	/* does not go beyond this block */
	      result->regType == sym->regType &&	/* same register types */
	      result->nRegs &&	/* which needs registers */
	      !result->isspilt &&	/* and does not already have them */
	      !result->remat &&
	      !bitVectBitValue (_G.regAssigned, result->key) &&
	  /* the number of free regs + number of regs in this LR
	     can accomodate the what result Needs */
	      ((nfreeRegsType (result->regType) +
		sym->nRegs) >= result->nRegs)
	    )
	    {

	      for (i = 0; i < max (sym->nRegs, result->nRegs); i++)
		if (i < sym->nRegs)
		  result->regs[i] = sym->regs[i];
		else
		  result->regs[i] = getRegGpr (ic, ebp, result);

	      _G.regAssigned = bitVectSetBit (_G.regAssigned, result->key);

	    }

	  /* free the remaining */
	  for (; i < sym->nRegs; i++)
	    {
	      if (psym)
		{
		  if (!symHasReg (psym, sym->regs[i]))
		    freeReg (sym->regs[i]);
		}
	      else
		freeReg (sym->regs[i]);
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

  debugLog ("%s\n", __FUNCTION__);
  /* not spilt any more */
  sym->isspilt = sym->blockSpil = sym->remainSpil = 0;
  bitVectUnSetBit (_G.spiltSet, sym->key);

  _G.regAssigned = bitVectSetBit (_G.regAssigned, sym->key);

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
  debugLog ("%s\n", __FUNCTION__);
  /* first check if there are any avlb registers
     of te type required */
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
      if (pic16_ptrRegReq)
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

  debugLog (" ... yep it will (cause a spill)\n");
  /* it will cause a spil */
  return 1;
}

/*-----------------------------------------------------------------*/
/* positionRegs - the allocator can allocate same registers to res- */
/* ult and operand, if this happens make sure they are in the same */
/* position as the operand otherwise chaos results                 */
/*-----------------------------------------------------------------*/
static void
positionRegs (symbol * result, symbol * opsym, int lineno)
{
  int count = min (result->nRegs, opsym->nRegs);
  int i, j = 0, shared = 0;

  debugLog ("%s\n", __FUNCTION__);
  /* if the result has been spilt then cannot share */
  if (opsym->isspilt)
    return;
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
      regs *tmp = result->regs[i];
      result->regs[i] = result->regs[j];
      result->regs[j] = tmp;
      goto again;
    }
}

/*-----------------------------------------------------------------*/
/* serialRegAssign - serially allocate registers to the variables  */
/*-----------------------------------------------------------------*/
static void
serialRegAssign (eBBlock ** ebbs, int count)
{
  int i;

  debugLog ("%s\n", __FUNCTION__);
  /* for all blocks */
  for (i = 0; i < count; i++)
    {

      iCode *ic;

      if (ebbs[i]->noPath &&
	  (ebbs[i]->entryLabel != entryLabel &&
	   ebbs[i]->entryLabel != returnLabel))
	continue;

      /* of all instructions do */
      for (ic = ebbs[i]->sch; ic; ic = ic->next)
	{

	  debugLog ("  op: %s\n", decodeOp (ic->op));

	  /* if this is an ipop that means some live
	     range will have to be assigned again */
	  if (ic->op == IPOP)
	    reassignLR (IC_LEFT (ic));

	  /* if result is present && is a true symbol */
	  if (IC_RESULT (ic) && ic->op != IFX &&
	      IS_TRUE_SYMOP (IC_RESULT (ic)))
	    OP_SYMBOL (IC_RESULT (ic))->allocreq = 1;

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
	    continue;

	  /* now we need to allocate registers
	     only for the result */
	  if (IC_RESULT (ic))
	    {
	      symbol *sym = OP_SYMBOL (IC_RESULT (ic));
	      bitVect *spillable;
	      int willCS;
	      int j;
	      int ptrRegSet = 0;

	      /* if it does not need or is spilt 
	         or is already assigned to registers
	         or will not live beyond this instructions */
	      if (!sym->nRegs ||
		  sym->isspilt ||
		  bitVectBitValue (_G.regAssigned, sym->key) ||
		  sym->liveTo <= ic->seq)
		continue;

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
	      if (sym->remat ||
		  (willCS && bitVectIsZero (spillable)))
		{

		  spillThis (sym);
		  continue;

		}

	      /* if it has a spillocation & is used less than
	         all other live ranges then spill this */
		if (willCS) {
		    if (sym->usl.spillLoc) {
			symbol *leastUsed = leastUsedLR (liveRangesWith (spillable,
									 allLRs, ebbs[i], ic));
			if (leastUsed && leastUsed->used > sym->used) {
			    spillThis (sym);
			    continue;
			}
		    } else {
			/* if none of the liveRanges have a spillLocation then better
			   to spill this one than anything else already assigned to registers */
			if (liveRangesWith(spillable,noSpilLoc,ebbs[i],ic)) {
			    /* if this is local to this block then we might find a block spil */
			    if (!(sym->liveFrom >= ebbs[i]->fSeq && sym->liveTo <= ebbs[i]->lSeq)) {
				spillThis (sym);
				continue;
			    }
			}
		    }
		}

	      if (ic->op == RECEIVE)
		debugLog ("When I get clever, I'll optimize the receive logic\n");

	      /* if we need ptr regs for the right side
	         then mark it */
	      if (POINTER_GET (ic) && getSize (OP_SYMBOL (IC_LEFT (ic))->type)
		  <= (unsigned) PTRSIZE)
		{
		  pic16_ptrRegReq++;
		  ptrRegSet = 1;
		}
	      /* else we assign registers to it */
	      _G.regAssigned = bitVectSetBit (_G.regAssigned, sym->key);

	      debugLog ("  %d - \n", __LINE__);
	      if(debugF) 
		bitVectDebugOn(_G.regAssigned, debugF);

	      for (j = 0; j < sym->nRegs; j++)
		{
		  if (sym->regType == REG_PTR)
		    sym->regs[j] = getRegPtr (ic, ebbs[i], sym);
		  else
		    sym->regs[j] = getRegGpr (ic, ebbs[i], sym);

		  /* if the allocation falied which means
		     this was spilt then break */
		  if (!sym->regs[j])
		    break;
		}
	      debugLog ("  %d - \n", __LINE__);

	      /* if it shares registers with operands make sure
	         that they are in the same position */
	      if (IC_LEFT (ic) && IS_SYMOP (IC_LEFT (ic)) &&
		  OP_SYMBOL (IC_LEFT (ic))->nRegs && ic->op != '=')
		positionRegs (OP_SYMBOL (IC_RESULT (ic)),
			      OP_SYMBOL (IC_LEFT (ic)), ic->lineno);
	      /* do the same for the right operand */
	      if (IC_RIGHT (ic) && IS_SYMOP (IC_RIGHT (ic)) &&
		  OP_SYMBOL (IC_RIGHT (ic))->nRegs && ic->op != '=')
		positionRegs (OP_SYMBOL (IC_RESULT (ic)),
			      OP_SYMBOL (IC_RIGHT (ic)), ic->lineno);

	      debugLog ("  %d - \n", __LINE__);
	      if (ptrRegSet)
		{
		  debugLog ("  %d - \n", __LINE__);
		  pic16_ptrRegReq--;
		  ptrRegSet = 0;
		}

	    }
	}
    }
}

/*-----------------------------------------------------------------*/
/* rUmaskForOp :- returns register mask for an operand             */
/*-----------------------------------------------------------------*/
static bitVect *
rUmaskForOp (operand * op)
{
  bitVect *rumask;
  symbol *sym;
  int j;

  debugLog ("%s\n", __FUNCTION__);
  /* only temporaries are assigned registers */
  if (!IS_ITEMP (op))
    return NULL;

  sym = OP_SYMBOL (op);

  /* if spilt or no registers assigned to it
     then nothing */
  if (sym->isspilt || !sym->nRegs)
    return NULL;

  rumask = newBitVect (pic16_nRegs);

  for (j = 0; j < sym->nRegs; j++)
    {
      rumask = bitVectSetBit (rumask,
			      sym->regs[j]->rIdx);
    }

  return rumask;
}

/*-----------------------------------------------------------------*/
/* regsUsedIniCode :- returns bit vector of registers used in iCode */
/*-----------------------------------------------------------------*/
static bitVect *
regsUsedIniCode (iCode * ic)
{
  bitVect *rmask = newBitVect (pic16_nRegs);

  debugLog ("%s\n", __FUNCTION__);
  /* do the special cases first */
  if (ic->op == IFX)
    {
      rmask = bitVectUnion (rmask,
			    rUmaskForOp (IC_COND (ic)));
      goto ret;
    }

  /* for the jumptable */
  if (ic->op == JUMPTABLE)
    {
      rmask = bitVectUnion (rmask,
			    rUmaskForOp (IC_JTCOND (ic)));

      goto ret;
    }

  /* of all other cases */
  if (IC_LEFT (ic))
    rmask = bitVectUnion (rmask,
			  rUmaskForOp (IC_LEFT (ic)));


  if (IC_RIGHT (ic))
    rmask = bitVectUnion (rmask,
			  rUmaskForOp (IC_RIGHT (ic)));

  if (IC_RESULT (ic))
    rmask = bitVectUnion (rmask,
			  rUmaskForOp (IC_RESULT (ic)));

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

  debugLog ("%s\n", __FUNCTION__);
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
	  ic->rMask = newBitVect (pic16_nRegs + 1);

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
		  exit (0);
		}

	      /* if no register assigned to it */
	      if (!sym->nRegs || sym->isspilt)
		continue;

	      /* for all the registers allocated to it */
	      for (k = 0; k < sym->nRegs; k++)
		if (sym->regs[k])
		  ic->rMask =
		    bitVectSetBit (ic->rMask, sym->regs[k]->rIdx);
	    }
	}
    }
}

/*-----------------------------------------------------------------*/
/* rematStr - returns the rematerialized string for a remat var    */
/*-----------------------------------------------------------------*/
static symbol *
rematStr (symbol * sym)
{
  char *s = buffer;
  iCode *ic = sym->rematiCode;
  symbol *psym = NULL;

  debugLog ("%s\n", __FUNCTION__);

  //printf ("%s\n", s);

  /* if plus or minus print the right hand side */

  if (ic->op == '+' || ic->op == '-') {
	
    iCode *ric = OP_SYMBOL (IC_LEFT (ic))->rematiCode;

    sprintf (s, "(%s %c 0x%04x)",
	     OP_SYMBOL (IC_LEFT (ric))->rname,
	     ic->op,
	     (int) operandLitValue (IC_RIGHT (ic)));


    //fprintf(stderr, "ralloc.c:%d OOPS %s\n",__LINE__,s);

    psym = newSymbol (OP_SYMBOL (IC_LEFT (ric))->rname, 1);
    psym->offset = (int) operandLitValue (IC_RIGHT (ic));

    return psym;
  }

  sprintf (s, "%s", OP_SYMBOL (IC_LEFT (ic))->rname);
  psym = newSymbol (OP_SYMBOL (IC_LEFT (ic))->rname, 1);

  //printf ("ralloc.c:%d %s\n", __LINE__,buffer);
  return psym;
}

#if 0
/*-----------------------------------------------------------------*/
/* rematStr - returns the rematerialized string for a remat var    */
/*-----------------------------------------------------------------*/
static char *
rematStr (symbol * sym)
{
  char *s = buffer;
  iCode *ic = sym->rematiCode;

  debugLog ("%s\n", __FUNCTION__);
  while (1)
    {

      printf ("%s\n", s);
      /* if plus or minus print the right hand side */
/*
   if (ic->op == '+' || ic->op == '-') {
   sprintf(s,"0x%04x %c ",(int) operandLitValue(IC_RIGHT(ic)),
   ic->op );
   s += strlen(s);
   ic = OP_SYMBOL(IC_LEFT(ic))->rematiCode;
   continue ;
   }
 */
      if (ic->op == '+' || ic->op == '-')
	{
	  iCode *ric = OP_SYMBOL (IC_LEFT (ic))->rematiCode;
	  sprintf (s, "(%s %c 0x%04x)",
		   OP_SYMBOL (IC_LEFT (ric))->rname,
		   ic->op,
		   (int) operandLitValue (IC_RIGHT (ic)));

	  //s += strlen(s);
	  //ic = OP_SYMBOL(IC_LEFT(ic))->rematiCode;
	  //continue ;
	  //fprintf(stderr, "ralloc.c:%d OOPS %s\n",__LINE__,s);
	  return buffer;
	}

      /* we reached the end */
      sprintf (s, "%s", OP_SYMBOL (IC_LEFT (ic))->rname);
      break;
    }

  printf ("%s\n", buffer);
  return buffer;
}
#endif

/*-----------------------------------------------------------------*/
/* regTypeNum - computes the type & number of registers required   */
/*-----------------------------------------------------------------*/
static void
regTypeNum ()
{
  symbol *sym;
  int k;
  iCode *ic;

  debugLog ("%s\n", __FUNCTION__);
  /* for each live range do */
  for (sym = hTabFirstItem (liveRanges, &k); sym;
       sym = hTabNextItem (liveRanges, &k)) {

    debugLog ("  %d - %s\n", __LINE__, sym->rname);

    /* if used zero times then no registers needed */
    if ((sym->liveTo - sym->liveFrom) == 0)
      continue;


    /* if the live range is a temporary */
    if (sym->isitmp) {

      debugLog ("  %d - itemp register\n", __LINE__);

      /* if the type is marked as a conditional */
      if (sym->regType == REG_CND)
	continue;

      /* if used in return only then we don't 
	 need registers */
      if (sym->ruonly || sym->accuse) {
	if (IS_AGGREGATE (sym->type) || sym->isptr)
	  sym->type = aggrToPtr (sym->type, FALSE);
	debugLog ("  %d - no reg needed - used as a return\n", __LINE__);

	continue;
      }

      /* if the symbol has only one definition &
	 that definition is a get_pointer and the
	 pointer we are getting is rematerializable and
	 in "data" space */

      if (bitVectnBitsOn (sym->defs) == 1 &&
	  (ic = hTabItemWithKey (iCodehTab,
				 bitVectFirstBit (sym->defs))) &&
	  POINTER_GET (ic) &&
	  !sym->noSpilLoc &&
	  !IS_BITVAR (sym->etype)) {
	

	debugLog ("  %d - \n", __LINE__);

	/* if remat in data space */
	if (OP_SYMBOL (IC_LEFT (ic))->remat &&
	    DCL_TYPE (aggrToPtr (sym->type, FALSE)) == POINTER) {

	  /* create a psuedo symbol & force a spil */
	  //X symbol *psym = newSymbol (rematStr (OP_SYMBOL (IC_LEFT (ic))), 1);
	  symbol *psym = rematStr (OP_SYMBOL (IC_LEFT (ic)));
	  psym->type = sym->type;
	  psym->etype = sym->etype;
	  strcpy (psym->rname, psym->name);
	  sym->isspilt = 1;
	  sym->usl.spillLoc = psym;
	  continue;
	}

	/* if in data space or idata space then try to
	   allocate pointer register */

      }

      /* if not then we require registers */
      sym->nRegs = ((IS_AGGREGATE (sym->type) || sym->isptr) ?
		    getSize (sym->type = aggrToPtr (sym->type, FALSE)) :
		    getSize (sym->type));


#if 0
    if(IS_PTR_CONST (sym->type)) {
#else
    if(IS_CODEPTR (sym->type)) {
#endif
      debugLog ("  %d const pointer type requires %d registers, changing to 2\n",__LINE__,sym->nRegs);
      sym->nRegs = 2;
    }

      if (sym->nRegs > 4) {
	fprintf (stderr, "allocated more than 4 or 0 registers for type ");
	printTypeChain (sym->type, stderr);
	fprintf (stderr, "\n");
      }

      /* determine the type of register required */
      if (sym->nRegs == 1 &&
	  IS_PTR (sym->type) &&
	  sym->uptr)
	sym->regType = REG_PTR;
      else
	sym->regType = REG_GPR;


      debugLog ("  reg name %s,  reg type %s\n", sym->rname, debugLogRegType (sym->regType));

    }
    else
      /* for the first run we don't provide */
      /* registers for true symbols we will */
      /* see how things go                  */
      sym->nRegs = 0;
  }

}
static DEFSETFUNC (markRegFree)
{
  ((regs *)item)->isFree = 1;

  return 0;
}

DEFSETFUNC (pic16_deallocReg)
{
  fprintf(stderr,"deallocting register %s\n",((regs *)item)->name);
  ((regs *)item)->isFree = 1;
  ((regs *)item)->wasUsed = 0;

  return 0;
}
/*-----------------------------------------------------------------*/
/* freeAllRegs - mark all registers as free                        */
/*-----------------------------------------------------------------*/
void
pic16_freeAllRegs ()
{
  //  int i;

  debugLog ("%s\n", __FUNCTION__);

  applyToSet(pic16_dynAllocRegs,markRegFree);
  applyToSet(pic16_dynStackRegs,markRegFree);

/*
  for (i = 0; i < pic16_nRegs; i++)
    regspic16[i].isFree = 1;
*/
}

/*-----------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
void
pic16_deallocateAllRegs ()
{
  //  int i;

  debugLog ("%s\n", __FUNCTION__);

  applyToSet(pic16_dynAllocRegs,pic16_deallocReg);

/*
  for (i = 0; i < pic16_nRegs; i++) {
    if(regspic16[i].pc_type == PO_GPR_TEMP) {
      regspic16[i].isFree = 1;
      regspic16[i].wasUsed = 0;
    }
  }
*/
}


/*-----------------------------------------------------------------*/
/* deallocStackSpil - this will set the stack pointer back         */
/*-----------------------------------------------------------------*/
static
DEFSETFUNC (deallocStackSpil)
{
  symbol *sym = item;

  debugLog ("%s\n", __FUNCTION__);
  deallocLocal (sym);
  return 0;
}

/*-----------------------------------------------------------------*/
/* farSpacePackable - returns the packable icode for far variables */
/*-----------------------------------------------------------------*/
static iCode *
farSpacePackable (iCode * ic)
{
  iCode *dic;

  debugLog ("%s\n", __FUNCTION__);
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

      /* if pointer get and size > 1 */
      if (POINTER_GET (dic) &&
	  getSize (aggrToPtr (operandType (IC_LEFT (dic)), FALSE)) > 1)
	return NULL;

      if (POINTER_SET (dic) &&
	  getSize (aggrToPtr (operandType (IC_RESULT (dic)), FALSE)) > 1)
	return NULL;

      /* if any three is a true symbol in far space */
      if (IC_RESULT (dic) &&
	  IS_TRUE_SYMOP (IC_RESULT (dic)) &&
	  isOperandInFarSpace (IC_RESULT (dic)))
	return NULL;

      if (IC_RIGHT (dic) &&
	  IS_TRUE_SYMOP (IC_RIGHT (dic)) &&
	  isOperandInFarSpace (IC_RIGHT (dic)) &&
	  !isOperandEqual (IC_RIGHT (dic), IC_RESULT (ic)))
	return NULL;

      if (IC_LEFT (dic) &&
	  IS_TRUE_SYMOP (IC_LEFT (dic)) &&
	  isOperandInFarSpace (IC_LEFT (dic)) &&
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

/*-----------------------------------------------------------------*/
/* packRegsForAssign - register reduction for assignment           */
/*-----------------------------------------------------------------*/
static int
packRegsForAssign (iCode * ic, eBBlock * ebp)
{

  iCode *dic, *sic;

  debugLog ("%s\n", __FUNCTION__);

  debugAopGet ("  result:", IC_RESULT (ic));
  debugAopGet ("  left:", IC_LEFT (ic));
  debugAopGet ("  right:", IC_RIGHT (ic));

  /* if this is at an absolute address, then get the address. */
  if (SPEC_ABSA ( OP_SYM_ETYPE(IC_RESULT(ic))) ) {
    if(IS_CONFIG_ADDRESS( SPEC_ADDR ( OP_SYM_ETYPE(IC_RESULT(ic))))) {
      debugLog ("  %d - found config word declaration\n", __LINE__);
      if(IS_VALOP(IC_RIGHT(ic))) {
	debugLog ("  setting config word to %x\n", 
		  (int) floatFromVal (IC_RIGHT(ic)->operand.valOperand));
	pic16_assignConfigWordValue(  SPEC_ADDR ( OP_SYM_ETYPE(IC_RESULT(ic))),
				(int) floatFromVal (IC_RIGHT(ic)->operand.valOperand));
      }

      /* remove the assignment from the iCode chain. */

      remiCodeFromeBBlock (ebp, ic);
      bitVectUnSetBit(OP_SYMBOL(IC_RESULT(ic))->defs,ic->key);
      hTabDeleteItem (&iCodehTab, ic->key, ic, DELETE_ITEM, NULL);

      return 1;

    }
  }

  if (!IS_ITEMP (IC_RESULT (ic))) {
    pic16_allocDirReg(IC_RESULT (ic));
    debugLog ("  %d - result is not temp\n", __LINE__);
  }
/*
  if (IC_LEFT (ic) && !IS_ITEMP (IC_LEFT (ic))) {
    debugLog ("  %d - left is not temp, allocating\n", __LINE__);
    pic16_allocDirReg(IC_LEFT (ic));
  }
*/

  if (!IS_ITEMP (IC_RIGHT (ic))) {
    debugLog ("  %d - not packing - right is not temp\n", __LINE__);
    pic16_allocDirReg(IC_RIGHT (ic));
    return 0;
  }

  if (OP_SYMBOL (IC_RIGHT (ic))->isind ||
      OP_LIVETO (IC_RIGHT (ic)) > ic->seq)
    {
      debugLog ("  %d - not packing - right side fails \n", __LINE__);
      return 0;
    }

  /* if the true symbol is defined in far space or on stack
     then we should not since this will increase register pressure */
  if (isOperandInFarSpace (IC_RESULT (ic)))
    {
      if ((dic = farSpacePackable (ic)))
	goto pack;
      else
	return 0;

    }
  /* find the definition of iTempNN scanning backwards if we find a 
     a use of the true symbol before we find the definition then 
     we cannot pack */
  for (dic = ic->prev; dic; dic = dic->prev)
    {

      /* if there is a function call and this is
         a parameter & not my parameter then don't pack it */
      if ((dic->op == CALL || dic->op == PCALL) &&
	  (OP_SYMBOL (IC_RESULT (ic))->_isparm &&
	   !OP_SYMBOL (IC_RESULT (ic))->ismyparm))
	{
	  debugLog ("  %d - \n", __LINE__);
	  dic = NULL;
	  break;
	}

      if (SKIP_IC2 (dic))
	continue;

      if (IS_TRUE_SYMOP (IC_RESULT (dic)) &&
	  IS_OP_VOLATILE (IC_RESULT (dic)))
	{
	  debugLog ("  %d - dic is VOLATILE \n", __LINE__);
	  dic = NULL;
	  break;
	}

      if (IS_SYMOP (IC_RESULT (dic)) &&
	  IC_RESULT (dic)->key == IC_RIGHT (ic)->key)
	{
	  /* A previous result was assigned to the same register - we'll our definition */
	  debugLog ("  %d - dic result key == ic right key -- pointer set=%c\n",
		    __LINE__, ((POINTER_SET (dic)) ? 'Y' : 'N'));
	  if (POINTER_SET (dic))
	    dic = NULL;

	  break;
	}

      if (IS_SYMOP (IC_RIGHT (dic)) &&
	  (IC_RIGHT (dic)->key == IC_RESULT (ic)->key ||
	   IC_RIGHT (dic)->key == IC_RIGHT (ic)->key))
	{
	  debugLog ("  %d - dic right key == ic rightor result key\n", __LINE__);
	  dic = NULL;
	  break;
	}

      if (IS_SYMOP (IC_LEFT (dic)) &&
	  (IC_LEFT (dic)->key == IC_RESULT (ic)->key ||
	   IC_LEFT (dic)->key == IC_RIGHT (ic)->key))
	{
	  debugLog ("  %d - dic left key == ic rightor result key\n", __LINE__);
	  dic = NULL;
	  break;
	}

      if (POINTER_SET (dic) &&
	  IC_RESULT (dic)->key == IC_RESULT (ic)->key)
	{
	  debugLog ("  %d - dic result key == ic result key -- pointer set=Y\n",
		    __LINE__);
	  dic = NULL;
	  break;
	}
    }

  if (!dic)
    return 0;			/* did not find */

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
  debugLog ("  packing. removing %s\n", OP_SYMBOL (IC_RIGHT (ic))->rname);
  debugLog ("  replacing with %s\n", OP_SYMBOL (IC_RESULT (dic))->rname);
  /* found the definition */
  /* replace the result with the result of */
  /* this assignment and remove this assignment */
  bitVectUnSetBit(OP_SYMBOL(IC_RESULT(dic))->defs,dic->key);
  IC_RESULT (dic) = IC_RESULT (ic);

  if (IS_ITEMP (IC_RESULT (dic)) && OP_SYMBOL (IC_RESULT (dic))->liveFrom > dic->seq)
    {
      OP_SYMBOL (IC_RESULT (dic))->liveFrom = dic->seq;
    }
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
  bitVectUnSetBit(OP_SYMBOL(IC_RESULT(ic))->defs,ic->key);
  hTabDeleteItem (&iCodehTab, ic->key, ic, DELETE_ITEM, NULL);
  OP_DEFS (IC_RESULT (dic)) = bitVectSetBit (OP_DEFS (IC_RESULT (dic)), dic->key);
  return 1;


}

/*-----------------------------------------------------------------*/
/* findAssignToSym : scanning backwards looks for first assig found */
/*-----------------------------------------------------------------*/
static iCode *
findAssignToSym (operand * op, iCode * ic)
{
  iCode *dic;

  debugLog ("%s\n", __FUNCTION__);
  for (dic = ic->prev; dic; dic = dic->prev)
    {

      /* if definition by assignment */
      if (dic->op == '=' &&
	  !POINTER_SET (dic) &&
	  IC_RESULT (dic)->key == op->key
/*          &&  IS_TRUE_SYMOP(IC_RIGHT(dic)) */
	)
	{

	  /* we are interested only if defined in far space */
	  /* or in stack space in case of + & - */

	  /* if assigned to a non-symbol then return
	     true */
	  if (!IS_SYMOP (IC_RIGHT (dic)))
	    break;

	  /* if the symbol is in far space then
	     we should not */
	  if (isOperandInFarSpace (IC_RIGHT (dic)))
	    return NULL;

	  /* for + & - operations make sure that
	     if it is on the stack it is the same
	     as one of the three operands */
	  if ((ic->op == '+' || ic->op == '-') &&
	      OP_SYMBOL (IC_RIGHT (dic))->onStack)
	    {

	      if (IC_RESULT (ic)->key != IC_RIGHT (dic)->key &&
		  IC_LEFT (ic)->key != IC_RIGHT (dic)->key &&
		  IC_RIGHT (ic)->key != IC_RIGHT (dic)->key)
		return NULL;
	    }

	  break;

	}

      /* if we find an usage then we cannot delete it */
      if (IC_LEFT (dic) && IC_LEFT (dic)->key == op->key)
	return NULL;

      if (IC_RIGHT (dic) && IC_RIGHT (dic)->key == op->key)
	return NULL;

      if (POINTER_SET (dic) && IC_RESULT (dic)->key == op->key)
	return NULL;
    }

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
/* packRegsForSupport :- reduce some registers for support calls   */
/*-----------------------------------------------------------------*/
static int
packRegsForSupport (iCode * ic, eBBlock * ebp)
{
  int change = 0;

  debugLog ("%s\n", __FUNCTION__);
  /* for the left & right operand :- look to see if the
     left was assigned a true symbol in far space in that
     case replace them */
  if (IS_ITEMP (IC_LEFT (ic)) &&
      OP_SYMBOL (IC_LEFT (ic))->liveTo <= ic->seq)
    {
      iCode *dic = findAssignToSym (IC_LEFT (ic), ic);
      iCode *sic;

      if (!dic)
	goto right;

      debugAopGet ("removing left:", IC_LEFT (ic));

      /* found it we need to remove it from the
         block */
      for (sic = dic; sic != ic; sic = sic->next)
	bitVectUnSetBit (sic->rlive, IC_LEFT (ic)->key);

      IC_LEFT (ic)->operand.symOperand =
	IC_RIGHT (dic)->operand.symOperand;
      IC_LEFT (ic)->key = IC_RIGHT (dic)->operand.symOperand->key;
      remiCodeFromeBBlock (ebp, dic);
      bitVectUnSetBit(OP_SYMBOL(IC_RESULT(dic))->defs,dic->key);
      hTabDeleteItem (&iCodehTab, dic->key, dic, DELETE_ITEM, NULL);
      change++;
    }

  /* do the same for the right operand */
right:
  if (!change &&
      IS_ITEMP (IC_RIGHT (ic)) &&
      OP_SYMBOL (IC_RIGHT (ic))->liveTo <= ic->seq)
    {
      iCode *dic = findAssignToSym (IC_RIGHT (ic), ic);
      iCode *sic;

      if (!dic)
	return change;

      /* if this is a subtraction & the result
         is a true symbol in far space then don't pack */
      if (ic->op == '-' && IS_TRUE_SYMOP (IC_RESULT (dic)))
	{
	  sym_link *etype = getSpec (operandType (IC_RESULT (dic)));
	  if (IN_FARSPACE (SPEC_OCLS (etype)))
	    return change;
	}

      debugAopGet ("removing right:", IC_RIGHT (ic));

      /* found it we need to remove it from the
         block */
      for (sic = dic; sic != ic; sic = sic->next)
	bitVectUnSetBit (sic->rlive, IC_RIGHT (ic)->key);

      IC_RIGHT (ic)->operand.symOperand =
	IC_RIGHT (dic)->operand.symOperand;
      IC_RIGHT (ic)->key = IC_RIGHT (dic)->operand.symOperand->key;

      remiCodeFromeBBlock (ebp, dic);
      bitVectUnSetBit(OP_SYMBOL(IC_RESULT(dic))->defs,dic->key);
      hTabDeleteItem (&iCodehTab, dic->key, dic, DELETE_ITEM, NULL);
      change++;
    }

  return change;
}

#define IS_OP_RUONLY(x) (x && IS_SYMOP(x) && OP_SYMBOL(x)->ruonly)


/*-----------------------------------------------------------------*/
/* packRegsForOneuse : - will reduce some registers for single Use */
/*-----------------------------------------------------------------*/
static iCode *
packRegsForOneuse (iCode * ic, operand * op, eBBlock * ebp)
{
  bitVect *uses;
  iCode *dic, *sic;

  debugLog ("%s\n", __FUNCTION__);
  /* if returning a literal then do nothing */
  if (!IS_SYMOP (op))
    return NULL;

  /* only upto 2 bytes since we cannot predict
     the usage of b, & acc */
  if (getSize (operandType (op)) > (pic16_fReturnSizePic - 2) &&
      ic->op != RETURN &&
      ic->op != SEND)
    return NULL;

  /* this routine will mark the a symbol as used in one 
     instruction use only && if the definition is local 
     (ie. within the basic block) && has only one definition &&
     that definition is either a return value from a 
     function or does not contain any variables in
     far space */
  uses = bitVectCopy (OP_USES (op));
  bitVectUnSetBit (uses, ic->key);	/* take away this iCode */
  if (!bitVectIsZero (uses))	/* has other uses */
    return NULL;

  /* if it has only one defintion */
  if (bitVectnBitsOn (OP_DEFS (op)) > 1)
    return NULL;		/* has more than one definition */

  /* get that definition */
  if (!(dic =
	hTabItemWithKey (iCodehTab,
			 bitVectFirstBit (OP_DEFS (op)))))
    return NULL;

  /* found the definition now check if it is local */
  if (dic->seq < ebp->fSeq ||
      dic->seq > ebp->lSeq)
    return NULL;		/* non-local */

  /* now check if it is the return from
     a function call */
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


  /* otherwise check that the definition does
     not contain any symbols in far space */
  if (isOperandInFarSpace (IC_LEFT (dic)) ||
      isOperandInFarSpace (IC_RIGHT (dic)) ||
      IS_OP_RUONLY (IC_LEFT (ic)) ||
      IS_OP_RUONLY (IC_RIGHT (ic)))
    {
      return NULL;
    }

  /* if pointer set then make sure the pointer
     is one byte */
  if (POINTER_SET (dic) &&
      !IS_DATA_PTR (aggrToPtr (operandType (IC_RESULT (dic)), FALSE)))
    return NULL;

  if (POINTER_GET (dic) &&
      !IS_DATA_PTR (aggrToPtr (operandType (IC_LEFT (dic)), FALSE)))
    return NULL;

  sic = dic;

  /* also make sure the intervenening instructions
     don't have any thing in far space */
  for (dic = dic->next; dic && dic != ic; dic = dic->next)
    {

      /* if there is an intervening function call then no */
      if (dic->op == CALL || dic->op == PCALL)
	return NULL;
      /* if pointer set then make sure the pointer
         is one byte */
      if (POINTER_SET (dic) &&
	  !IS_DATA_PTR (aggrToPtr (operandType (IC_RESULT (dic)), FALSE)))
	return NULL;

      if (POINTER_GET (dic) &&
	  !IS_DATA_PTR (aggrToPtr (operandType (IC_LEFT (dic)), FALSE)))
	return NULL;

      /* if address of & the result is remat then okay */
      if (dic->op == ADDRESS_OF &&
	  OP_SYMBOL (IC_RESULT (dic))->remat)
	continue;

      /* if operand has size of three or more & this
         operation is a '*','/' or '%' then 'b' may
         cause a problem */
      if ((dic->op == '%' || dic->op == '/' || dic->op == '*') &&
	  getSize (operandType (op)) >= 3)
	return NULL;

      /* if left or right or result is in far space */
      if (isOperandInFarSpace (IC_LEFT (dic)) ||
	  isOperandInFarSpace (IC_RIGHT (dic)) ||
	  isOperandInFarSpace (IC_RESULT (dic)) ||
	  IS_OP_RUONLY (IC_LEFT (dic)) ||
	  IS_OP_RUONLY (IC_RIGHT (dic)) ||
	  IS_OP_RUONLY (IC_RESULT (dic)))
	{
	  return NULL;
	}
    }

  OP_SYMBOL (op)->ruonly = 1;
  return sic;

}

/*-----------------------------------------------------------------*/
/* isBitwiseOptimizable - requirements of JEAN LOUIS VERN          */
/*-----------------------------------------------------------------*/
static bool
isBitwiseOptimizable (iCode * ic)
{
  sym_link *ltype = getSpec (operandType (IC_LEFT (ic)));
  sym_link *rtype = getSpec (operandType (IC_RIGHT (ic)));

  debugLog ("%s\n", __FUNCTION__);
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
  if (IS_LITERAL (rtype) ||
      (IS_BITVAR (ltype) && IN_BITSPACE (SPEC_OCLS (ltype))))
    return TRUE;
  else
    return FALSE;
}

/*-----------------------------------------------------------------*/
/* packRegsForAccUse - pack registers for acc use                  */
/*-----------------------------------------------------------------*/
static void
packRegsForAccUse (iCode * ic)
{
  iCode *uic;

  debugLog ("%s\n", __FUNCTION__);

  /* if this is an aggregate, e.g. a one byte char array */
  if (IS_AGGREGATE(operandType(IC_RESULT(ic)))) {
    return;
  }
  debugLog ("  %s:%d\n", __FUNCTION__,__LINE__);

  /* if + or - then it has to be one byte result */
  if ((ic->op == '+' || ic->op == '-')
      && getSize (operandType (IC_RESULT (ic))) > 1)
    return;

  debugLog ("  %s:%d\n", __FUNCTION__,__LINE__);
  /* if shift operation make sure right side is not a literal */
  if (ic->op == RIGHT_OP &&
      (isOperandLiteral (IC_RIGHT (ic)) ||
       getSize (operandType (IC_RESULT (ic))) > 1))
    return;

  if (ic->op == LEFT_OP &&
      (isOperandLiteral (IC_RIGHT (ic)) ||
       getSize (operandType (IC_RESULT (ic))) > 1))
    return;

  if (IS_BITWISE_OP (ic) &&
      getSize (operandType (IC_RESULT (ic))) > 1)
    return;


  /* has only one definition */
  if (bitVectnBitsOn (OP_DEFS (IC_RESULT (ic))) > 1)
    return;

  /* has only one use */
  if (bitVectnBitsOn (OP_USES (IC_RESULT (ic))) > 1)
    return;

  /* and the usage immediately follows this iCode */
  if (!(uic = hTabItemWithKey (iCodehTab,
			       bitVectFirstBit (OP_USES (IC_RESULT (ic))))))
    return;

  debugLog ("  %s:%d\n", __FUNCTION__,__LINE__);
  if (ic->next != uic)
    return;

  /* if it is a conditional branch then we definitely can */
  if (uic->op == IFX)
    goto accuse;

  if (uic->op == JUMPTABLE)
    return;

  /* if the usage is not is an assignment
     or an arithmetic / bitwise / shift operation then not */
  if (POINTER_SET (uic) &&
      getSize (aggrToPtr (operandType (IC_RESULT (uic)), FALSE)) > 1)
    return;

  debugLog ("  %s:%d\n", __FUNCTION__,__LINE__);
  if (uic->op != '=' &&
      !IS_ARITHMETIC_OP (uic) &&
      !IS_BITWISE_OP (uic) &&
      uic->op != LEFT_OP &&
      uic->op != RIGHT_OP)
    return;

  debugLog ("  %s:%d\n", __FUNCTION__,__LINE__);
  /* if used in ^ operation then make sure right is not a 
     literl */
  if (uic->op == '^' && isOperandLiteral (IC_RIGHT (uic)))
    return;

  /* if shift operation make sure right side is not a literal */
  if (uic->op == RIGHT_OP &&
      (isOperandLiteral (IC_RIGHT (uic)) ||
       getSize (operandType (IC_RESULT (uic))) > 1))
    return;

  if (uic->op == LEFT_OP &&
      (isOperandLiteral (IC_RIGHT (uic)) ||
       getSize (operandType (IC_RESULT (uic))) > 1))
    return;

  /* make sure that the result of this icode is not on the
     stack, since acc is used to compute stack offset */
  if (IS_TRUE_SYMOP (IC_RESULT (uic)) &&
      OP_SYMBOL (IC_RESULT (uic))->onStack)
    return;

  /* if either one of them in far space then we cannot */
  if ((IS_TRUE_SYMOP (IC_LEFT (uic)) &&
       isOperandInFarSpace (IC_LEFT (uic))) ||
      (IS_TRUE_SYMOP (IC_RIGHT (uic)) &&
       isOperandInFarSpace (IC_RIGHT (uic))))
    return;

  /* if the usage has only one operand then we can */
  if (IC_LEFT (uic) == NULL ||
      IC_RIGHT (uic) == NULL)
    goto accuse;

  /* make sure this is on the left side if not
     a '+' since '+' is commutative */
  if (ic->op != '+' &&
      IC_LEFT (uic)->key != IC_RESULT (ic)->key)
    return;

  debugLog ("  %s:%d\n", __FUNCTION__,__LINE__);
  /* if one of them is a literal then we can */
  if ( ((IC_LEFT (uic) && IS_OP_LITERAL (IC_LEFT (uic))) ||
	(IC_RIGHT (uic) && IS_OP_LITERAL (IC_RIGHT (uic))))  &&
       (getSize (operandType (IC_RESULT (uic))) <= 1))
    {
      OP_SYMBOL (IC_RESULT (ic))->accuse = 1;
      return;
    }

  debugLog ("  %s:%d\n", __FUNCTION__,__LINE__);
  /* if the other one is not on stack then we can */
  if (IC_LEFT (uic)->key == IC_RESULT (ic)->key &&
      (IS_ITEMP (IC_RIGHT (uic)) ||
       (IS_TRUE_SYMOP (IC_RIGHT (uic)) &&
	!OP_SYMBOL (IC_RIGHT (uic))->onStack)))
    goto accuse;

  if (IC_RIGHT (uic)->key == IC_RESULT (ic)->key &&
      (IS_ITEMP (IC_LEFT (uic)) ||
       (IS_TRUE_SYMOP (IC_LEFT (uic)) &&
	!OP_SYMBOL (IC_LEFT (uic))->onStack)))
    goto accuse;

  return;

accuse:
  debugLog ("%s - Yes we are using the accumulator\n", __FUNCTION__);
  OP_SYMBOL (IC_RESULT (ic))->accuse = 1;


}

/*-----------------------------------------------------------------*/
/* packForPush - hueristics to reduce iCode for pushing            */
/*-----------------------------------------------------------------*/
static void
packForReceive (iCode * ic, eBBlock * ebp)
{
  iCode *dic;

  debugLog ("%s\n", __FUNCTION__);
  debugAopGet ("  result:", IC_RESULT (ic));
  debugAopGet ("  left:", IC_LEFT (ic));
  debugAopGet ("  right:", IC_RIGHT (ic));

  if (!ic->next)
    return;

  for (dic = ic->next; dic; dic = dic->next)
    {



      if (IC_LEFT (dic) && (IC_RESULT (ic)->key == IC_LEFT (dic)->key))
	debugLog ("    used on left\n");
      if (IC_RIGHT (dic) && IC_RESULT (ic)->key == IC_RIGHT (dic)->key)
	debugLog ("    used on right\n");
      if (IC_RESULT (dic) && IC_RESULT (ic)->key == IC_RESULT (dic)->key)
	debugLog ("    used on result\n");

      if ((IC_LEFT (dic) && (IC_RESULT (ic)->key == IC_LEFT (dic)->key)) ||
	  (IC_RESULT (dic) && IC_RESULT (ic)->key == IC_RESULT (dic)->key))
	return;

    }

  debugLog ("  hey we can remove this unnecessary assign\n");
}
/*-----------------------------------------------------------------*/
/* packForPush - hueristics to reduce iCode for pushing            */
/*-----------------------------------------------------------------*/
static void
packForPush (iCode * ic, eBBlock * ebp)
{
  iCode *dic;

  debugLog ("%s\n", __FUNCTION__);
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

  /* we now we know that it has one & only one def & use
     and the that the definition is an assignment */
  IC_LEFT (ic) = IC_RIGHT (dic);

  remiCodeFromeBBlock (ebp, dic);
  bitVectUnSetBit(OP_SYMBOL(IC_RESULT(dic))->defs,dic->key);
  hTabDeleteItem (&iCodehTab, dic->key, dic, DELETE_ITEM, NULL);
}

static void printSymType(char * str, sym_link *sl)
{
  debugLog ("    %s Symbol type: ",str);
  printTypeChain( sl, debugF);
  debugLog ("\n");

}

/*-----------------------------------------------------------------*/
/* some debug code to print the symbol S_TYPE. Note that
 * the function checkSClass in src/SDCCsymt.c dinks with
 * the S_TYPE in ways the PIC port doesn't fully like...*/
/*-----------------------------------------------------------------*/
static void isData(sym_link *sl)
{
  FILE *of = stderr;

  if(!sl)
    return;

  if(debugF)
    of = debugF;

  for ( ; sl; sl=sl->next) {
    if(!IS_DECL(sl) ) {
      switch (SPEC_SCLS(sl)) {
	
      case S_DATA: fprintf (of, "data "); break;
      case S_XDATA: fprintf (of, "xdata "); break;
      case S_SFR: fprintf (of, "sfr "); break;
      case S_SBIT: fprintf (of, "sbit "); break;
      case S_CODE: fprintf (of, "code "); break;
      case S_IDATA: fprintf (of, "idata "); break;
      case S_PDATA: fprintf (of, "pdata "); break;
      case S_LITERAL: fprintf (of, "literal "); break;
      case S_STACK: fprintf (of, "stack "); break;
      case S_XSTACK: fprintf (of, "xstack "); break;
      case S_BIT: fprintf (of, "bit "); break;
      case S_EEPROM: fprintf (of, "eeprom "); break;
      default: break;
      }

    }

  }
    
}
/*--------------------------------------------------------------------*/
/* pic16_packRegisters - does some transformations to reduce          */
/*                   register pressure                                */
/*                                                                    */
/*--------------------------------------------------------------------*/
static void
pic16_packRegisters (eBBlock * ebp)
{
  iCode *ic;
  int change = 0;

  debugLog ("%s\n", __FUNCTION__);

  while (1) {

    change = 0;

    /* look for assignments of the form */
    /* iTempNN = TRueSym (someoperation) SomeOperand */
    /*       ....                       */
    /* TrueSym := iTempNN:1             */
    for (ic = ebp->sch; ic; ic = ic->next)
      {

	/* find assignment of the form TrueSym := iTempNN:1 */
	/* see BUGLOG0001 for workaround with the CAST - VR */
	if ((ic->op == '=' || ic->op == CAST) && !POINTER_SET (ic))
	  change += packRegsForAssign (ic, ebp);
	/* debug stuff */
	if (ic->op == '=')
	  {
	    if (POINTER_SET (ic))
	      debugLog ("pointer is set\n");
	    debugAopGet ("  result:", IC_RESULT (ic));
	    debugAopGet ("  left:", IC_LEFT (ic));
	    debugAopGet ("  right:", IC_RIGHT (ic));
	  }

      }

    if (!change)
      break;
  }

  for (ic = ebp->sch; ic; ic = ic->next) {

    if(IS_SYMOP ( IC_LEFT(ic))) {
      sym_link *etype = getSpec (operandType (IC_LEFT (ic)));

      debugAopGet ("  left:", IC_LEFT (ic));
#if 0
      if(IS_PTR_CONST(OP_SYMBOL(IC_LEFT(ic))->type))
#else
      if(IS_CODEPTR(OP_SYMBOL(IC_LEFT(ic))->type))
#endif
	debugLog ("    is a pointer\n");

      if(IS_OP_VOLATILE(IC_LEFT(ic)))
	debugLog ("    is volatile\n");

      isData(etype);

      printSymType("   ", OP_SYMBOL(IC_LEFT(ic))->type);
    }

    if(IS_SYMOP ( IC_RIGHT(ic))) {
      debugAopGet ("  right:", IC_RIGHT (ic));
      printSymType("    ", OP_SYMBOL(IC_RIGHT(ic))->type);
    }

    if(IS_SYMOP ( IC_RESULT(ic))) {
      debugAopGet ("  result:", IC_RESULT (ic));
      printSymType("     ", OP_SYMBOL(IC_RESULT(ic))->type);
    }

    if (POINTER_SET (ic))
	debugLog ("  %d - Pointer set\n", __LINE__);


    /* if this is an itemp & result of a address of a true sym 
       then mark this as rematerialisable   */
    if (ic->op == ADDRESS_OF &&
	IS_ITEMP (IC_RESULT (ic)) &&
	IS_TRUE_SYMOP (IC_LEFT (ic)) &&
	bitVectnBitsOn (OP_DEFS (IC_RESULT (ic))) == 1 &&
	!OP_SYMBOL (IC_LEFT (ic))->onStack)
      {

	debugLog ("  %d - %s. result is rematerializable\n", __LINE__,__FUNCTION__);

	OP_SYMBOL (IC_RESULT (ic))->remat = 1;
	OP_SYMBOL (IC_RESULT (ic))->rematiCode = ic;
	OP_SYMBOL (IC_RESULT (ic))->usl.spillLoc = NULL;

      }

    /* if straight assignment then carry remat flag if
       this is the only definition */
    if (ic->op == '=' &&
	!POINTER_SET (ic) &&
	IS_SYMOP (IC_RIGHT (ic)) &&
	OP_SYMBOL (IC_RIGHT (ic))->remat &&
	bitVectnBitsOn (OP_SYMBOL (IC_RESULT (ic))->defs) <= 1)
      {
	debugLog ("  %d - %s. straight rematerializable\n", __LINE__,__FUNCTION__);

	OP_SYMBOL (IC_RESULT (ic))->remat =
	  OP_SYMBOL (IC_RIGHT (ic))->remat;
	OP_SYMBOL (IC_RESULT (ic))->rematiCode =
	  OP_SYMBOL (IC_RIGHT (ic))->rematiCode;
      }

    /* if this is a +/- operation with a rematerizable 
       then mark this as rematerializable as well */
    if ((ic->op == '+' || ic->op == '-') &&
	(IS_SYMOP (IC_LEFT (ic)) &&
	 IS_ITEMP (IC_RESULT (ic)) &&
	 OP_SYMBOL (IC_LEFT (ic))->remat &&
	 bitVectnBitsOn (OP_DEFS (IC_RESULT (ic))) == 1 &&
	 IS_OP_LITERAL (IC_RIGHT (ic))))
      {
	debugLog ("  %d - %s. rematerializable because op is +/-\n", __LINE__,__FUNCTION__);
	//int i = 
	operandLitValue (IC_RIGHT (ic));
	OP_SYMBOL (IC_RESULT (ic))->remat = 1;
	OP_SYMBOL (IC_RESULT (ic))->rematiCode = ic;
	OP_SYMBOL (IC_RESULT (ic))->usl.spillLoc = NULL;
      }

    /* mark the pointer usages */
    if (POINTER_SET (ic))
      {
	OP_SYMBOL (IC_RESULT (ic))->uptr = 1;
	debugLog ("  marking as a pointer (set) =>");
	debugAopGet ("  result:", IC_RESULT (ic));
      }
    if (POINTER_GET (ic))
      {
	OP_SYMBOL (IC_LEFT (ic))->uptr = 1;
	debugLog ("  marking as a pointer (get) =>");
	debugAopGet ("  left:", IC_LEFT (ic));
      }

    if (!SKIP_IC2 (ic))
      {
	/* if we are using a symbol on the stack
	   then we should say pic16_ptrRegReq */
	if (ic->op == IFX && IS_SYMOP (IC_COND (ic)))
	  pic16_ptrRegReq += ((OP_SYMBOL (IC_COND (ic))->onStack ||
			       OP_SYMBOL (IC_COND (ic))->iaccess) ? 1 : 0);
	else if (ic->op == JUMPTABLE && IS_SYMOP (IC_JTCOND (ic)))
	  pic16_ptrRegReq += ((OP_SYMBOL (IC_JTCOND (ic))->onStack ||
			       OP_SYMBOL (IC_JTCOND (ic))->iaccess) ? 1 : 0);
	else
	  {
	    if (IS_SYMOP (IC_LEFT (ic)))
	      pic16_ptrRegReq += ((OP_SYMBOL (IC_LEFT (ic))->onStack ||
				   OP_SYMBOL (IC_LEFT (ic))->iaccess) ? 1 : 0);
	    if (IS_SYMOP (IC_RIGHT (ic)))
	      pic16_ptrRegReq += ((OP_SYMBOL (IC_RIGHT (ic))->onStack ||
				   OP_SYMBOL (IC_RIGHT (ic))->iaccess) ? 1 : 0);
	    if (IS_SYMOP (IC_RESULT (ic)))
	      pic16_ptrRegReq += ((OP_SYMBOL (IC_RESULT (ic))->onStack ||
				   OP_SYMBOL (IC_RESULT (ic))->iaccess) ? 1 : 0);
	  }

	debugLog ("  %d - pointer reg req = %d\n", __LINE__,pic16_ptrRegReq);

      }

    /* if the condition of an if instruction
       is defined in the previous instruction then
       mark the itemp as a conditional */
    if ((IS_CONDITIONAL (ic) ||
	 ((ic->op == BITWISEAND ||
	   ic->op == '|' ||
	   ic->op == '^') &&
	  isBitwiseOptimizable (ic))) &&
	ic->next && ic->next->op == IFX &&
	isOperandEqual (IC_RESULT (ic), IC_COND (ic->next)) &&
	OP_SYMBOL (IC_RESULT (ic))->liveTo <= ic->next->seq)
      {

	debugLog ("  %d\n", __LINE__);
	OP_SYMBOL (IC_RESULT (ic))->regType = REG_CND;
	continue;
      }

    /* reduce for support function calls */
    if (ic->supportRtn || ic->op == '+' || ic->op == '-')
      packRegsForSupport (ic, ebp);

    /* if a parameter is passed, it's in W, so we may not
       need to place a copy in a register */
    if (ic->op == RECEIVE)
      packForReceive (ic, ebp);

    /* some cases the redundant moves can
       can be eliminated for return statements */
    if ((ic->op == RETURN || ic->op == SEND) &&
	!isOperandInFarSpace (IC_LEFT (ic)) &&
	!options.model)
      packRegsForOneuse (ic, IC_LEFT (ic), ebp);

    /* if pointer set & left has a size more than
       one and right is not in far space */
    if (POINTER_SET (ic) &&
	!isOperandInFarSpace (IC_RIGHT (ic)) &&
	!OP_SYMBOL (IC_RESULT (ic))->remat &&
	!IS_OP_RUONLY (IC_RIGHT (ic)) &&
	getSize (aggrToPtr (operandType (IC_RESULT (ic)), FALSE)) > 1)

      packRegsForOneuse (ic, IC_RESULT (ic), ebp);

    /* if pointer get */
    if (POINTER_GET (ic) &&
	!isOperandInFarSpace (IC_RESULT (ic)) &&
	!OP_SYMBOL (IC_LEFT (ic))->remat &&
	!IS_OP_RUONLY (IC_RESULT (ic)) &&
	getSize (aggrToPtr (operandType (IC_LEFT (ic)), FALSE)) > 1)

      packRegsForOneuse (ic, IC_LEFT (ic), ebp);


    /* if this is cast for intergral promotion then
       check if only use of  the definition of the 
       operand being casted/ if yes then replace
       the result of that arithmetic operation with 
       this result and get rid of the cast */
    if (ic->op == CAST) {
	
      sym_link *fromType = operandType (IC_RIGHT (ic));
      sym_link *toType = operandType (IC_LEFT (ic));

      debugLog ("  %d - casting\n", __LINE__);

      if (IS_INTEGRAL (fromType) && IS_INTEGRAL (toType) &&
	  getSize (fromType) != getSize (toType)) {
	    

	iCode *dic = packRegsForOneuse (ic, IC_RIGHT (ic), ebp);
	if (dic) {
		
	  if (IS_ARITHMETIC_OP (dic)) {
		    
	    bitVectUnSetBit(OP_SYMBOL(IC_RESULT(dic))->defs,dic->key);
	    IC_RESULT (dic) = IC_RESULT (ic);
	    remiCodeFromeBBlock (ebp, ic);
	    bitVectUnSetBit(OP_SYMBOL(IC_RESULT(ic))->defs,ic->key);
	    hTabDeleteItem (&iCodehTab, ic->key, ic, DELETE_ITEM, NULL);
	    OP_DEFS (IC_RESULT (dic)) = bitVectSetBit (OP_DEFS (IC_RESULT (dic)), dic->key);
	    ic = ic->prev;
	  }  else
		
	    OP_SYMBOL (IC_RIGHT (ic))->ruonly = 0;
	}
      } else {

	/* if the type from and type to are the same
	   then if this is the only use then packit */
	if (compareType (operandType (IC_RIGHT (ic)),
			 operandType (IC_LEFT (ic))) == 1) {
		
	  iCode *dic = packRegsForOneuse (ic, IC_RIGHT (ic), ebp);
	  if (dic) {
		    
	    bitVectUnSetBit(OP_SYMBOL(IC_RESULT(dic))->defs,dic->key);
	    IC_RESULT (dic) = IC_RESULT (ic);
	    bitVectUnSetBit(OP_SYMBOL(IC_RESULT(ic))->defs,ic->key);
	    remiCodeFromeBBlock (ebp, ic);
	    hTabDeleteItem (&iCodehTab, ic->key, ic, DELETE_ITEM, NULL);
	    OP_DEFS (IC_RESULT (dic)) = bitVectSetBit (OP_DEFS (IC_RESULT (dic)), dic->key);
	    ic = ic->prev;
	  }
	}
      }
    }

    /* pack for PUSH 
       iTempNN := (some variable in farspace) V1
       push iTempNN ;
       -------------
       push V1
    */
    if (ic->op == IPUSH)
      {
	packForPush (ic, ebp);
      }


    /* pack registers for accumulator use, when the
       result of an arithmetic or bit wise operation
       has only one use, that use is immediately following
       the defintion and the using iCode has only one
       operand or has two operands but one is literal &
       the result of that operation is not on stack then
       we can leave the result of this operation in acc:b
       combination */
    if ((IS_ARITHMETIC_OP (ic)

	 || IS_BITWISE_OP (ic)

	 || ic->op == LEFT_OP || ic->op == RIGHT_OP

	 ) &&
	IS_ITEMP (IC_RESULT (ic)) &&
	getSize (operandType (IC_RESULT (ic))) <= 2)

      packRegsForAccUse (ic);

  }
}

static void
dumpEbbsToDebug (eBBlock ** ebbs, int count)
{
  int i;

  if (!debug || !debugF)
    return;

  for (i = 0; i < count; i++)
    {
      fprintf (debugF, "\n----------------------------------------------------------------\n");
      fprintf (debugF, "Basic Block %s : loop Depth = %d noPath = %d , lastinLoop = %d\n",
	       ebbs[i]->entryLabel->name,
	       ebbs[i]->depth,
	       ebbs[i]->noPath,
	       ebbs[i]->isLastInLoop);
      fprintf (debugF, "depth 1st num %d : bbnum = %d 1st iCode = %d , last iCode = %d\n",
	       ebbs[i]->dfnum,
	       ebbs[i]->bbnum,
	       ebbs[i]->fSeq,
	       ebbs[i]->lSeq);
      fprintf (debugF, "visited %d : hasFcall = %d\n",
	       ebbs[i]->visited,
	       ebbs[i]->hasFcall);

      fprintf (debugF, "\ndefines bitVector :");
      bitVectDebugOn (ebbs[i]->defSet, debugF);
      fprintf (debugF, "\nlocal defines bitVector :");
      bitVectDebugOn (ebbs[i]->ldefs, debugF);
      fprintf (debugF, "\npointers Set bitvector :");
      bitVectDebugOn (ebbs[i]->ptrsSet, debugF);
      fprintf (debugF, "\nin pointers Set bitvector :");
      bitVectDebugOn (ebbs[i]->inPtrsSet, debugF);
      fprintf (debugF, "\ninDefs Set bitvector :");
      bitVectDebugOn (ebbs[i]->inDefs, debugF);
      fprintf (debugF, "\noutDefs Set bitvector :");
      bitVectDebugOn (ebbs[i]->outDefs, debugF);
      fprintf (debugF, "\nusesDefs Set bitvector :");
      bitVectDebugOn (ebbs[i]->usesDefs, debugF);
      fprintf (debugF, "\n----------------------------------------------------------------\n");
      printiCChain (ebbs[i]->sch, debugF);
    }
}
/*-----------------------------------------------------------------*/
/* pic16_assignRegisters - assigns registers to each live range as need  */
/*-----------------------------------------------------------------*/
void
pic16_assignRegisters (eBBlock ** ebbs, int count)
{
  iCode *ic;
  int i;

  debugLog ("<><><><><><><><><><><><><><><><><>\nstarting\t%s:%s", __FILE__, __FUNCTION__);
  debugLog ("\nebbs before optimizing:\n");
  dumpEbbsToDebug (ebbs, count);

  setToNull ((void *) &_G.funcrUsed);
  pic16_ptrRegReq = _G.stackExtend = _G.dataExtend = 0;


  /* change assignments this will remove some
     live ranges reducing some register pressure */
  for (i = 0; i < count; i++)
    pic16_packRegisters (ebbs[i]);

  {
    regs *reg;
    int hkey;
    int i=0;

    debugLog("dir registers allocated so far:\n");
    reg = hTabFirstItem(dynDirectRegNames, &hkey);

    while(reg) {
      debugLog("  -- #%d reg = %s  key %d, rIdx = %d, size %d\n",i++,reg->name,hkey, reg->rIdx,reg->size);
      reg = hTabNextItem(dynDirectRegNames, &hkey);
    }

  }

  /* liveranges probably changed by register packing
     so we compute them again */
  recomputeLiveRanges (ebbs, count);

  if (options.dump_pack)
    dumpEbbsToFileExt (DUMP_PACK, ebbs, count);

  /* first determine for each live range the number of 
     registers & the type of registers required for each */
  regTypeNum ();

  /* and serially allocate registers */
  serialRegAssign (ebbs, count);

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
  redoStackOffsets ();

  if (options.dump_rassgn)
    dumpEbbsToFileExt (DUMP_RASSGN, ebbs, count);

  /* now get back the chain */
  ic = iCodeLabelOptimize (iCodeFromeBBlock (ebbs, count));

  debugLog ("ebbs after optimizing:\n");
  dumpEbbsToDebug (ebbs, count);


  genpic16Code (ic);

  /* free up any _G.stackSpil locations allocated */
  applyToSet (_G.stackSpil, deallocStackSpil);
  _G.slocNum = 0;
  setToNull ((void *) &_G.stackSpil);
  setToNull ((void *) &_G.spiltSet);
  /* mark all registers as free */
  //pic16_freeAllRegs ();

  debugLog ("leaving\n<><><><><><><><><><><><><><><><><>\n");
  debugLogClose ();
  return;
}
