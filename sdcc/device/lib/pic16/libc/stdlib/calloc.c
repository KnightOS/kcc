/*
 * malloc.c - dynamic memory allocation
 *
 * written by Vangelis Rokas, 2004 (vrokas@otenet.gr)
 *
 */

#include "malloc.h"

extern unsigned char *_dynamicHeap;	/* pointer to heap */

unsigned char *calloc(unsigned char num)	//, unsigned char len)
{
  unsigned char len=num;
  unsigned char total;
  unsigned char *result, *ch;

	total = num * len;
	if(total > MAX_BLOCK_SIZE)return ((unsigned char *)0);
	result = ch = malloc( (char)(total) );
	
	if(result != 0) {
		while(total) {
		  total--;
		  *ch = 0;
		  ch++;
                }
        }
  
  return (result);
}
