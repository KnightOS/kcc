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
  puts ("Hello world.\n\r");
  _asm ;johan _endasm;
  if (d2==2) {
    puts ("d2=2");
  } else {
    puts ("d2!=2");
  }
  if (d1!=3) {
    puts ("d1!=3");
  } else {
    puts ("d1==3");
  }
  exit_simulator();
}
