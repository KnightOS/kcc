/** @name makebin - turn a .ihx file into a binary image.
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

typedef unsigned char BYTE;

#define FILL_BYTE 0xFF

int getnibble(char **p)
{
  int ret = *((*p)++) - '0';
  if (ret > 9) {
    ret -= 'A' - '9' - 1;
  }
  return ret;
}

int getbyte(char **p)
{
  return (getnibble(p) << 4) | getnibble(p);
}

int main(int argc, char **argv)
{
    int opt;
    int size = 32768;
    BYTE *rom;
    char line[256];
    char *p;

    while ((opt = getopt(argc, argv, "s:h"))!=-1) {
	switch (opt) {
	case 's':
	    size = atoi(optarg);
	    break;
	case 'h':
	    printf("makebin: convert a Intel IHX file to binary.\n"
		   "Usage: %s [-s romsize] [-h]\n", argv[0]);
	    return 0;
	}
    }
    rom = malloc(size);
    if (rom == NULL) {
	fprintf(stderr, "error: couldn't allocate room for the image.\n");
	return -1;
    }
    memset(rom, FILL_BYTE, size);
    while (fgets(line, 256, stdin) != NULL) {
	int nbytes;
	int addr;

	if (*line != ':') {
	    fprintf(stderr, "error: invalid IHX line.\n");
	    return -2;
	}
	p = line+1;
	nbytes = getbyte(&p);
	addr = getbyte(&p)<<8 | getbyte(&p);
	getbyte(&p);

	while (nbytes--) {
	    if (addr < size)
		rom[addr++] = getbyte(&p);
	}
    }
    fwrite(rom, 1, size, stdout);
    return 0;
}
