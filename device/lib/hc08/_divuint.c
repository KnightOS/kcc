/*-------------------------------------------------------------------------
   _divuint.c - routine for unsigned int (16 bit) division

   Copyright (C) 1999, Jean-Louis Vern <jlvern AT gmail.com>

   This library is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License 
   along with this library; see the file COPYING. If not, write to the
   Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
   MA 02110-1301, USA.

   As a special exception, if you link this library with other files,
   some of which are compiled with SDCC, to produce an executable,
   this library does not by itself cause the resulting executable to
   be covered by the GNU General Public License. This exception does
   not however invalidate any other reasons why the executable file
   might be covered by the GNU General Public License.
-------------------------------------------------------------------------*/

/*   Assembler-functions are provided for:
     mcs51 small
     mcs51 small stack-auto
*/

#if !defined(SDCC_USE_XSTACK) && !defined(_SDCC_NO_ASM_LIB_FUNCS)
#  if defined(SDCC_mcs51)
#    if defined(SDCC_MODEL_SMALL)
#      if defined(SDCC_STACK_AUTO)
#        define _DIVUINT_ASM_SMALL_AUTO
#      else
#        define _DIVUINT_ASM_SMALL
#      endif
#    endif
#  endif
#endif

#if defined _DIVUINT_ASM_SMALL || defined _DIVUINT_ASM_SMALL_AUTO

static void
_divuint_dummy (void) _naked
{
  __asm

    .globl __divuint

  __divuint:

    #define count   r2
    #define reste_l r3
    #define reste_h r4
    #define al      dpl
    #define ah      dph

#if defined(SDCC_STACK_AUTO) && !defined(SDCC_PARMS_IN_BANK1)

    ar0 = 0     ; BUG register set is not considered
    ar1 = 1

    .globl __divint

    mov a,sp
    add a,#-2   ; 2 bytes return address
    mov r0,a    ; r0 points to bh
    mov ar1,@r0   ; load bh
    dec r0
    mov ar0,@r0   ; load bl

    #define bl      r0
    #define bh      r1

  __divint:     ; entry point for __divsint


#else // SDCC_STACK_AUTO

#if !defined(SDCC_PARMS_IN_BANK1)
#if defined(SDCC_NOOVERLAY)
    .area DSEG    (DATA)
#else
    .area OSEG    (OVR,DATA)
#endif

    .globl __divuint_PARM_2
    .globl __divsint_PARM_2

  __divuint_PARM_2:
  __divsint_PARM_2:
    .ds 2

    .area CSEG    (CODE)
#endif // !SDCC_PARMS_IN_BANK1
#if defined(SDCC_PARMS_IN_BANK1)
    #define bl      (b1_0)
    #define bh      (b1_1)
#else
    #define bl      (__divuint_PARM_2)
    #define bh      (__divuint_PARM_2 + 1)
#endif // SDCC_PARMS_IN_BANK1
#endif // SDCC_STACK_AUTO

    mov count,#16
    clr a
    mov reste_l,a
    mov reste_h,a

  loop: mov a,al    ; a <<= 1
    add a,acc
    mov al,a
    mov a,ah
    rlc a
    mov ah,a

    mov a,reste_l ; reste <<= 1
    rlc a   ;   feed in carry
    mov reste_l,a
    mov a,reste_h
    rlc a
    mov reste_h,a

    mov a,reste_l ; reste - b
    subb  a,bl    ; here carry is always clear, because
          ; reste <<= 1 never overflows
    mov b,a
    mov a,reste_h
    subb  a,bh

    jc  smaller   ; reste >= b?

    mov reste_h,a ; -> yes;  reste = reste - b;
    mov reste_l,b
    orl al,#1
  smaller:      ; -> no
    djnz  count,loop
    ret

  __endasm ;
}

#else  // defined _DIVUINT_ASM_SMALL || defined _DIVUINT_ASM_SMALL_AUTO

#define MSB_SET(x) ((x >> (8*sizeof(x)-1)) & 1)

unsigned int
_divuint (unsigned int a, unsigned int b)
{
  unsigned int reste = 0;
  unsigned char count = 16;
  char c;

  do
  {
    // reste: a <- 0;
    c = MSB_SET(a);
    a <<= 1;
    reste <<= 1;
    if (c)
      reste |= 1;

    if (reste >= b)
    {
      reste -= b;
      // a <- (result = 1)
      a |= 1;
    }
  }
  while (--count);
  return a;
}

#endif  // defined _DIVUINT_ASM_SMALL || defined _DIVUINT_ASM_SMALL_AUTO
