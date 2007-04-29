/*-------------------------------------------------------------------------
   Register Declarations for the Intel 8052 Processor

   Written By -  Bela Torok / bela.torok@kssg.ch (July 2000)

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

   In other words, you are welcome to use, share and improve this program.
   You are forbidden to forbid anyone else to use, share and improve
   what you give them.   Help stamp out software-hoarding!
-------------------------------------------------------------------------*/

#ifndef REG8052_H
#define REG8052_H

#include <8051.h>     /* load definitions for the 8051 core */

#ifdef REG8051_H
#undef REG8051_H
#endif

/* define 8052 specific registers only */

/* T2CON */
__sfr __at (0xC8) T2CON ;

/* RCAP2 L & H */
__sfr __at (0xCA) RCAP2L  ;
__sfr __at (0xCB) RCAP2H  ;
__sfr __at (0xCC) TL2     ;
__sfr __at (0xCD) TH2     ;

/*  IE  */
__sbit __at (0xAD) ET2    ; /* Enable timer2 interrupt */

/*  IP  */
__sbit __at (0xBD) PT2    ; /* T2 interrupt priority bit */

/* T2CON bits */
__sbit __at (0xC8) T2CON_0 ;
__sbit __at (0xC9) T2CON_1 ;
__sbit __at (0xCA) T2CON_2 ;
__sbit __at (0xCB) T2CON_3 ;
__sbit __at (0xCC) T2CON_4 ;
__sbit __at (0xCD) T2CON_5 ;
__sbit __at (0xCE) T2CON_6 ;
__sbit __at (0xCF) T2CON_7 ;

__sbit __at (0xC8) CP_RL2  ;
__sbit __at (0xC9) C_T2    ;
__sbit __at (0xCA) TR2     ;
__sbit __at (0xCB) EXEN2   ;
__sbit __at (0xCC) TCLK    ;
__sbit __at (0xCD) RCLK    ;
__sbit __at (0xCE) EXF2    ;
__sbit __at (0xCF) TF2     ;

#endif
