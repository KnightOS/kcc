/*-------------------------------------------------------------------------

  SDCCralloc.h - header file register allocation

                Written By -  Sandeep Dutta . sandeep.dutta@usa.net (1998)
		PIC port   - T. Scott Dattalo scott@dattalo.com (2000)

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
#include "SDCCicode.h"
#include "SDCCBBlock.h"
#ifndef SDCCRALLOC_H
#define SDCCRALLOC_H 1



enum
  {
    R2_IDX = 0, R3_IDX, R4_IDX,
    R5_IDX, R6_IDX, R7_IDX,
    R0_IDX, R1_IDX, X8_IDX,
    X9_IDX, X10_IDX, X11_IDX,
    X12_IDX, CND_IDX
  };


#define REG_PTR 0x01
#define REG_GPR 0x02
#define REG_CND 0x04
#define REG_SFR 0x08

/* definition for the registers */
typedef struct regs
  {
    short type;			/* can have value 
				 * REG_GPR, REG_PTR or REG_CND 
				 * This like the "meta-type" */
    short pc_type;              /* pcode type */
    short rIdx;			/* index into register table */
    //    short otype;        
    char *name;			/* name */
    char *dname;		/* name when direct access needed */
    //  char *base ;         /* base address */
    short offset;		/* offset from the base */
    unsigned isFree:1;		/* is currently unassigned  */
    unsigned wasUsed:1;		/* becomes true if register has been used */
  }
regs;
extern regs regspic14[];
extern int pic14_nRegs;

regs *pic14_regWithIdx (int);
void  pic14_freeAllRegs ();
void  pic14_deallocateAllRegs ();
regs *pic14_findFreeReg(void);

enum PIC_register_types
  {
    PIC_INDF,
    PIC_TMR0,
    PIC_FSR,
    PIC_STATUS,
    PIC_IOPORT,
    PIC_IOTRIS,
    PIC_GPR,			/*  A general purpose file register */
    PIC_SFR			/*  A special function register */
  };


#endif
