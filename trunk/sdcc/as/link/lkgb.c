/* lkgb.c

   Copyright (C) 1989-1995 Alan R. Baldwin
   721 Berkeley St., Kent, Ohio 44240

This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 3, or (at your option) any
later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>. */

/*
 * P. Felber
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include "sdld.h"
#include "aslink.h"

/* Value used to fill the unused portions of the image */
/* FFh puts less stress on a EPROM/Flash */
#define FILLVALUE       0xFF

#define CARTSIZE ((unsigned long)nb_rom_banks*16UL*1024UL)
#define NBSEG 8UL
#define SEGSIZE (CARTSIZE/NBSEG)

#define ROMSIZE 0x8000UL
#define BANKSTART 0x4000UL
#define BANKSIZE 0x4000UL

unsigned char *cart[NBSEG];

int current_rom_bank;

VOID gb(int in)
{
  if (TARGET_IS_GB) {
    static int first = 1;
    unsigned long pos, chk;
    int i;
    patch *p;

    if(first) {
      for(i = 0; i < NBSEG; i++) {
        if((cart[i] = malloc(SEGSIZE)) == NULL) {
          fprintf(stderr, "ERROR: can't allocate %dth segment of memory (%d bytes)\n", i, (int)SEGSIZE);
          exit(EXIT_FAILURE);
        }
        memset(cart[i], FILLVALUE, SEGSIZE);
      }
      first = 0;
    }
    if(in) {
      if(rtcnt > 2) {
        if(hilo == 0)
          pos = rtval[0] | (rtval[1]<<8);
        else
          pos = rtval[1] | (rtval[0]<<8);

        /* Perform some validity checks */
        if(pos >= ROMSIZE) {
          fprintf(stderr, "ERROR: address overflow (addr %lx >= %lx)\n", pos, ROMSIZE);
          exit(EXIT_FAILURE);
        }
        if(current_rom_bank >= nb_rom_banks) {
          fprintf(stderr, "ERROR: bank overflow (addr %x > %x)\n", current_rom_bank, nb_rom_banks);
          exit(EXIT_FAILURE);
        }
        if(current_rom_bank > 0 && pos < BANKSTART) {
          fprintf(stderr, "ERROR: address underflow (addr %lx < %lx)\n", pos, BANKSTART);
          exit(EXIT_FAILURE);
        }
        if(nb_rom_banks == 2 && current_rom_bank > 0) {
          fprintf(stderr, "ERROR: only 1 32kB segment with 2 bank\n");
          exit(EXIT_FAILURE);
        }
        if(current_rom_bank > 1)
          pos += (current_rom_bank-1)*BANKSIZE;
        for(i = 2; i < rtcnt; i++) {
          if(rtflg[i]) {
            if(pos < CARTSIZE) {
              if(cart[pos/SEGSIZE][pos%SEGSIZE] != FILLVALUE)
                fprintf(stderr, "WARNING: possibly wrote twice at addr %lx (%02X->%02X)\n", pos, rtval[i], cart[pos/SEGSIZE][pos%SEGSIZE]);
              cart[pos/SEGSIZE][pos%SEGSIZE] = rtval[i];
            } else {
              fprintf(stderr, "ERROR: cartridge size overflow (addr %lx >= %lx)\n", pos, CARTSIZE);
              exit(EXIT_FAILURE);
            }
            pos++;
          }
        }
      }
    } else {
      /* EOF */
      if(cart_name[0] == 0 && linkp->f_idp != NULL) {
        for(i = strlen(linkp->f_idp);
            i > 0 && (isalnum((unsigned char)linkp->f_idp[i-1]) || linkp->f_idp[i-1] == '.');
            i--)
          ;
        for(pos = 0; pos < 16 && linkp->f_idp[i] != '.'; pos++, i++)
          cart_name[pos] = toupper((unsigned char)linkp->f_idp[i]);
        if(pos < 16)
          cart_name[pos] = 0;
      }
      for(pos = 0x0134, i = 0;
          pos < 0x0144 && cart_name[i];
          pos++, i++)
        cart[pos/SEGSIZE][pos%SEGSIZE] = cart_name[i];
      for(; pos < 0x0144; pos++)
        cart[pos/SEGSIZE][pos%SEGSIZE] = 0;
      cart[0x147/SEGSIZE][0x147%SEGSIZE] = mbc_type;
      switch(nb_rom_banks) {
      case 2:
        cart[0x148/SEGSIZE][0x148%SEGSIZE] = 0;
        break;
      case 4:
        cart[0x148/SEGSIZE][0x148%SEGSIZE] = 1;
        break;
      case 8:
        cart[0x148/SEGSIZE][0x148%SEGSIZE] = 2;
        break;
      case 16:
        cart[0x148/SEGSIZE][0x148%SEGSIZE] = 3;
        break;
      case 32:
        cart[0x148/SEGSIZE][0x148%SEGSIZE] = 4;
        break;
      case 64:
        cart[0x148/SEGSIZE][0x148%SEGSIZE] = 5;
        break;
      case 128:
        cart[0x148/SEGSIZE][0x148%SEGSIZE] = 6;
        break;
      case 256:
        cart[0x148/SEGSIZE][0x148%SEGSIZE] = 7;
        break;
      case 512:
        cart[0x148/SEGSIZE][0x148%SEGSIZE] = 8;
        break;
      default:
        fprintf(stderr, "WARNING: unsupported number of ROM banks (%d)\n", nb_rom_banks);
        cart[0x148/SEGSIZE][0x148%SEGSIZE] = 0;
        break;
      }
      switch(nb_ram_banks) {
      case 0:
        cart[0x149/SEGSIZE][0x149%SEGSIZE] = 0;
        break;
      case 1:
        cart[0x149/SEGSIZE][0x149%SEGSIZE] = 2;
        break;
      case 4:
        cart[0x149/SEGSIZE][0x149%SEGSIZE] = 3;
        break;
      case 16:
        cart[0x149/SEGSIZE][0x149%SEGSIZE] = 4;
        break;
      default:
        fprintf(stderr, "WARNING: unsupported number of RAM banks (%d)\n", nb_ram_banks);
        cart[0x149/SEGSIZE][0x149%SEGSIZE] = 0;
        break;
      }

      /* Patch before calculating the checksum */
      if(patches)
        for(p = patches; p; p = p->next)
          cart[p->addr/SEGSIZE][p->addr%SEGSIZE] = p->value;

      /* Update complement checksum */
      chk = 0;
      for(pos = 0x0134; pos < 0x014D; pos++)
        chk += cart[pos/SEGSIZE][pos%SEGSIZE];
      cart[0x014D/SEGSIZE][0x014D%SEGSIZE] = (unsigned char)(0xE7 - (chk&0xFF));
      /* Update checksum */
      chk = 0;
      cart[0x014E/SEGSIZE][0x014E%SEGSIZE] = 0;
      cart[0x014F/SEGSIZE][0x014F%SEGSIZE] = 0;
      for(i = 0; i < NBSEG; i++)
        for(pos = 0; pos < SEGSIZE; pos++)
          chk += cart[i][pos];
      cart[0x014E/SEGSIZE][0x014E%SEGSIZE] = (unsigned char)((chk>>8)&0xFF);
      cart[0x014F/SEGSIZE][0x014F%SEGSIZE] = (unsigned char)(chk&0xFF);

      for(i = 0; i < NBSEG; i++)
        fwrite(cart[i], 1, SEGSIZE, ofp);
    }
  }
  else {
    assert(0);
  }
}

