/* Test of transcendent float functions.
   Original from Jesus Calvino-Fraga

   func: SQRTF, LOGF, POWF, TANF
*/
#include <testfwk.h>
#include <math.h>

#define {func} 1

void
testTrans(void)
{
#if !defined(SDCC_z80) && !defined(SDCC_hc08) && !PORT_HOST
# ifdef SQRTF
      ASSERT(fabsf (sqrtf (5.0)     -   2.23606801) < 0.00001);
# endif
# ifdef LOGF
    ASSERT(fabsf (logf (124.0)    -   4.82028150) < 0.00001);
    ASSERT(fabsf (log10f (124.0)  -   2.09342169) < 0.00001);
# endif
# ifdef POWF
    /*  too big for small modell */
#   ifndef SDCC_MODEL_SMALL
      ASSERT(fabsf (powf (1.5, 2.0) -   2.24999976) < 0.00001);
#   endif
# endif
# ifdef TANF
    ASSERT(fabsf (tanf (1.6)      - -34.23250579) < 0.00001);
# endif
#endif
}
