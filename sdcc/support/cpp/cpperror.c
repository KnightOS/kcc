/* Default error handlers for CPP Library.
   Copyright (C) 1986, 87, 89, 92, 93, 94, 1995 Free Software Foundation, Inc.
   Written by Per Bothner, 1994.
   Based on CCCP program by by Paul Rubin, June 1986
   Adapted to ANSI C, Richard Stallman, Jan 1987

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
 what you give them.   Help stamp out software-hoarding!  */

#define EMACS
#define FATAL_EXIT_CODE	33
#ifndef EMACS
#include "config.h"
#endif /* not EMACS */

#include "cpplib.h"
#include <stdio.h>
#include <stdlib.h>

/* Print the file names and line numbers of the #include
   commands which led to the current file.  */

void
cpp_print_containing_files (
     cpp_reader *pfile)
{
  cpp_buffer *ip;
  int first = 1;

  /* If stack of files hasn't changed since we last printed
     this info, don't repeat it.  */
  if (pfile->input_stack_listing_current)
    return;

  ip = cpp_file_buffer (pfile);

  /* Give up if we don't find a source file.  */
  if (ip == NULL)
    return;

  /* Find the other, outer source files.  */
  while ((ip = CPP_PREV_BUFFER (ip)), ip != CPP_NULL_BUFFER (pfile))
    {
      long line, col;
      cpp_buf_line_and_col (ip, &line, &col);
      if (ip->fname != NULL)
	{
	  if (first)
	    {
	      first = 0;
	      fprintf (stderr, "In file included");
	    }
	  else
	    fprintf (stderr, ",\n                ");
	}

      fprintf (stderr, " from %s:%d", ip->nominal_fname, line);
    }
  if (! first)
    fprintf (stderr, ":\n");

  /* Record we have printed the status as of this time.  */
  pfile->input_stack_listing_current = 1;
}

void
cpp_file_line_for_message (
     cpp_reader *pfile,
     char *filename ,
     int line, int column)
{
  if (column > 0)
    fprintf (stderr, "%s:%d:%d: ", filename, line, column);
  else
    fprintf (stderr, "%s:%d: ", filename, line);
}

/* IS_ERROR is 1 for error, 0 for warning */
void cpp_message (
     cpp_reader *pfile,
     int is_error,
     char *msg,
     char *arg1, char *arg2, char *arg3)
{
	if (is_error) {
		pfile->errors++;
		fprintf (stderr,"error:");
	}
	else {
		fprintf (stderr, "warning: ");
	}
  fprintf (stderr, msg, arg1, arg2, arg3);
  fprintf (stderr, "\n");
}

void
fatal (char *str,char *arg)
{
  fprintf (stderr, "%s: ", progname);
  fprintf (stderr, str, arg);
  fprintf (stderr, "\n");
  exit (FATAL_EXIT_CODE);
}


void
cpp_pfatal_with_name (
     cpp_reader *pfile,
     char *name)
{
  cpp_perror_with_name (pfile, name);
#ifdef VMS
  exit (vaxc$errno);
#else
  exit (FATAL_EXIT_CODE);
#endif
}
