/*
 * Simulator of microcontrollers (inst.cc)
 *
 * Copyright (C) 1999,2002 Drotos Daniel, Talker Bt.
 *
 * To contact author send email to drdani@mazsola.iit.uni-miskolc.hu
 * Other contributors include:
 *   Karl Bongers karl@turbobit.com,
 *   Johan Knol 
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

// local
#include "glob.h"
#include "xacl.h"
#include "regsxa.h"

int cl_xa::get_reg(int word_flag, unsigned int index)
{
  //if (index < 3) { /* banked */
  //  if (word_flag)
  //    return get_word_direct(0x400+index);
  //  else
  //    return mem_direct[0x400+index];
  //} else { /* non-banked */
    if (word_flag)
      return get_word_direct(0x400+index);
    else
      return mem_direct[0x400+index];
  //}
}

#define RI_F0 ((code >> 4) & 0xf)
#define RI_70 ((code >> 4) & 0x7)
#define RI_0F (code & 0xf)
#define RI_07 (code & 0x7)

int cl_xa::inst_ADD(uint code, int operands)
{
#undef FUNC1
#define FUNC1 add1
#undef FUNC2
#define FUNC2 add2
#include "inst_gen.cc"

  return(resGO);
}

int cl_xa::inst_ADDC(uint code, int operands)
{
#undef FUNC1
#define FUNC1 addc1
#undef FUNC2
#define FUNC2 addc2
#include "inst_gen.cc"

  return(resGO);
}

int cl_xa::inst_ADDS(uint code, int operands)
{
  return(resGO);
}

int cl_xa::inst_AND(uint code, int operands)
{
#undef FUNC1
#define FUNC1 and1
#undef FUNC2
#define FUNC2 and2
#include "inst_gen.cc"
  return(resGO);
}

int cl_xa::inst_ANL(uint code, int operands)
{
  return(resGO);
}

int cl_xa::inst_ASL(uint code, int operands)
{
  return(resGO);
}

int cl_xa::inst_ASR(uint code, int operands)
{
  return(resGO);
}

int cl_xa::inst_BCC(uint code, int operands)
{
  return(resGO);
}

int cl_xa::inst_BCS(uint code, int operands)
{
  return(resGO);
}

int cl_xa::inst_BEQ(uint code, int operands)
{
  short jmpAddr = fetch1()*2;
  if (get_psw() & BIT_Z) {
    PC=(PC+jmpAddr)&0xfffffffe;
  }
  return(resGO);
}

int cl_xa::inst_BG(uint code, int operands)
{
  return(resGO);
}
int cl_xa::inst_BGE(uint code, int operands)
{
  return(resGO);
}
int cl_xa::inst_BGT(uint code, int operands)
{
  return(resGO);
}
int cl_xa::inst_BKPT(uint code, int operands)
{
  return(resGO);
}
int cl_xa::inst_BL(uint code, int operands)
{
  return(resGO);
}
int cl_xa::inst_BLE(uint code, int operands)
{
  return(resGO);
}
int cl_xa::inst_BLT(uint code, int operands)
{
  return(resGO);
}
int cl_xa::inst_BMI(uint code, int operands)
{
  return(resGO);
}
int cl_xa::inst_BNE(uint code, int operands)
{
  return(resGO);
}
int cl_xa::inst_BNV(uint code, int operands)
{
  return(resGO);
}
int cl_xa::inst_BOV(uint code, int operands)
{
  return(resGO);
}
int cl_xa::inst_BPL(uint code, int operands)
{
  return(resGO);
}

int cl_xa::inst_BR(uint code, int operands)
{
  short jmpAddr = fetch1()*2;
  PC=(PC+jmpAddr)&0xfffffffe;
  return(resGO);
}

