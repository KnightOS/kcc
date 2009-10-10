/* aslex.c

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
 * 28-Oct-97 JLH bug in getst(): sign extend on ~(SPACE|ILL)
 *           causes infinite loop
 */

/*
 * Extensions: P. Felber, M. Hope
 */

#include <stdio.h>
#include <setjmp.h>
#include <string.h>
#include "dbuf_string.h"
#include "asxxxx.h"

/*)Module       aslex.c
 *
 *      The module aslex.c includes the general lexical
 *      analysis routines for the assembler.
 *
 *      aslex.c contains the following functions:
 *              char    endline()
 *              char    get()
 *              VOID    getid(id,c)
 *              int     as_getline()
 *              int     getmap()
 *              char    getnb()
 *              VOID    getst()
 *              int     more()
 *              VOID    unget(c)
 *
 *      aslex.c contains no local/static variables
 */

/*)Function     VOID    getid(id,c)
 *
 *              char *  id              a pointer to a string of
 *                                      maximum length NCPS-1
 *              int     c               mode flag
 *                                      >=0     this is first character to
 *                                              copy to the string buffer
 *                                      <0      skip white space, first
 *                                              character must be a LETTER
 *
 *      The function getid() scans the current assembler-source text line
 *      from the current position copying the next LETTER | DIGIT string
 *      into the external string buffer (id).  The string ends when a non
 *	LETTER or DIGIT character is found. The maximum number of characters
 *	copied is NCPS-1.  If the input string is larger than NCPS-1
 *	characters then the string is truncated.  The string is always
 *	NULL terminated.  If the mode argument (c) is >=0 then (c) is
 *	the first character copied to the string buffer, if (c) is <0
 *	then intervening white space (SPACES and TABS) are skipped and
 *	the first character found must be a LETTER else a 'q' error
 *	terminates the parse of this assembler-source text line.
 *
 *      local variables:
 *              char *  p               pointer to external string buffer
 *              int     c               current character value
 *
 *      global variables:
 *              char    ctype[]         a character array which defines the
 *                                      type of character being processed.
 *                                      This index is the character
 *                                      being processed.
 *
 *      called functions:
 *              char    get()           aslex.c
 *              char    getnb()         aslex.c
 *              VOID    unget()         aslex.c
 *
 *      side effects:
 *              use of getnb(), get(), and unget() updates the
 *              global pointer ip, the position in the current
 *              assembler-source text line.
 */

VOID
getid(id, c)
register int c;
char *id;
{
        register char *p;

        if (c < 0) {
                c = getnb();
                if ((ctype[c] & LETTER) == 0)
                        qerr();
        }
        p = id;
        do {
                if (p < &id[NCPS-1])
                        *p++ = c;
        } while (ctype[c=get()] & (LETTER|DIGIT));
        unget(c);
        *p++ = 0;
}

/*)Function     VOID    getst(id,c)
 *
 *              char *  id              a pointer to a string of
 *                                      maximum length NCPS-1
 *              int     c               mode flag
 *                                      >=0     this is first character to
 *                                              copy to the string buffer
 *                                      <0      skip white space, first
 *                                              character must be a LETTER
 *
 *      The function getst() scans the current assembler-source text line
 *      from the current position copying the next character string into
 *      the external string buffer (id).  The string ends when a SPACE or
 *	ILL character is found. The maximum number of characters copied is
 *	NCPS-1.  If the input string is larger than NCPS-1 characters then
 *	the string is truncated.  The string is always NULL terminated.
 *      If the mode argument (c) is >=0 then (c) is the first character
 *      copied to the string buffer, if (c) is <0 then intervening white
 *      space (SPACES and TABS) are skipped and the first character found
 *      must be a LETTER else a 'q' error terminates the parse of this
 *      assembler-source text line.
 *
 *      local variables:
 *              char *  p               pointer to external string buffer
 *              int     c               current character value
 *
 *      global variables:
 *              char    ctype[]         a character array which defines the
 *                                      type of character being processed.
 *                                      This index is the character
 *                                      being processed.
 *
 *      called functions:
 *              char    get()           aslex.c
 *              char    getnb()         aslex.c
 *              VOID    unget()         aslex.c
 *
 *      side effects:
 *              use of getnb(), get(), and unget() updates the
 *              global pointer ip, the position in the current
 *              assembler-source text line.
 */

VOID
getst(id, c)
register int c;
char *id;
{
        register char *p;

        if (c < 0) {
                c = getnb();
                if ((ctype[c] & LETTER) == 0)
                        qerr();
        }
        p = id;
        do {
                if (p < &id[NCPS-1])
                        *p++ = c;
        } while (ctype[c=get()] & ~(SPACE|ILL) & 0xFF);
        unget(c);
        *p++ = 0;
}

