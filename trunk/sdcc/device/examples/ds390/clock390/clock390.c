#include <stdio.h>

/* If you don't have an lcd display, don't worry, it will just
   toggle some bits in the void, but ok
*/
#define USE_LCD

void main (void) {
  unsigned long ms, seconds, oldSeconds=-1;
  
  printf ("\nStarting systemclock test.\n");

#ifdef USE_LCD
  LcdInit();
  LcdLPutString(0, "Testing clock");
  LcdLPutString(2, "ms: ");
#endif

  while (1) {
    ms=ClockTicks();
    seconds=ms/1000;

#ifdef USE_LCD
    LcdLPrintf (2 + (4<<8), "%ld", ms);
#endif

    if (seconds!=oldSeconds) {
      printf ("%02d:%02d.%02d\n", (int)seconds/3600, 
	      (int)(seconds/60)%60, 
	      (int)seconds%60);
      oldSeconds=seconds;
      _asm
	cpl P3.5 ; toggle led
      _endasm;
    }
    if (Serial0CharArrived()) {
      switch (getchar()) {
      case '2': CpuSpeed(2); break;
      case '4': CpuSpeed(4); break;
      }
    }
  }
}


