/*
   20000314-3.c from the execute part of the gcc torture tests.
 */

#include <testfwk.h>

#ifdef __SDCC
#pragma std_c99
#endif

static char arg0[] = "arg0";
static char arg1[] = "arg1";

static void attr_rtx		(char *, char *);
static char *attr_string        (char *);
static void attr_eq		(char *, char *);

static void 
attr_rtx (char *varg0, char *varg1)
{
  if (varg0 != arg0)
    ASSERT (0);

  if (varg1 != arg1)
    ASSERT (0);

  return;
}

static void 
attr_eq (char *name, char *value)
{
  return attr_rtx (attr_string (name),
		   attr_string (value));
}

static char *
attr_string (char *str)
{
  return str;
}

void
testTortureExecute (void)
{
  attr_eq (arg0, arg1);
  return;
}

