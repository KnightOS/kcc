/*-------------------------------------------------------------------------
  Register Declarations for 80c552 Processor    
  
       Written By -  Sandeep Dutta . sandeep.dutta@usa.net (1998)

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
   
   In other words, you are welcome to use, share and improve this program.
   You are forbidden to forbid anyone else to use, share and improve
   what you give them.   Help stamp out software-hoarding!  
-------------------------------------------------------------------------*/

#ifndef REG552_H
#define REG552_H

/*  BYTE Register  */
sfr P0    = 0x80;
sfr P1    = 0x90;
sfr P2    = 0xA0;
sfr P3    = 0xB0;
sfr P4    = 0xC0;
sfr P5    = 0xC4;

sfr PSW   = 0xD0;
sfr ACC   = 0xE0;
sfr B     = 0xF0;
sfr SP    = 0x81;
sfr DPL   = 0x82;
sfr DPH   = 0x83;
sfr PCON  = 0x87;
sfr TCON  = 0x88;
sfr TMOD  = 0x89;
sfr TL0   = 0x8A;
sfr TL1   = 0x8B;
sfr TH0   = 0x8C;
sfr TH1   = 0x8D;
sfr IEN0  = 0xA8;
sfr IEN1  = 0xE8;
sfr IP0   = 0xB8;
sfr IP1   = 0xF8;
sfr S0CON = 0x98;
sfr S0BUF = 0x99;
sfr CML0  = 0xA9;
sfr CML1  = 0xAA;
sfr CML2  = 0xAB;
sfr CTL0  = 0xAC;
sfr CTL1  = 0xAD;
sfr CTL2  = 0xAE;
sfr CTL3  = 0xAF;

sfr ADCON = 0xC5;
sfr ADCH  = 0xC6;
sfr TM2IR = 0xC8;
sfr CMH0  = 0xC9;
sfr CMH1  = 0xCA;
sfr CMH2  = 0xCB;
sfr CTH0  = 0xCC;
sfr CTH1  = 0xCD;
sfr CTH2  = 0xCE;
sfr CTH3  = 0xCF;

sfr S1CON  = 0xD8;
sfr S1STA  = 0xD9;
sfr S1DAT  = 0xDA;
sfr S1ADR  = 0xDB;

sfr TM2CON = 0xEA;
sfr CTCON  = 0xEB;
sfr TML2   = 0xEC;
sfr TMH2   = 0xED;
sfr STE    = 0xEE;
sfr RTE    = 0xEF;
sfr PWM0   = 0xFC;
sfr PWM1   = 0xFD;
sfr PWMP   = 0xFE;
sfr T3     = 0xFF;


/*  BIT Register  */
/*  PSW  */
sbit CY    = 0xD7;
sbit AC    = 0xD6;
sbit F0    = 0xD5;
sbit RS1   = 0xD4;
sbit RS0   = 0xD3;
sbit OV    = 0xD2;
sbit P     = 0xD0;

/*  TCON  */
sbit TF1   = 0x8F;
sbit TR1   = 0x8E;
sbit TF0   = 0x8D;
sbit TR0   = 0x8C;
sbit IE1   = 0x8B;
sbit IT1   = 0x8A;
sbit IE0   = 0x89;
sbit IT0   = 0x88;

/*  IEN0  */
sbit EA    = 0xAF;
sbit EAD   = 0xAE;
sbit ES1   = 0xAD;
sbit ES0   = 0xAC;
sbit ET1   = 0xAB;
sbit EX1   = 0xAA;
sbit ET0   = 0xA9;
sbit EX0   = 0xA8;

/*  IEN1  */
sbit ET2   = 0xEF;
sbit ECM2  = 0xEE;
sbit ECM1  = 0xED;
sbit ECM0  = 0xEC;
sbit ECT3  = 0xEB;
sbit ECT2  = 0xEA;
sbit ECT1  = 0xE9;
sbit ECT0  = 0xE8;

/*  IP0 */
sbit PAD   = 0xBE;
sbit PS1   = 0xBD;
sbit PS0   = 0xBC;
sbit PT1   = 0xBB;
sbit PX1   = 0xBA;
sbit PT0   = 0xB9;
sbit PX0   = 0xB8;

/*  IP1 */
sbit PT2   = 0xFF;
sbit PCM2  = 0xFE;
sbit PCM1  = 0xFD;
sbit PCM0  = 0xFC;
sbit PCT3  = 0xFB;
sbit PCT2  = 0xFA;
sbit PCT1  = 0xF9;
sbit PCT0  = 0xF8;

/*  P1  */
sbit SDA   = 0x97;
sbit SCL   = 0x96;
sbit RT2   = 0x95;
sbit T2    = 0x94;
sbit CT3I  = 0x93;
sbit CT2I  = 0x92;
sbit CT1I  = 0x91;
sbit CT0I  = 0x90;

/*  P3  */
sbit RD    = 0xB7;
sbit WR    = 0xB6;
sbit T1    = 0xB5;
sbit T0    = 0xB4;
sbit INT1  = 0xB3;
sbit INT0  = 0xB2;
sbit TXD   = 0xB1;
sbit RXD   = 0xB0;

/*  P4  */
sbit CMT1  = 0xC7;
sbit CMT0  = 0xC6;
sbit CMSR5 = 0xC5;
sbit CMSR4 = 0xC4;
sbit CMSR3 = 0xC3;
sbit CMSR2 = 0xC2;
sbit CMSR1 = 0xC1;
sbit CMSR0 = 0xC0;

/*  S0CON  */
sbit SM0   = 0x9F;
sbit SM1   = 0x9E;
sbit SM2   = 0x9D;
sbit REN   = 0x9C;
sbit TB8   = 0x9B;
sbit RB8   = 0x9A;
sbit TI    = 0x99;
sbit RI    = 0x98;

/*  TM2IR  */
sbit T2OV  = 0xCF;
sbit CMI2  = 0xCE;
sbit CMI1  = 0xCD;
sbit CMI0  = 0xCC;
sbit CTI3  = 0xCB;
sbit CTI2  = 0xCA;
sbit CTI1  = 0xC9;
sbit CTI0  = 0xC8;

/*  S1CON   */
sbit CR0   = 0xD8;
sbit CR1   = 0xD9;
sbit AA    = 0xDA;
sbit SI    = 0xDB;
sbit STO   = 0xDC;
sbit STA   = 0xDD;
sbit ENS1  = 0xDE;

/* T2CON */
sfr at 0xC8 T2CON ;

/* T2CON bits */
sbit at 0xC8 T2CON_0 ;
sbit at 0xC9 T2CON_1 ;
sbit at 0xCA T2CON_2 ;
sbit at 0xCB T2CON_3 ;
sbit at 0xCC T2CON_4 ;
sbit at 0xCD T2CON_5 ;
sbit at 0xCE T2CON_6 ;
sbit at 0xCF T2CON_7 ;

/* RCAP2 L & H */
sfr at 0xCB RCAP2H;
sfr at 0xCA RCAP2L;
#endif
