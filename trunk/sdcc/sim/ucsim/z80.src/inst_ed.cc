/*
 * Simulator of microcontrollers (inst_ed.cc)
 *   ED escaped multi-byte opcodes for Z80.
 *
 * Copyright (C) 1999,99 Drotos Daniel, Talker Bt.
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

// local
#include "z80cl.h"
#include "regsz80.h"
#include "z80mac.h"


int
cl_z80::inst_ed_(t_mem code)
{
  switch(code) {
  }
  return(resGO);
}

/******** start CB codes *****************/
int
cl_z80::inst_ed(void)
{
  t_mem code;
  unsigned short tw;

  if (fetch(&code))
    return(resBREAKPOINT);

  switch (code)
  {
#if 0
    case 0x40: // IN B,(C)
    return(resGO);
    case 0x41: // OUT (C),B
    return(resGO);
#endif
    case 0x42: // SBC HL,BC
      sbc_HL_wordreg(regs.BC);
    return(resGO);
    case 0x43: // LD (nnnn),BC
      tw = fetch2();
      store2(tw, regs.BC);
    return(resGO);
    case 0x44: // NEG
      regs.F &= ~(BIT_ALL);  /* clear these */
      regs.A -= regs.A;
      regs.F |= BIT_N; /* not addition */
      if (regs.A == 0) regs.F |= BIT_Z;
      if (regs.A & 0x80) regs.F |= BIT_S;
      /* Skip BIT_A for now */
    return(resGO);
    case 0x45: // RETN (return from non-maskable interrupt)
      pop2(PC);
    return(resGO);
#if 0
    case 0x46: // IM 0
      /* interrupt device puts opcode on data bus */
    return(resGO);
#endif
    case 0x47: // LD IV,A
      regs.iv = regs.A;
    return(resGO);

    case 0x48: // IN C,(C)
    return(resGO);
    case 0x49: // OUT (C),C
    return(resGO);

    case 0x4A: // ADC HL,BC
      adc_HL_wordreg(regs.BC);
    return(resGO);
    case 0x4B: // LD BC,(nnnn)
      tw = fetch2();
      regs.BC = get2(tw);
    return(resGO);
    case 0x4D: // RETI (return from interrupt)
      pop2(PC);
    return(resGO);
    case 0x4F: // LD R,A
      /* Load "refresh" register(whats that?) */
    return(resGO);

    case 0x50: // IN D,(C)
    return(resGO);
    case 0x51: // OUT (C),D
    return(resGO);

    case 0x52: // SBC HL,DE
      sbc_HL_wordreg(regs.DE);
    return(resGO);
    case 0x53: // LD (nnnn),DE
      tw = fetch2();
      store2(tw, regs.DE);
    return(resGO);
#if 0
    case 0x56: // IM 1
    return(resGO);
#endif
    case 0x57: // LD A,IV
      regs.A = regs.iv;
    return(resGO);

    case 0x58: // IN E,(C)
    return(resGO);
    case 0x59: // OUT (C),E
    return(resGO);

    case 0x5A: // ADC HL,DE
      adc_HL_wordreg(regs.DE);
    return(resGO);
    case 0x5B: // LD DE,(nnnn)
      tw = fetch2();
      regs.DE = get2(tw);
    return(resGO);

#if 0
    case 0x5E: // IM 2
    return(resGO);
    case 0x5F: // LD A,R
    return(resGO);
    case 0x60: // IN H,(C)
    return(resGO);
    case 0x61: // OUT (C),H
    return(resGO);
#endif
    case 0x62: // SBC HL,HL
      sbc_HL_wordreg(regs.HL);
    return(resGO);
    case 0x63: // LD (nnnn),HL opcode 22 does the same faster
      tw = fetch2();
      store2(tw, regs.HL);
    return(resGO);

#if 0
    case 0x67: // RRD
    return(resGO);
#endif
    case 0x68: // IN L,(C)
    return(resGO);
    case 0x69: // OUT (C),L
    return(resGO);

    case 0x6A: // ADC HL,HL
      adc_HL_wordreg(regs.HL);
    return(resGO);
    case 0x6B: // LD HL,(nnnn) opcode 2A does the same faster
      tw = fetch2();
      regs.HL = get2(tw);
    return(resGO);

#if 0
    case 0x6F: // RLD
      /* rotate 1 bcd digit left between ACC and memory location */
    return(resGO);
#endif

    case 0x70: // IN (C)  set flags only (TSTI)
    return(resGO);
    case 0x71: //  OUT (C),0
    return(resGO);

    case 0x72: // SBC HL,SP
      sbc_HL_wordreg(regs.SP);
    return(resGO);
    case 0x73: // LD (nnnn),SP
      tw = fetch2();
      store2(tw, regs.SP);
    return(resGO);

    case 0x78: // IN A,(C)
    return(resGO);
    case 0x79: // OUT (C),A
    return(resGO);

    case 0x7A: // ADC HL,SP
      adc_HL_wordreg(regs.SP);
    return(resGO);
    case 0x7B: // LD SP,(nnnn)
      tw = fetch2();
      regs.SP = get2(tw);
    return(resGO);

    case 0xA0: // LDI
      // BC - count, sourc=HL, dest=DE.  *DE++ = *HL++, --BC until zero
      regs.F &= ~(BIT_P | BIT_N | BIT_A);  /* clear these */
      store1(regs.DE, get1(regs.HL));
      ++regs.HL;
      ++regs.DE;
      --regs.BC;
      if (regs.BC != 0) regs.F |= BIT_P;
    return(resGO);
    case 0xA1: // CPI
      // compare acc with mem(HL), if ACC=0 set Z flag.  Incr HL, decr BC.
      {
        unsigned char tmp;
        tmp = get1(regs.HL);
        cp_bytereg(tmp);
        ++regs.HL;
        --regs.BC;
        if (regs.BC != 0) regs.F |= BIT_P;
      }
    return(resGO);

    case 0xA2: // INI
    return(resGO);
    case 0xA3: // OUTI
    return(resGO);

    case 0xA8: // LDD
      // BC - count, source=HL, dest=DE.  *DE-- = *HL--, --BC until zero
      regs.F &= ~(BIT_P | BIT_N | BIT_A);  /* clear these */
      store1(regs.DE, get1(regs.HL));
      --regs.HL;
      --regs.DE;
      --regs.BC;
      if (regs.BC != 0) regs.F |= BIT_P;
    return(resGO);
    case 0xA9: // CPD
/* fixme: checkme, compare to other emul. */

      regs.F &= ~(BIT_ALL);  /* clear these */
      if ((regs.A - get1(regs.HL)) == 0) {
        regs.F |= (BIT_Z | BIT_P);
      }
      ++regs.HL;
      --regs.BC;
      if (regs.BC != 0) regs.F |= BIT_P;

    return(resGO);

    case 0xAA: // IND
    return(resGO);
    case 0xAB: // OUTD
    return(resGO);

    case 0xB0: // LDIR
      // BC - count, sourc=HL, dest=DE.  *DE++ = *HL++, --BC until zero
      regs.F &= ~(BIT_P | BIT_N | BIT_A);  /* clear these */
      do {
        store1(regs.DE, get1(regs.HL));
        ++regs.HL;
        ++regs.DE;
        --regs.BC;
      } while (regs.BC != 0);
    return(resGO);

    case 0xB1: // CPIR
/* fixme: checkme, compare to other emul. */
      // compare acc with mem(HL), if ACC=0 set Z flag.  Incr HL, decr BC.
      regs.F &= ~(BIT_ALL);  /* clear these */
      regs.F |= BIT_N | BIT_P;
      do {
        if ((regs.A - get1(regs.HL)) == 0) {
          regs.F |= (BIT_Z | BIT_P);
          return(resGO);
        }
        ++regs.HL;
        --regs.BC;
      } while (regs.BC != 0);

    return(resGO);
#if 0
    case 0xB2: // INIR
    return(resGO);
    case 0xB3: // OTIR
    return(resGO);
#endif
    case 0xB8: // LDDR
      // BC - count, source=HL, dest=DE.  *DE-- = *HL--, --BC until zero
      regs.F &= ~(BIT_P | BIT_N | BIT_A);  /* clear these */
      do {
        store1(regs.DE, get1(regs.HL));
        --regs.HL;
        --regs.DE;
        --regs.BC;
      } while (regs.BC != 0);
    return(resGO);
    case 0xB9: // CPDR
      // compare acc with mem(HL), if ACC=0 set Z flag.  Incr HL, decr BC.
      regs.F &= ~(BIT_ALL);  /* clear these */
      do {
        if ((regs.A - get1(regs.HL)) == 0) {
          regs.F |= (BIT_Z | BIT_P);
          break;
        }
        --regs.HL;
        --regs.BC;
      } while (regs.BC != 0);
    return(resGO);
#if 0
    case 0xBA: // INDR
    return(resGO);
    case 0xBB: // OTDR
    return(resGO);
#endif

    default:
    return(resINV_INST);
  }

  return(resINV_INST);
}

/* End of z80.src/inst_ed.cc */
