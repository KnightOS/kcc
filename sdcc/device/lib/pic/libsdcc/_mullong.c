/* ---------------------------------------------------------------------------
   _mullong.c : routine for 32 bit multiplication

  	Written By	Raphael Neider, rneider@web.de (2005)

   This library is free software; you can redistribute it and/or modify it
   under the terms of the GNU Library General Public License as published by
   the Free Software Foundation; either version 2, or (at your option) any
   later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; if not, write to the Free Software
   Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   In other words, you are welcome to use, share and improve this program.
   You are forbidden to forbid anyone else to use, share and improve
   what you give them.   Help stamp out software-hoarding!

   $Id$
   ------------------------------------------------------------------------ */

#pragma save
#pragma disable_warning 126 /* unreachable code */
#pragma disable_warning 116 /* left shifting more than size of object */
long
_mullong (long a, long b)
{
  long result = 0;
  unsigned char i;

  /* check all bits in a byte */
  for (i = 0; i < 8u; i++) {
    /* check all bytes in operand (generic code, optimized by the compiler) */
    if (a & 0x0001) result += b;
    if (sizeof (long) > 1 && (a & 0x00000100)) result += (b << 8);
    if (sizeof (long) > 2 && (a & 0x00010000)) result += (b << 16);
    if (sizeof (long) > 3 && (a & 0x01000000)) result += (b << 24);
    a = ((unsigned long)a) >> 1;
    b <<= 1;
  } // for i

  return result;
}
#pragma restore
