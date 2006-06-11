
/*
 * adcopen - initialize AD module
 *
 * written by Vangelis Rokas, 2004 <vrokas AT otenet.gr>
 *
 * Devices implemented:
 *	PIC18F[24][45][28]
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/*
** $Id$
*/

#include <pic18fregs.h>

#include <adc.h>


/* parameters are:
 *   channel: one of ADC_CHN_*
 *   fosc:    one of ADC_FOSC_*
 *   pcfg:    one of ADC_CFG_*
 *   config:  ADC_FRM_*  |  ADC_INT_*
 */

void adc_open(unsigned char channel, unsigned char fosc, unsigned char pcfg, unsigned char config)
{
  ADCON0 = 0;
  ADCON1 = 0;

  /* setup channel */
  ADCON0 |= (channel & 0x07) << 3;

  /* setup fosc */
  ADCON0 |= (fosc & 0x03) << 6;
  ADCON1 |= (fosc & 0x04) << 4;
  
  /* setup reference and pins */
  ADCON1 |= pcfg & 0x0f;
  
  ADCON1 |= (config & ADC_FRM_RJUST);
  
  if(config & ADC_INT_ON) {
    PIR1bits.ADIF = 0;
    PIE1bits.ADIE = 1;
    INTCONbits.PEIE = 1;
  }
  
  /* enable the A/D module */
  ADCON0bits.ADON = 1;
}
