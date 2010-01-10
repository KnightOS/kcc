/* z80ext.c

   Copyright (C) 1989-1995 Alan R. Baldwin
   721 Berkeley St., Kent, Ohio 44240

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>. */

/*
 * Extensions: P. Felber
 */

#include <stdio.h>
#include <setjmp.h>
#include "asxxxx.h"
#include "z80.h"

char	*cpu	= "Zilog Z80 / Hitachi HD64180";
int	hilo	= 0;
char	*dsft	= "asm";
