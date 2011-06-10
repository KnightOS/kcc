/*-------------------------------------------------------------------------
  SDCCgen51.h - header file for code generation for 8051

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

#ifndef SDCCGEN51_H
#define SDCCGEN51_H

enum
  {
    AOP_LIT = 1,
    AOP_REG, AOP_DIR,
    AOP_STK, AOP_IMMD, AOP_STR,
    AOP_CRY, 
    AOP_EXT, AOP_SOF, AOP_DUMMY
  };

enum
  {
    ACCUSE_XA = 1,
    ACCUSE_HX
  };

/* type asmop : a homogenised type for 
   all the different spaces an operand can be
   in */
typedef struct asmop
  {

    short type;			
    /* can have values
       AOP_LIT    -  operand is a literal value
       AOP_REG    -  is in registers
       AOP_DIR    -  operand using direct addressing mode
       AOP_STK    -  should be pushed on stack this
       can happen only for the result
       AOP_IMMD   -  immediate value for eg. remateriazable 
       AOP_CRY    -  carry contains the value of this
       AOP_STR    -  array of strings
       AOP_SOF    -  operand at an offset on the stack
       AOP_EXT    -  operand using extended addressing mode
    */
    short coff;			/* current offset */
    short size;			/* total size */
    short psize;		/* pointer size */
    operand *op;		/* originating operand */
    unsigned code:1;		/* is in Code space */
    unsigned paged:1;		/* in paged memory  */
    unsigned freed:1;		/* already freed    */
    unsigned isaddr:1;		/* is an address to actual operand */
    unsigned stacked:1;		/* partial results stored on stack */
    struct asmop *stk_aop[4];	/* asmops for the results on the stack */
    union
      {
	value *aop_lit;		/* if literal */
	reg_info *aop_reg[4];	/* array of registers */
	char *aop_dir;		/* if direct  */
	reg_info *aop_ptr;		/* either -> to r0 or r1 */
	struct {
		int  from_cast_remat;   /* cast remat created this : immd2 field used for highest order*/
		char *aop_immd1;	/* if immediate others are implied */
		char *aop_immd2;	/* cast remat will generate this   */
	} aop_immd;
	int aop_stk;		/* stack offset when AOP_STK */
	char *aop_str[4];	/* just a string array containing the location */
      }
    aopu;
  }
asmop;

void genhc08Code (iCode *);
void hc08_emitDebuggerSymbol (const char *);

//extern char *fReturn8051[];
extern unsigned fReturnSizeHC08;
//extern char **fReturn;

#endif
