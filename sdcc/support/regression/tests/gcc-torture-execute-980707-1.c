/*
   980707-1.c from the execute part of the gcc torture suite.
 */

#include <testfwk.h>

#ifdef __SDCC
#pragma std_c99
#pragma disable_warning 85
#endif

#ifndef __SDCC_mcs51
#include <stdlib.h>
#include <string.h>

char **
buildargv (char *input)
{
  static char *arglist[256];
  int numargs = 0;

  while (1)
    {
      while (*input == ' ')
	input++;
      if (*input == 0)
	break;
      arglist [numargs++] = input;
      while (*input != ' ' && *input != 0)
	input++;
      if (*input == 0)
	break;
      *(input++) = 0;
    }
  arglist [numargs] = NULL;
  return arglist;
}
#endif

void
testTortureExecute (void)
{
#ifndef __SDCC_mcs51
  char **args;
  char input[256];
  int i;

  strcpy(input, " a b");
  args = buildargv(input);

  if (strcmp (args[0], "a"))
    ASSERT (0);
  if (strcmp (args[1], "b"))
    ASSERT (0);
  if (args[2] != NULL)
    ASSERT (0);
  
  return;
#endif
}

