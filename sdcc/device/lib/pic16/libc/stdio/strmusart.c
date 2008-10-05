/*-----------------------------------------------------------------
    strmusart.c - usart stream putchar

    Written for pic16 port, by Vangelis Rokas, 2004 (vrokas@otenet.gr)

    Written By - Sandeep Dutta . sandeep.dutta@usa.net (1999)

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by the
    Free Software Foundation; either version 2, or (at your option) any
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

   As a special exception, if you link this library with other files,
   some of which are compiled with SDCC, to produce an executable,
   this library does not by itself cause the resulting executable
   to be covered by the GNU General Public License.
   This exception does not however invalidate any other reasons why
   the executable file might be covered by the GNU General Public License.
-------------------------------------------------------------------------*/

extern WREG;
extern TXREG;
extern TXSTA;

/* note that USART should already been initialized */
void
__stream_usart_putchar (char c) __wparam __naked
{
  (void)c;
  __asm
@1:
    BTFSS       _TXSTA, 1
    BRA         @1
    MOVWF       _TXREG
    RETURN
  __endasm;
}
