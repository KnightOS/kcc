/*
 * Simulator of microcontrollers (xa.cc)
 *
 * Copyright (C) 1999,99 Drotos Daniel, Talker Bt.
 *
 * Written by Karl Bongers karl@turbobit.com
 * 
 * To contact author send email to drdani@mazsola.iit.uni-miskolc.hu
 *
 */

/* This file is part of microcontroller simulator: ucsim.

UCSIM is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

UCSIM is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with UCSIM; see the file COPYING.  If not, write to the Free
Software Foundation, 59 Temple Place - Suite 330, Boston, MA
02111-1307, USA. */
/*@1@*/

#include "ddconfig.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "i_string.h"

// prj
#include "pobjcl.h"

// sim
#include "simcl.h"

// local
#include "xacl.h"
#include "glob.h"
#include "regsxa.h"


/*
 * Base type of xa controllers
 */

cl_xa::cl_xa(class cl_sim *asim):
  cl_uc(asim)
{
  type= CPU_XA;
}

int
cl_xa::init(void)
{
  cl_uc::init(); /* Memories now exist */
  ram= mem(MEM_XRAM);
  rom= mem(MEM_ROM);

  wmem_direct = (TYPE_UWORD *) &mem_direct[0];

  /* initialize SP to 100H */
  set_reg2(7*2, 0x100);

  printf("The XA Simulator is in development, UNSTABLE, DEVELOPERS ONLY!\n");

  return(0);
}

char *
cl_xa::id_string(void)
{
  return("unspecified XA");
}


/*
 * Making elements of the controller
 */

t_addr
cl_xa::get_mem_size(enum mem_class type)
{
  switch(type)
    {
    case MEM_ROM: return(0x10000);
    case MEM_XRAM: return(0x10000);
    default: return(0);
    }
 return(cl_uc::get_mem_size(type));
}

void
cl_xa::mk_hw_elements(void)
{
  //class cl_base *o;
  /* t_uc::mk_hw() does nothing */
}


/*
 * Help command interpreter
 */

struct dis_entry *
cl_xa::dis_tbl(void)
{
  // this should be unused, we need to make main prog code
  // independent of any array thing.
  printf("ERROR - Using disass[] table in XA sim code!\n");
  return(glob_disass_xa);
}

/*struct name_entry *
cl_xa::sfr_tbl(void)
{
  return(0);
}*/

/*struct name_entry *
cl_xa::bit_tbl(void)
{
  //FIXME
  return(0);
}*/

int
cl_xa::inst_length(t_addr addr)
{
  int len = 0;

  get_disasm_info(addr, &len, NULL, NULL, NULL, NULL);

  return len;
}

int
cl_xa::inst_branch(t_addr addr)
{
  int b;

  get_disasm_info(addr, NULL, &b, NULL, NULL, NULL);

  return b;
}

int
cl_xa::longest_inst(void)
{
  return 6;
}

/*--------------------------------------------------------------------
get_disasm_info -
|--------------------------------------------------------------------*/
int
cl_xa::get_disasm_info(t_addr addr,
                       int *ret_len,
                       int *ret_branch,
                       int *immed_offset,
                       int *parms,
                       int *mnemonic)
{
  uint code;
  int len = 0;
  int immed_n = 0;
  int i;
  int start_addr = addr;

  code= get_mem(MEM_ROM, addr++);
  if (code == 0x00) {
    i= 0;
    while (disass_xa[i].mnemonic != NOP)
      i++;
  } else {
    len = 2;
    code = (code << 8) | get_mem(MEM_ROM, addr++);
    i= 0;
    while ((code & disass_xa[i].mask) != disass_xa[i].code &&
           disass_xa[i].mnemonic != BAD_OPCODE)
      i++;
  }

  if (ret_len)
    *ret_len = disass_xa[i].length;
  if (ret_branch)
   *ret_branch = disass_xa[i].branch;
  if (immed_offset) {
    if (immed_n > 0)
         *immed_offset = immed_n;
    else *immed_offset = (addr - start_addr);
  }
  if (parms) {
    *parms = disass_xa[i].operands;
  }
  if (mnemonic) {
    *mnemonic = disass_xa[i].mnemonic;
  }

  return code;
}

static char *w_reg_strs[] = {
 "R0", "R1",
 "R2", "R3",
 "R4", "R5",
 "R6", "R7",
 "R8", "R9",
 "R10", "R11",
 "R12", "R13",
 "R14", "R15"};

static char *b_reg_strs[] = {
 "R0l", "R0h",
 "R1l", "R1h",
 "R2l", "R2h",
 "R3l", "R3h",
 "R4l", "R4h",
 "R5l", "R5h",
 "R6l", "R6h",
 "R7l", "R7h"};

