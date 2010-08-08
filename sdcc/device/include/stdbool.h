/*-------------------------------------------------------------------------
   stdbool.h - ANSI functions forward declarations

   Copyright (C) 2004, Maarten Brock, sourceforge.brock@dse.nl

   This library is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2.1, or (at your option) any
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

#ifndef __SDC51_STDBOOL_H
#define __SDC51_STDBOOL_H 1

#define true 1
#define false 0

#if defined (SDCC_hc08) || defined (SDCC_pic14) || defined (SDCC_pic16)
 /* The ports that don't have anything worthy of being bool */
 #define BOOL char
 #define __SDCC_WEIRD_BOOL 2
#elif defined (SDCC_ds390) || defined (SDCC_mcs51) || defined (SDCC_xa51)
 /* The ports that have __bit and use it as an imperfect substitute for bool */
 #define _Bool __bit
 #define BOOL  __bit
 #define bool  _Bool
 #define __bool_true_false_are_defined 1
 #define __SDCC_WEIRD_BOOL 1
#else
 /* The ports that have bool */
 #define bool _Bool
 #define BOOL _Bool
 #define __bool_true_false_are_defined 1
#endif

#endif

