unsigned char success=0;
unsigned char failures=0;
unsigned char dummy=0;
unsigned bit bit1;

typedef unsigned char byte;

byte d2;

unsigned char uchar0 = 0xa5;

data at 0xa0 unsigned char  uc_bank1_temp=0x42;
data at 0xa2 unsigned int  ui_bank1_temp=0;

void done()
{

  dummy++;

}

void main(void)
{
  dummy = 0;
  ui_bank1_temp = 0;
  uc_bank1_temp = 0;

  bit1 = 0;

  uchar0 = (uchar0<<4) | (uchar0>>4);

  if(uchar0 > 7) {
    dummy = 8;
    uc_bank1_temp = failures;
  }

  if(uc_bank1_temp > 3)
    bit1 = 1;

  success = failures;
  done();
}
