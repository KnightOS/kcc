/*-------------------------------------------------------------------------
  _strtok.c - part of string library functions

             Written By -  Sandeep Dutta . sandeep.dutta@usa.net (1999)

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
   
   In other words, you are welcome to use, share and improve this program.
   You are forbidden to forbid anyone else to use, share and improve
   what you give them.   Help stamp out software-hoarding!  
-------------------------------------------------------------------------*/
#include "string.h" 
#define NULL (void *)0

#ifdef SDCC_MODEL_LARGE
#pragma NOINDUCTION
#pragma NOINVARIANT
#endif

char _generic *strtok (
		char    _generic *str ,
		char    _generic *control
		      ) 
{
	static  char _generic   *s ;
	register char _generic *s1 ;

	if ( str )
		s = str ;

	s1 = s ;

	while (*s) {
		if (strchr(control,*s)) {
			*s++ = '\0';
			return s1 ;
		}
		s++ ;
	}
	return (NULL);
}  

