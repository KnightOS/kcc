/*-------------------------------------------------------------------------
   stddef.h - ANSI functions forward declarations

   Written By -  Maarten Brock / sourceforge.brock@dse.nl (June 2004)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
-------------------------------------------------------------------------*/

#ifndef __SDC51_STDDEF_H
#define __SDC51_STDDEF_H 1

#ifndef NULL
  #define NULL (void *)0
#endif

#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
  typedef unsigned int size_t;
#endif

#if defined(SDCC_z80) || defined(SDCC_gbz80)
  #define offsetof(s,m)   (size_t)&(((s *)0)->m)
#else
  /* temporary hack to fix bug 1518273 */
  #define offsetof(s,m)   (size_t)&(((s __code *)0)->m)
#endif

#endif
