
#include <pic18fregs.h>

#include <i2c.h>


void i2c_ack(void)
{
  SSPCON2bits.ACKDT = 0;
  SSPCON2bits.ACKEN = 1;
}