int cl_xa::inst_CALL(uint code, int operands)
{
  int jmpaddr;
  unsigned int sp;

  switch(operands) {
    case REL16:
    {
      jmpaddr = (signed short)fetch2();
      sp = get_sp() - 4;
      set_sp(sp);
      store2(sp, PC);
      store2(sp+2, 0);  /* segment(not sure about ordering...) */
      jmpaddr *= 2;
      PC = (PC + jmpaddr) & 0xfffffffe;
    }
    break;
    case IREG:
    {
      sp = get_sp() - 2;
      set_sp(sp);
      store2(sp, PC);
#if 0 // only in huge model
      store2(sp+2, ...
#endif
      jmpaddr = reg2(RI_07);
      jmpaddr *= 2;
      PC = (PC + jmpaddr) & 0xfffffffe;
    }
    break;
    /* fixme 2 more... */ /* johan: which ones? */
  }
  return(resGO);
}

int cl_xa::inst_CJNE(uint code, int operands)
{
  switch(operands) {
    case REG_DIRECT_REL8:
    {
       // update C,N,Z
       if (code & 0x800) {  // word op
         int result;
         int src = get_word_direct( ((code & 0x7)<<4) | fetch1());
         int addr = (fetch1() * 2);
         int dst = reg2(RI_F0);
         unsigned char flags;
         flags = get_psw();
         flags &= ~BIT_ALL; /* clear these bits */
         result = dst - src;
         if (result == 0) flags |= BIT_Z;
         if (result > 0xffff) flags |= BIT_C;
         if (dst < src) flags |= BIT_N;
         set_psw(flags);
         if (flags & BIT_Z)
           PC += addr;
       } else {
         int result;
         int src = get_byte_direct( ((code & 0x7)<<4) | fetch1());
         int addr = (fetch1() * 2);
         int dst = reg1(RI_F0);
         unsigned char flags;
         flags = get_psw();
         flags &= ~BIT_ALL; /* clear these bits */
         result = dst - src;
         if (result == 0) flags |= BIT_Z;
         if (result > 0xff) flags |= BIT_C;
         if (dst < src) flags |= BIT_N;
         set_psw(flags);
         if (flags & BIT_Z)
           PC += addr;
       }
    }
    break;

    case DIRECT_REL8:
    {
       int daddr = ((code & 0x7) << 8) | fetch();
       int addr = fetch() * 2;

       if (code & 0x800) {  // word op
         unsigned short tmp = get_word_direct(daddr)-1;
         set_word_direct(daddr, tmp);
         if (tmp != 0)
           PC += addr;
       } else {
         unsigned char tmp = get_word_direct(daddr)-1;
         set_byte_direct(daddr, tmp);
         if (tmp != 0)
           PC += addr;
       }
    }
    break;
  }
  return(resGO);
}

int cl_xa::inst_CLR(uint code, int operands)
{
  unsigned short bitAddr = (code&0x03 << 8) + fetch();
  // fixme: implement
  bitAddr=bitAddr;
  return(resGO);
}

int cl_xa::inst_CMP(uint code, int operands)
{
#undef FUNC1
#define FUNC1 cmp1
#undef FUNC2
#define FUNC2 cmp2
#include "inst_gen.cc"
  return(resGO);
}
int cl_xa::inst_CPL(uint code, int operands)
{
  return(resGO);
}
int cl_xa::inst_DA(uint code, int operands)
{
  return(resGO);
}
int cl_xa::inst_DIV(uint code, int operands)
{
  return(resGO);
}

int cl_xa::inst_DJNZ(uint code, int operands)
{
  // update N Z flags.
  switch(operands) {
    case REG_REL8:
    {
       int addr = ( ((char)fetch1()) * 2);
       if (code & 0x800) {  // word op
         unsigned short tmp = mov2(0, reg2(RI_F0)-1);
         set_reg2(RI_F0, tmp);
         if (tmp != 0)
           PC = (PC + addr) & 0xfffffffe;
       } else {
         unsigned char tmp = mov1(0, reg1(RI_F0)-1);
         set_reg1(RI_F0, tmp);
         if (tmp != 0)
           PC = (PC + addr) & 0xfffffffe;
       }
    }
    break;

    case DIRECT_REL8:
    {
       int daddr = ((code & 0x7) << 8) | fetch();
       int addr = fetch() * 2;

       if (code & 0x800) {  // word op
         unsigned short tmp = get_word_direct(daddr)-1;
         set_word_direct(daddr, tmp);
         if (tmp != 0)
           PC += addr;
       } else {
         unsigned char tmp = get_word_direct(daddr)-1;
         set_byte_direct(daddr, tmp);
         if (tmp != 0)
           PC += addr;
       }
    }
    break;
  }

  return(resGO);
}

int cl_xa::inst_FCALL(uint code, int operands)
{
  return(resGO);
}

int cl_xa::inst_FJMP(uint code, int operands)
{
  return(resGO);
}

int cl_xa::inst_JB(uint code, int operands)
{
  short bitAddr=((code&0x3)<<8) + fetch1();
  short jmpAddr = (fetch1() * 2);
  if (get_bit(bitAddr)) {
    PC = (PC+jmpAddr)&0xfffffe;
  }
  return(resGO);
}
int cl_xa::inst_JBC(uint code, int operands)
{
  return(resGO);
}
int cl_xa::inst_JNB(uint code, int operands)
{
  short bitAddr=((code&0x3)<<8) + fetch1();
  short jmpAddr = (fetch1() * 2);
  if (!get_bit(bitAddr)) {
    PC = (PC+jmpAddr)&0xfffffe;
  }
  return(resGO);
  return(resGO);
}
int cl_xa::inst_JMP(uint code, int operands)
{
  int jmpAddr;

  switch(operands) {
    case REL16:
    {
      jmpAddr = (signed short)fetch2()*2;
      PC = (PC + jmpAddr) & 0xfffffffe;
    }
    break;
    case IREG:
      PC &= 0xff0000;
      PC |= (reg2(RI_07) & 0xfffe);  /* word aligned */
    break;
    /* fixme 2 more... */
  }
  return(resGO);
}
int cl_xa::inst_JNZ(uint code, int operands)
{
  short saddr = (fetch1() * 2);
  /* reg1(8) = R4L, is ACC for MCS51 compatiblility */
  if (reg1(8)!=0) {
    PC = (PC + saddr) & 0xfffffe;
  }
  return(resGO);
}
int cl_xa::inst_JZ(uint code, int operands)
{
  /* reg1(8) = R4L, is ACC for MCS51 compatiblility */
  short saddr = (fetch1() * 2);
  if (reg1(8)==0) {
      PC += saddr;
  }
  return(resGO);
}
int cl_xa::inst_LEA(uint code, int operands)
{
  return(resGO);
}
int cl_xa::inst_LSR(uint code, int operands)
{
  return(resGO);
}
int cl_xa::inst_MOV(uint code, int operands)
{
#undef FUNC1
#define FUNC1 mov1
#undef FUNC2
#define FUNC2 mov2
#include "inst_gen.cc"
  return(resGO);
}
int cl_xa::inst_MOVC(uint code, int operands)
{
  switch (operands) {
    case REG_IREGINC:
    {
      short srcreg = reg2(RI_07);
      if (code & 0x0800) {  /* word op */
        set_reg2( RI_F0,
                  mov2( reg2(RI_F0),
                        getcode2(srcreg)
                      )
                );
      } else {
        set_reg1( RI_F0,
                  mov1( reg1(RI_F0),
                        getcode1(srcreg)
                      )
                );
      }
      if (operands == REG_IREGINC) {
        set_reg2(RI_07,  srcreg+1);
      }
    }
    break;
    // fixme, 2 more
  }
  return(resGO);
}
int cl_xa::inst_MOVS(uint code, int operands)
{
  return(resGO);
}
int cl_xa::inst_MOVX(uint code, int operands)
{
  return(resGO);
}
int cl_xa::inst_MUL(uint code, int operands)
{
  return(resGO);
}
int cl_xa::inst_NEG(uint code, int operands)
{
  return(resGO);
}
int cl_xa::inst_NOP(uint code, int operands)
{
  return(resGO);
}
int cl_xa::inst_NORM(uint code, int operands)
{
  return(resGO);
}
int cl_xa::inst_OR(uint code, int operands)
{
#undef FUNC1
#define FUNC1 or1
#undef FUNC2
#define FUNC2 or2
#include "inst_gen.cc"
  return(resGO);
}

int cl_xa::inst_ORL(uint code, int operands)
{
  return(resGO);
}

int cl_xa::inst_POP(uint code, int operands)
{
  switch(operands) {
    case DIRECT:
    {
      unsigned short sp;
      unsigned short direct_addr = ((operands & 0x7) << 8) | fetch();

      sp = get_sp();
      if (code & 0x0800) {  /* word op */
        set_word_direct(direct_addr, get2(sp) );
      } else {
        set_byte_direct(direct_addr, get2(sp) & 0xff );
      }
      set_sp(sp+2);
    }
    break;

    case RLIST:
      // fixme: implement
      unsigned char rlist = fetch();
      rlist = rlist; //shutup compiler
    break;
  }
  return(resGO);
}

int cl_xa::inst_PUSH(uint code, int operands)
{
  switch(operands) {
    case DIRECT:
    {
      unsigned short sp;
      unsigned short direct_addr = ((operands & 0x7) << 8) | fetch();

      sp = get_sp()-2;
      set_sp(sp);
      if (code & 0x0800) {  /* word op */
        store2( sp, get_word_direct(direct_addr));
      } else {
        store2( sp, get_byte_direct(direct_addr));
      }
    }
    break;

    case RLIST:
      // fixme: implement
      unsigned char rlist = fetch();
      rlist = rlist; //shutup compiler
    break;
  }
  
  return(resGO);
}
int cl_xa::inst_RESET(uint code, int operands)
{
  return(resGO);
}
int cl_xa::inst_RET(uint code, int operands)
{
  unsigned int retaddr;
  unsigned short sp;
  sp = get_sp();
  retaddr = get2(sp);
#if 0 // only in huge model
  retaddr |= get2(sp+2) << 16;
#endif
  set_sp(sp+2);
  PC = retaddr;
  return(resGO);
}
int cl_xa::inst_RETI(uint code, int operands)
{
  return(resGO);
}
int cl_xa::inst_RL(uint code, int operands)
{
  return(resGO);
}
int cl_xa::inst_RLC(uint code, int operands)
{
  return(resGO);
}
int cl_xa::inst_RR(uint code, int operands)
{
  return(resGO);
}
int cl_xa::inst_RRC(uint code, int operands)
{
  return(resGO);
}
int cl_xa::inst_SETB(uint code, int operands)
{
  unsigned short bitAddr = (code&0x03 << 8) + fetch();
  // fixme: implement
  bitAddr=bitAddr;
  return(resGO);
}

int cl_xa::inst_SEXT(uint code, int operands)
{
  return(resGO);
}

int cl_xa::inst_SUB(uint code, int operands)
{
#undef FUNC1
#define FUNC1 sub1
#undef FUNC2
#define FUNC2 sub2
#include "inst_gen.cc"
  return(resGO);
}

int cl_xa::inst_SUBB(uint code, int operands)
{
#undef FUNC1
#define FUNC1 subb1
#undef FUNC2
#define FUNC2 subb2
#include "inst_gen.cc"
  return(resGO);
}
int cl_xa::inst_TRAP(uint code, int operands)
{
  return(resGO);
}
int cl_xa::inst_XCH(uint code, int operands)
{
  return(resGO);
}
int cl_xa::inst_XOR(uint code, int operands)
{
#undef FUNC1
#define FUNC1 xor1
#undef FUNC2
#define FUNC2 xor2
#include "inst_gen.cc"
  return(resGO);
}

/* End of xa.src/inst.cc */
