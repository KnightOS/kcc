/* lkihx.c */

/*
 * (C) Copyright 1989-1995
 * All Rights Reserved
 *
 * Alan R. Baldwin
 * 721 Berkeley St.
 * Kent, Ohio  44240
 */

#include <stdio.h>
#include <string.h>
#include "aslink.h"

/*)Module	lkihx.c
 *
 *	The module lkihx.c contains the function to
 *	output the relocated object code in the
 *	Intel Hex format.
 *
 *	lkihx.c contains the following functions:
 *		VOID	hexRecord(addr, rtvalIndex)
 *		VOID	ihx(i)
 *		VOID	ihxEntendedLinearAddress(a)
 *
 *	local variables: hexPageOverrun, lastHexAddr
 */

/*Intel Hex Format
 *      Record Mark Field    -  This  field  signifies  the  start  of a
 *                              record, and consists of an  ascii  colon
 *                              (:).  
 *
 *      Record Length Field  -  This   field   consists   of  two  ascii
 *                              characters which indicate the number  of
 *                              data   bytes   in   this   record.   The
 *                              characters are the result of  converting
 *                              the  number  of  bytes  in binary to two
 *                              ascii characters, high digit first.   An
 *                              End  of  File  record contains two ascii
 *                              zeros in this field.  
 *
 *      Load Address Field   -  This  field  consists  of the four ascii
 *                              characters which result from  converting
 *                              the  the  binary value of the address in
 *                              which to begin loading this record.  The
 *                              order is as follows:  
 *
 *                                  High digit of high byte of address. 
 *                                  Low digit of high byte of address.  
 *                                  High digit of low byte of address.  
 *                                  Low digit of low byte of address.  
 *
 *                              In an End of File record this field con-
 *                              sists of either four ascii zeros or  the
 *                              program  entry  address.   Currently the
 *                              entry address option is not supported.  
 *
 *      Record Type Field    -  This  field  identifies the record type,
 *                              which is either 0 for data records or  1
 *                              for  an End of File record.  It consists
 *                              of two ascii characters, with  the  high
 *                              digit of the record type first, followed
 *                              by the low digit of the record type.  
 *
 *      Data Field           -  This  field consists of the actual data,
 *                              converted to two ascii characters,  high
 *                              digit first.  There are no data bytes in
 *                              the End of File record.  
 *
 *      Checksum Field       -  The  checksum  field is the 8 bit binary
 *                              sum of the record length field, the load
 *                              address  field,  the  record type field,
 *                              and the data field.  This  sum  is  then
 *                              negated  (2's  complement) and converted
 *                              to  two  ascii  characters,  high  digit
 *                              first.  
 */

/* Static variable which holds the count of hex page overruns
 * (crossings of the 64kB boundary). Cleared at explicit extended
 * address output.
 */
static int hexPageOverrun = 0;

/* Global which holds the last (16 bit) address of hex record.
 * Cleared at begin of new area or when the extended address is output.
 */
unsigned int lastHexAddr = 0;


/*)Function	hexRecord(addr, rtvalIndex)
 *
 *		unsigned addr	starting address of hex record
 *		int rtvalIndex	starting index into the rtval[] array
 *
 *	The function hexRecord() outputs the relocated data
 *	in the standard Intel Hex format (with inserting
 *	the extended address record if necessary).
 *
 *	local variables:
 *		Addr_T	chksum		byte checksum
 *		int		i			index for loops
 *		int		overrun		temporary storage for hexPageOverrun
 *		int		bytes		counter for bytes written
 *
 *	global variables:
 *		FILE *	ofp		output file handle
 *		int	rtcnt		count of data words
 *		int	rtflg[]		output the data flag
 *		Addr_T	rtval[]		relocated data
 *
 *	functions called:
 *		int	fprintf()	c_library
 *		ihxEntendedLinearAddress()	lkihx.c
 *		hexRecord()		lkihx.c		(recursion)
 *
 *	side effects:
 *		hexPageOverrun is eventually incremented,
 *		lastHexAddr is updated
 */

