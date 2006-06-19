/* bug-716242.c

   syntax tests about function pointers at compile time
 */
#include <testfwk.h>

void *p;
int ret;

int mul2 (int i)
{
  return 2 * i;
}

void g (int (*h) (int))
{
  ret = h (2);
}

void f1()
{
#if defined(SDCC_ds390)
  p = (void code *) mul2;
#else
  p = (void *) mul2;
#endif
  g ((int (*) (int)) p);
}

/****************************/

void g (int (*h) (int));

void f2()
{
  int (*fp) (int) = p;

  g (fp);
}

/****************************/

void g (int (*h) (int));

void f3()
{
  int (*fp) (int) = (int (*) (int)) p;

  g (fp);
}

/****************************/

void f4()
{
  ((void (code *) (void)) p) ();
}

/****************************/

void f5()
{
  int (*fp) (int) = mul2;

  fp(1);
}

/****************************/

void f6()
{
  ((void (code *) (void)) 0) ();
}

/****************************/

static void
testFuncPtr(void)
{
  f1();
  ASSERT(ret == 4);
}