/*--------------------------------------------------------------------
disass -
|--------------------------------------------------------------------*/
char *
cl_xa::disass(t_addr addr, char *sep)
{
  char work[256], parm_str[40];
  char *buf, *p, *b;
  int code;
  int len = 0;
  int immed_offset = 0;
  int operands;
  int mnemonic;
  char **reg_strs;

  p= work;

  code = get_disasm_info(addr, &len, NULL, &immed_offset, &operands, &mnemonic);

  if (mnemonic == BAD_OPCODE) {
    buf= (char*)malloc(30);
    strcpy(buf, "UNKNOWN/INVALID");
    return(buf);
  }

  if (code & 0x0800)
    reg_strs = w_reg_strs;
  else
    reg_strs = b_reg_strs;

  switch(operands) {
     // the repeating common parameter encoding for ADD, ADDC, SUB, AND...
    case REG_REG :
      sprintf(parm_str, "%s,%s",
              reg_strs[((code >> 4) & 0xf)],
              reg_strs[(code & 0xf)]);
    break;
    case REG_IREG :
      sprintf(parm_str, "%s,[%s]",
              reg_strs[((code >> 4) & 0xf)],
              w_reg_strs[(code & 0xf)]);
    break;
    case IREG_REG :
      sprintf(parm_str, "[%s],%s",
              w_reg_strs[(code & 0x7)],
              reg_strs[((code >> 4) & 0xf)] );
    break;
    case REG_IREGOFF8 :
      sprintf(parm_str, "%s,[%s+%02d]",
              reg_strs[((code >> 4) & 0xf)],
              w_reg_strs[(code & 0x7)],
              get_mem(MEM_ROM, addr+immed_offset));
      ++immed_offset;
    break;
    case IREGOFF8_REG :
      sprintf(parm_str, "[%s+%02d],%s",
              w_reg_strs[(code & 0x7)],
              get_mem(MEM_ROM, addr+immed_offset),
              reg_strs[((code >> 4) & 0xf)] );
      ++immed_offset;
    break;
    case REG_IREGOFF16 :
      sprintf(parm_str, "%s,[%s+%04d]",
              reg_strs[((code >> 4) & 0xf)],
              w_reg_strs[(code & 0x7)],
              (short)((get_mem(MEM_ROM, addr+immed_offset+1)) |
                     (get_mem(MEM_ROM, addr+immed_offset)<<8)) );
      ++immed_offset;
      ++immed_offset;
    break;
    case IREGOFF16_REG :
      sprintf(parm_str, "[%s+%04d],%s",
              w_reg_strs[(code & 0x7)],
              (short)((get_mem(MEM_ROM, addr+immed_offset+1)) |
                     (get_mem(MEM_ROM, addr+immed_offset)<<8)),
              reg_strs[((code >> 4) & 0xf)] );
      ++immed_offset;
      ++immed_offset;
    break;
    case REG_IREGINC :
      sprintf(parm_str, "%s,[%s+]",
              reg_strs[((code >> 4) & 0xf)],
              w_reg_strs[(code & 0xf)]);
    break;
    case IREGINC_REG :
      sprintf(parm_str, "[%s+],%s",
              w_reg_strs[(code & 0x7)],
              reg_strs[((code >> 4) & 0xf)] );
    break;
    case DIRECT_REG :
      sprintf(parm_str, "0x%04x,%s",
              ((code & 0x3) << 8) | get_mem(MEM_ROM, addr+immed_offset),
              reg_strs[((code >> 4) & 0xf)] );
      ++immed_offset;
    break;
    case REG_DIRECT :
      sprintf(parm_str, "%s, @0x%04x",
              reg_strs[((code >> 4) & 0xf)],
              ((code & 0x3) << 8) | get_mem(MEM_ROM, addr+immed_offset) );
      ++immed_offset;
    break;
    case REG_DATA8 :
      sprintf(parm_str, "%s, #%02d",
              b_reg_strs[((code >> 4) & 0xf)],
              get_mem(MEM_ROM, addr+immed_offset) );
      ++immed_offset;
    break;
    case REG_DATA16 :
      sprintf(parm_str, "%s, #%04d",
              reg_strs[((code >> 4) & 0xf)],
              (short)((get_mem(MEM_ROM, addr+immed_offset+1)) |
                     (get_mem(MEM_ROM, addr+immed_offset)<<8)) );
      ++immed_offset;
      ++immed_offset;
    break;
    case IREG_DATA8 :
      sprintf(parm_str, "[%s], 0x%02x",
              w_reg_strs[((code >> 4) & 0x7)],
              get_mem(MEM_ROM, addr+immed_offset) );
      ++immed_offset;
    break;
    case IREG_DATA16 :
      sprintf(parm_str, "[%s], 0x%04x",
              w_reg_strs[((code >> 4) & 0x7)],
              (short)((get_mem(MEM_ROM, addr+immed_offset+1)) |
                     (get_mem(MEM_ROM, addr+immed_offset)<<8)) );
      ++immed_offset;
      ++immed_offset;
    break;
    case IREGINC_DATA8 :
      sprintf(parm_str, "[%s+], 0x%02x",
              w_reg_strs[((code >> 4) & 0x7)],
              get_mem(MEM_ROM, addr+immed_offset) );
      ++immed_offset;
    break;
    case IREGINC_DATA16 :
      sprintf(parm_str, "[%s+], 0x%04x",
              w_reg_strs[((code >> 4) & 0x7)],
              (short)((get_mem(MEM_ROM, addr+immed_offset+1)) |
                     (get_mem(MEM_ROM, addr+immed_offset)<<8)) );
      ++immed_offset;
      ++immed_offset;
    break;
    case IREGOFF8_DATA8 :
      sprintf(parm_str, "[%s+%02d], 0x%02x",
              w_reg_strs[((code >> 4) & 0x7)],
              get_mem(MEM_ROM, addr+immed_offset),
              get_mem(MEM_ROM, addr+immed_offset+1) );
      immed_offset += 2;
    break;
    case IREGOFF8_DATA16 :
      sprintf(parm_str, "[%s+%02d], 0x%04x",
              w_reg_strs[((code >> 4) & 0x7)],
              get_mem(MEM_ROM, addr+immed_offset),
              (short)((get_mem(MEM_ROM, addr+immed_offset+2)) |
                     (get_mem(MEM_ROM, addr+immed_offset+1)<<8)) );
      immed_offset += 3;
    break;
    case IREGOFF16_DATA8 :
      sprintf(parm_str, "[%s+%04d], 0x%02x",
              w_reg_strs[((code >> 4) & 0x7)],
              (short)((get_mem(MEM_ROM, addr+immed_offset+1)) |
                     (get_mem(MEM_ROM, addr+immed_offset+0)<<8)),
              get_mem(MEM_ROM, addr+immed_offset+2) );
      immed_offset += 3;
    break;
    case IREGOFF16_DATA16 :
      sprintf(parm_str, "[%s+%04d], 0x%04x",
              w_reg_strs[((code >> 4) & 0x7)],
              (short)((get_mem(MEM_ROM, addr+immed_offset+1)) |
                     (get_mem(MEM_ROM, addr+immed_offset+0)<<8)),
              (short)((get_mem(MEM_ROM, addr+immed_offset+3)) |
                     (get_mem(MEM_ROM, addr+immed_offset+2)<<8)) );
      immed_offset += 4;
    break;
    case DIRECT_DATA8 :
      sprintf(parm_str, "#%04d 0x%02x",
              ((code & 0x3) << 8) | get_mem(MEM_ROM, addr+immed_offset),
              get_mem(MEM_ROM, addr+immed_offset+1) );
      immed_offset += 3;
    break;
    case DIRECT_DATA16 :
      sprintf(parm_str, "#%04d 0x%04x",
              ((code & 0x3) << 8) | get_mem(MEM_ROM, addr+immed_offset),
              (short)((get_mem(MEM_ROM, addr+immed_offset+2)) |
                     (get_mem(MEM_ROM, addr+immed_offset+1)<<8)) );
      immed_offset += 3;
    break;

// odd-ball ones
    case NO_OPERANDS :  // for NOP
      strcpy(parm_str, "");
    break;
    case C_BIT :
      strcpy(parm_str, "C_BIT");
    break;
    case REG_DATA4 :
      strcpy(parm_str, "REG_DATA4");
    break;
    case IREG_DATA4 :
      strcpy(parm_str, "IREG_DATA4");
    break;
    case IREGINC_DATA4 :
      strcpy(parm_str, "IREGINC_DATA4");
    break;
    case IREGOFF8_DATA4 :
      strcpy(parm_str, "IREGOFF8_DATA4");
    break;
    case IREGOFF16_DATA4 :
      strcpy(parm_str, "IREGOFF16_DATA4");
    break;
    case DIRECT_DATA4 :
      strcpy(parm_str, "DIRECT_DATA4");
    break;

    default:
      strcpy(parm_str, "???");
    break;
  }

  sprintf(work, "%s %s",
          op_mnemonic_str[ mnemonic ],
          parm_str);

  p= strchr(work, ' ');
  if (!p)
    {
      buf= strdup(work);
      return(buf);
    }
  if (sep == NULL)
    buf= (char *)malloc(6+strlen(p)+1);
  else
    buf= (char *)malloc((p-work)+strlen(sep)+strlen(p)+1);
  for (p= work, b= buf; *p != ' '; p++, b++)
    *b= *p;
  p++;
  *b= '\0';
  if (sep == NULL)
    {
      while (strlen(buf) < 6)
        strcat(buf, " ");
    }
  else
    strcat(buf, sep);
  strcat(buf, p);
  return(buf);
}

