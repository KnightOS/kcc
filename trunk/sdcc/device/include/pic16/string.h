/*-------------------------------------------------------------------------
   string.h - ANSI functions forward declarations    
  
   Modified for pic16 port by Vangelis Rokas, 2004, vrokas@otenet.gr
   
       Written By -  Sandeep Dutta . sandeep.dutta@usa.net (1998)

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
/*
** $Id$
*/

#ifndef __STRING_H	/* { */
#define __STRING_H 1

#define _STRING_SPEC	data

#ifndef NULL
# define NULL (void *)0
#endif

#ifndef _SIZE_T_DEFINED
# define _SIZE_T_DEFINED
  typedef unsigned int size_t;
#endif

extern char *strcat (char *, char *);
extern char *strchr (char *, char);
extern int   strcmp (char *, char *);
extern char *strcpy (char *, char *);
extern int   strcspn(char *, char *);
extern int   strlen (char *);
extern char *strlwr (char *);
extern char *strncat(char *, char *, size_t );
extern int   strncmp(char *, char *, size_t );
extern char *strncpy(char *, char *, size_t );
extern char *strpbrk(char *, char *);
extern char *strrchr(char *, char);
extern int   strspn (char *, char *);
extern char *strstr (char *, char *);
extern char *strtok (char *, char *);
extern char *strupr (char *);

extern void *memccpy(void *, void *, int, size_t);
extern void *memchr(void *, char, size_t);
extern int   memcmp (void *, void *, size_t);
extern void *memcpy (void *, void *, size_t);
extern void *memmove (void *, void *, size_t);
extern void *memrchr(void *, char, size_t);
extern void *memset (_STRING_SPEC void *, unsigned char, size_t );

#endif	/* } */
