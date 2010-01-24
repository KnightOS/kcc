/*
   bug1337835.c
*/

#include <testfwk.h>

#ifdef SDCC
#pragma std_sdcc99
#endif

#include <stdbool.h>

#ifndef BOOL
#define BOOL bool
#endif

char e;

void foo (unsigned long ul, BOOL b, char c) REENTRANT
{
	if (b && (long) ul < 0)
		e = 0;
	if (ul && b && c)
		e = 1;
}

void
test_1337835(void)
{
  e = 2;
  foo(0, 1, 0);
  ASSERT( e == 2 );
}
