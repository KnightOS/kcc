#include "hw.h"

bit b1, b2=1;
code c1=0;
data d1, d2=2;
xdata x1, x2=3;

extern bit be;
extern code ce;
extern data de;
extern xdata xe;
xdata at 0x1234 abs;
extern xdata xee;

void main(void) {
  xe=getchar();
  abs=1;
  putchar('1');
  putchar('2');
  putchar('3');
  putchar('\n');
  puts ("Hello world.\n\r");
  exit_simulator();
}
