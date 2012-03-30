/*
   961122-1.c from the execute part of the gcc torture suite.
 */

#include <testfwk.h>

#ifdef __SDCC
#pragma std_c99
#endif

// TODO: Enable when sdcc supports long long constants!
#if 0
long long acc;

addhi (short a)
{
  acc += (long long) a << 32;
}

subhi (short a)
{
  acc -= (long long) a << 32;
}
#endif

void
testTortureExecute (void)
{
#if 0
  acc = 0xffff00000000ll;
  addhi (1);
  if (acc != 0x1000000000000ll)
    ASSERT (0);
  subhi (1);
  if (acc != 0xffff00000000ll)
    ASSERT (0);
  return;
#endif
}