/*)Function     char    getnb()
 *
 *      The function getnb() scans the current assembler-source
 *      text line returning the first character not a SPACE or TAB.
 *
 *      local variables:
 *              int     c               current character from
 *                                      assembler-source text line
 *
 *      global variables:
 *              none
 *
 *      called functions:
 *              char    get()           aslex.c
 *
 *      side effects:
 *              use of get() updates the global pointer ip, the position
 *              in the current assembler-source text line
 */

char
getnb()
{
        register int c;

        while ((c=get()) == ' ' || c == '\t')
                ;
        return (c);
}

/*)Function     char    get()
 *
 *      The function get() returns the next character in the
 *      assembler-source text line, at the end of the line a
 *      NULL character is returned.
 *
 *      local variables:
 *              int     c               current character from
 *                                      assembler-source text line
 *
 *      global variables:
 *              char *  ip              pointer into the current
 *                                      assembler-source text line
 *
 *      called functions:
 *              none
 *
 *      side effects:
 *              updates ip to the next character position in the
 *              assembler-source text line.  If ip is at the end of the
 *              line, ip is not updated.
 */

char
get()
{
        register int c;

        if ((c = *ip) != 0)
                ++ip;
        return (c & 0x007F);
}

/*)Function     VOID    unget(c)
 *
 *              int     c               value of last character read from
 *                                      assembler-source text line
 *
 *      If (c) is not a NULL character then the global pointer ip
 *      is updated to point to the preceeding character in the
 *      assembler-source text line.
 *
 *      NOTE:   This function does not push the character (c)
 *              back into the assembler-source text line, only
 *              the pointer ip is changed.
 *
 *      local variables:
 *              int     c               last character read from
 *                                      assembler-source text line
 *
 *      global variables:
 *              char *  ip              position into the current
 *                                      assembler-source text line
 *
 *      called functions:
 *              none
 *
 *      side effects:
 *              ip decremented by 1 character position
 */

VOID
unget(c)
int c;
{
        if (c)
                if (ip != ib)
                        --ip;
}

/*)Function     int     getmap(d)
 *
 *              int     d               value to compare with the
 *                                      assembler-source text line character
 *
 *      The function getmap() converts the 'C' style characters \b, \f,
 *      \n, \r, and \t to their equivalent ascii values and also
 *      converts 'C' style octal constants '\123' to their equivalent
 *      numeric values.  If the first character is equivalent to (d) then
 *      a (-1) is returned, if the end of the line is detected then
 *      a 'q' error terminates the parse for this line, or if the first
 *      character is not a \ then the character value is returned.
 *
 *      local variables:
 *              int     c               value of character from the
 *                                      assembler-source text line
 *              int     n               looping counter
 *              int     v               current value of numeric conversion
 *
 *      global variables:
 *              none
 *
 *      called functions:
 *              char    get()           aslex.c
 *
 *      side effects:
 *              use of get() updates the global pointer ip the position
 *              in the current assembler-source text line
 */

int
getmap(d)
int d;
{
        register int c, n, v;

        if ((c=get()) == '\0')
                qerr();
        if (c == d)
                return (-1);
        if (c == '\\') {
                c = get();
                switch (c) {

                case 'b':
                        c = '\b';
                        break;

                case 'f':
                        c = '\f';
                        break;

                case 'n':
                        c = '\n';
                        break;

                case 'r':
                        c = '\r';
                        break;

                case 't':
                        c = '\t';
                        break;

                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                        n = 0;
                        v = 0;
                        while (++n<=3 && c>='0' && c<='7') {
                                v = (v<<3) + c - '0';
                                c = get();
                        }
                        unget(c);
                        c = v;
                        break;
                }
        }
        return (c);
}

/*)Function	int	comma(flag)
 *
 *		int	flag		when flag is non zero a 'q' error is
 *					generated if a COMMA is not found.
 *
 *	The function comma() skips SPACEs and TABs and returns
 *	a '1' if the next character is a COMMA else a '0' is
 *	returned.  If a COMMA is not found and flag is non zero
 *	then a 'q' error is reported.
 *
 *	local variables:
 *		int	c		last character read from
 *					assembler-source text line
 *
 *	global variables:
 *		none
 *
 *	called functions:
 *		int	getnb()		aslex.c
 *		VOID	qerr()		assubr.c
 *		VOID	unget()		aslex.c
 *
 *	side effects:
 *		assembler-source text line pointer updated
 */

