/* Floating point library in optimized assembly for 8051
 * Copyright (c) 2004, Paul Stoffregen, paul@pjrc.com
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */


#define SDCC_FLOAT_LIB
#include <float.h>


#ifdef FLOAT_ASM_MCS51

// char __fseq (float a, float b)
static void dummy(void) __naked
{
	__asm
	.globl	___fseq
___fseq:
	mov	r7, a
	mov	r0, sp
	dec	r0
	dec	r0
	lcall	fs_check_negative_zeros
	lcall	fs_compare_uint32
	mov	a, r1
	xrl	a, #1
	mov	dpl, a
	ret
	__endasm;
}

#else

/*
** libgcc support for software floating point.
** Copyright (C) 1991 by Pipeline Associates, Inc.  All rights reserved.
** Permission is granted to do *anything* you want with this file,
** commercial or otherwise, provided this message remains intact.  So there!
** I would appreciate receiving any updates/patches/changes that anyone
** makes, and am willing to be the repository for said changes (am I
** making a big mistake?).
**
** Pat Wood
** Pipeline Associates, Inc.
** pipeline!phw@motown.com or
** sun!pipeline!phw or
** uunet!motown!pipeline!phw
*/

/* (c)2000/2001: hacked a little by johan.knol@iduna.nl for sdcc */

union float_long
  {
    float f;
    long l;
  };

/* compare two floats */
char
__fseq (float a1, float a2)
{
  volatile union float_long fl1, fl2;

  fl1.f = a1;
  fl2.f = a2;

  if (fl1.l == fl2.l)
    return (1);
  if (((fl1.l | fl2.l) & 0x7FFFFFFF) == 0)
    return (1);
  return (0);
}

#endif