VOID
hexRecord(unsigned addr, int rtvalIndex)
{
	Addr_T chksum;
	int i, overrun, bytes;

	for (i = rtvalIndex, chksum = 0; i < rtcnt; i++) {
		if (rtflg[i]) {
			if (addr + ++chksum > 0xffff)
				break;
		}
	}
	if (chksum == 0)
		return;			// nothing to output

	if ( (lastHexAddr > addr) && (rflag) ) {
		overrun = hexPageOverrun + 1;
		ihxEntendedLinearAddress(lastExtendedAddress + overrun);
		hexPageOverrun = overrun;
		hexRecord(addr, rtvalIndex);
		return;
	}

	lastHexAddr = addr;
	fprintf(ofp, ":%02X%04X00", chksum, addr);
	chksum += (addr >> 8) + (addr & 0xff);
	for (i = rtvalIndex, bytes = 0; i < rtcnt; i++) {
		if (rtflg[i]) {
		    fprintf(ofp, "%02X", rtval[i]);
		    chksum += rtval[i];
			if (addr + ++bytes > 0xffff) {
				if (rflag) {
					fprintf(ofp, "%02X\n", (0-chksum) & 0xff);
					overrun = hexPageOverrun + 1;
					ihxEntendedLinearAddress(lastExtendedAddress + overrun);
					hexPageOverrun = overrun;
					hexRecord(0, i + 1);
					return;
				} else {
					fprintf(stderr, 
						"warning: extended linear address encountered; "
						"you probably want the -r flag.\n");
				}
			}
		}
	}
	fprintf(ofp, "%02X\n", (0-chksum) & 0xff);
}

/*)Function	ihx(i)
 *
 *		int	i		0 - process data
 *					1 - end of data
 *
 *	The function ihx() calls the hexRecord() function for processing data
 *	or writes the End of Data record to the file defined by ofp.
 *
 *	local variables:
 *		Addr_T	n		auxiliary variable
 *
 *	global variables:
 *		int	hilo		byte order
 *		FILE *	ofp		output file handle
 *		Addr_T	rtval[]		relocated data
 *
 *	functions called:
 *		VOID hexRecord()	lkihx.c
 *		int	fprintf()		c_library
 *
 *	side effects:
 *		The sequence of rtval[0], rtval[1] is eventually changed.
 */

VOID
ihx(i)
{
	Addr_T n;
	if (i) {
		if (hilo == 0) {
			n = rtval[0];
			rtval[0] = rtval[1];
			rtval[1] = n;
		}
		hexRecord((rtval[0]<<8) + rtval[1], 2);
	} else {
		fprintf(ofp, ":00000001FF\n");
	}
}

/*)Function	newArea(i)
 * The function newArea() is called when processing of new area is started.
 * It resets the value of lastHexAddr.
 */ 

VOID
newArea()
{
	lastHexAddr = 0;
}

/*)Function	ihxEntendedLinearAddress(i)
 *
 *		Addr_T	i		16 bit extended linear address.
 *
 *	The function ihxEntendedLinearAddress() writes an extended
 *	linear address record (type 04) to the output file.
 *
 *	local variables:
 *		Addr_T	chksum		byte checksum
 *
 *	global variables:
 *		FILE *	ofp		output file handle
 *
 *	functions called:
 *		int	fprintf()	c_library
 *
 *	side effects:
 *		The data is output to the file defined by ofp.
 *		hexPageOverrun and lastHexAddr is cleared
 */
VOID
ihxEntendedLinearAddress(Addr_T a)
{
    Addr_T 	chksum;
  
    /* The checksum is the complement of the bytes in the
     * record: the 2 is record length, 4 is the extended linear
     * address record type, plus the two address bytes.
     */ 
    chksum = 2 + 4 + (a & 0xff) + ((a >> 8) & 0xff);    
    
    fprintf(ofp, ":02000004%04X%02X\n", a & 0xffff, (0-chksum) & 0xff);
	hexPageOverrun = 0;
	lastHexAddr = 0;
}