int
comma(flag)
int flag;
{
	int c;

	if ((c = getnb()) != ',') {
		if (flag) {
			qerr();
		} else {
			unget(c);
		}
		return(0);
	}
	return(1);
}

/*)Function     int     as_getline()
 *
 *      The function as_getline() reads a line of assembler-source text
 *      from an assembly source text file or an include file.
 *      Lines of text are processed from assembler-source files until
 *      all files have been read.  If an include file is opened then
 *      lines of text are read from the include file (or nested
 *      include file) until the end of the include file is found.
 *      The input text line is copied into the global string ib[]
 *      and converted to a NULL terminated string.  The function
 *      as_getline() returns a (1) after succesfully reading a line
 *      or a (0) if all files have been read.
 *
 *      local variables:
 *              int     i               string length
 *
 *      global variables:
 *              const char ib[]         string buffer containing
 *                                      assembler-source text line
 *              char    ifp[]           array of file handles for
 *                                      include files
 *              int     incfil          index for ifp[] specifies
 *                                      active include file
 *              int     incline[]       array of include file
 *                                      line numbers
 *              char    sfp[]           array of file handles for
 *                                      assembler source files
 *              int     cfile           index for sfp[] specifies
 *                                      active source file
 *              int     srcline[]       array of source file
 *                                      line numbers
 *              int     inpfil          maximum input file index
 *
 *      called functions:
 *              int     dbuf_init()
 *              int     dbuf_set_length()
 *              int     dbuf_getline()
 *              const char * dbuf_c_str()
 *              int     fclose()        c-library
 *              char *  fgets()         c-library
 *              int     strlen()        c-library
 *
 *      side effects:
 *              include file will be closed at detection of end of file.
 *              the next sequential source file may be selected.
 *              the global file indexes incfil or cfile may be changed.
 *              The respective source line or include line counter
 *              will be updated.
 */

int
as_getline(void)
{
  static struct dbuf_s dbuf;
  static char dbufInitialized = 0;
  size_t len;

  if (!dbufInitialized)
    {
      dbuf_init (&dbuf, 1024);
      dbufInitialized = 1;
    }
  else
    dbuf_set_length (&dbuf, 0);

loop:
  if (incfil >= 0)
    {
      if ((len = dbuf_getline (&dbuf, ifp[incfil])) == 0)
        {
          fclose (ifp[incfil]);
          ifp[incfil--] = NULL;
          lop = NLPP;
          goto loop;
        }
      else
        {
          ++incline[incfil];
        }
    }
  else
    {
      if ((len = dbuf_getline (&dbuf, sfp[cfile])) == 0)
        {
          if (++cfile <= inpfil)
            {
              srcline[cfile] = 0;
              goto loop;
            }
          return 0;
        }
      else
        {
          ++srcline[cfile];
        }
    }
  ib = dbuf_c_str (&dbuf);

  /* remove the trailing NL */
  if (len > 0 && '\n' == ib[len - 1])
    {
      --len;
      if (len > 0 && '\r' == ib[len - 1])
        --len;
      dbuf_set_length (&dbuf, len);
      ib = dbuf_c_str (&dbuf);
    }

  return 1;
}

/*)Function     int     more()
 *
 *      The function more() scans the assembler-source text line
 *      skipping white space (SPACES and TABS) and returns a (0)
 *      if the end of the line or a comment delimeter (;) is found,
 *      or a (1) if their are additional characters in the line.
 *
 *      local variables:
 *              int     c               next character from the
 *                                      assembler-source text line
 *
 *      global variables:
 *              none
 *
 *      called functions:
 *              char    getnb()         aslex.c
 *              VOID    unget()         aslex.c
 *
 *      side effects:
 *              use of getnb() and unget() updates the global pointer ip
 *              the position in the current assembler-source text line
 */

int
more()
{
        register int c;

        c = getnb();
        unget(c);
        return( (c == '\0' || c == ';') ? 0 : 1 );
}

/*)Function     char    endline()
 *
 *      The function endline() scans the assembler-source text line
 *      skipping white space (SPACES and TABS) and returns the next
 *      character or a (0) if the end of the line is found or a
 *      comment delimiter (;) is found.
 *
 *      local variables:
 *              int     c               next character from the
 *                                      assembler-source text line
 *
 *      global variables:
 *              none
 *
 *      called functions:
 *              char    getnb()         aslex.c
 *
 *      side effects:
 *              use of getnb() updates the global pointer ip the
 *              position in the current assembler-source text line
 */

char
endline()
{
        register int c;

        c = getnb();
        return( (c == '\0' || c == ';') ? 0 : c );
}