/*--------------------------------------------------------------------
 print_regs -
|--------------------------------------------------------------------*/
void
cl_xa::print_regs(class cl_console *con)
{
  unsigned char flags;

  flags = get_psw();
  con->dd_printf("CA---VNZ  Flags: %02x ", flags);
  con->dd_printf("R0:%04x R1:%04x R2:%04x R3:%04x\n",
                 get_reg(1,0), get_reg(1,2), get_reg(1,4), get_reg(1,6));

  con->dd_printf("%c%c---%c%c%c            ",
         (flags & BIT_C)?'1':'0',
         (flags & BIT_AC)?'1':'0',
         (flags & BIT_V)?'1':'0',
         (flags & BIT_N)?'1':'0',
         (flags & BIT_Z)?'1':'0');

  con->dd_printf("R4:%04x R5:%04x R6:%04x R7(SP):%04x ES:%04x  DS:%04x\n",
            get_reg(1,8), get_reg(1,10), get_reg(1,12), get_reg(1,14), 0, 0);

  print_disass(PC, con);
}


/*
 * Execution
 */

int
cl_xa::exec_inst(void)
{
  t_mem code1, code2;
  uint code;
  int i;

  if (fetch(&code1))
    return(resBREAKPOINT);
  tick(1);

  if (code1 == 0) // nop, 1 byte instr
    return(inst_NOP(code1));

  if (fetch(&code2))
    return(resBREAKPOINT);
  code = (code1 << 8) | code2;

  i= 0;
  while ((code & disass_xa[i].mask) != disass_xa[i].code &&
         disass_xa[i].mnemonic != BAD_OPCODE)
    i++;

  code |= ((int)(disass_xa[i].operands)) << 16;  // kludgy, tack on operands info
  switch (disass_xa[i].mnemonic)
  {
    case ADD:
    return inst_ADD(code);
    case ADDC:
    return inst_ADDC(code);
    case SUB:
    return inst_SUB(code);
    case SUBB:
    return inst_SUBB(code);
    case CMP:
    return inst_CMP(code);
    case AND:
    return inst_AND(code);
    case OR:
    return inst_OR(code);
    case XOR:
    return inst_XOR(code);
    case ADDS:
    return inst_ADDS(code);
    case NEG:
    return inst_NEG(code);
    case SEXT:
    return inst_SEXT(code);
    case MUL:
    return inst_MUL(code);
    case DIV:
    return inst_DIV(code);
    case DA:
    return inst_DA(code);
    case ASL:
    return inst_ASL(code);
    case ASR:
    return inst_ASR(code);
    case LEA:
    return inst_LEA(code);
    case CPL:
    return inst_CPL(code);
    case LSR:
    return inst_LSR(code);
    case NORM:
    return inst_NORM(code);
    case RL:
    return inst_RL(code);
    case RLC:
    return inst_RLC(code);
    case RR:
    return inst_RR(code);
    case RRC:
    return inst_RRC(code);
    case MOVS:
    return inst_MOVS(code);
    case MOVC:
    return inst_MOVC(code);
    case MOVX:
    return inst_MOVX(code);
    case PUSH:
    return inst_PUSH(code);
    case POP:
    return inst_POP(code);
    case XCH:
    return inst_XCH(code);
    case SETB:
    return inst_SETB(code);
    case CLR:
    return inst_CLR(code);
    case MOV:
    return inst_MOV(code);
    case ANL:
    return inst_ANL(code);
    case ORL:
    return inst_ORL(code);
    case BR:
    return inst_BR(code);
    case JMP:
    return inst_JMP(code);
    case CALL:
    return inst_CALL(code);
    case RET:
    return inst_RET(code);
    case Bcc:
    return inst_Bcc(code);
    case JB:
    return inst_JB(code);
    case JNB:
    return inst_JNB(code);
    case CJNE:
    return inst_CJNE(code);
    case DJNZ:
    return inst_DJNZ(code);
    case JZ:
    return inst_JZ(code);
    case JNZ:
    return inst_JNZ(code);
    case NOP:
    return inst_NOP(code);
    case BKPT:
    return inst_BKPT(code);
    case TRAP:
    return inst_TRAP(code);
    case RESET:
    return inst_RESET(code);
    case BAD_OPCODE:
    default:
    break;
  }

  if (PC)
    PC--;
  else
    PC= get_mem_size(MEM_ROM)-1;
  //tick(-clock_per_cycle());
  sim->stop(resINV_INST);
  return(resINV_INST);
}


/* End of xa.src/xa.cc */
