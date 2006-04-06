//
// Register Declarations for Microchip 16F689 Processor
//
//
// This header file was automatically generated by:
//
//	inc2h.pl V1.6
//
//	Copyright (c) 2002, Kevin L. Pauba, All Rights Reserved
//
//	SDCC is licensed under the GNU Public license (GPL) v2.  Note that
//	this license covers the code to the compiler and other executables,
//	but explicitly does not cover any code or objects generated by sdcc.
//	We have not yet decided on a license for the run time libraries, but
//	it will not put any requirements on code linked against it. See:
// 
//	http://www.gnu.org/copyleft/gpl/html
//
//	See http://sdcc.sourceforge.net/ for the latest information on sdcc.
//
// 
#ifndef P16F689_H
#define P16F689_H

//
// Register addresses.
//
#define INDF_ADDR	0x0000
#define TMR0_ADDR	0x0001
#define PCL_ADDR	0x0002
#define STATUS_ADDR	0x0003
#define FSR_ADDR	0x0004
#define PORTA_ADDR	0x0005
#define PORTB_ADDR	0x0006
#define PORTC_ADDR	0x0007
#define PCLATH_ADDR	0x000A
#define INTCON_ADDR	0x000B
#define PIR1_ADDR	0x000C
#define PIR2_ADDR	0x000D
#define TMR1L_ADDR	0x000E
#define TMR1H_ADDR	0x000F
#define T1CON_ADDR	0x0010
#define SSPBUF_ADDR	0x0013
#define SSPCON_ADDR	0x0014
#define RCSTA_ADDR	0x0018
#define TXREG_ADDR	0x0019
#define RCREG_ADDR	0x001A
#define ADRESH_ADDR	0x001E
#define ADCON0_ADDR	0x001F
#define OPTION_REG_ADDR	0x0081
#define TRISA_ADDR	0x0085
#define TRISB_ADDR	0x0086
#define TRISC_ADDR	0x0087
#define PIE1_ADDR	0x008C
#define PIE2_ADDR	0x008D
#define PCON_ADDR	0x008E
#define OSCCON_ADDR	0x008F
#define OSCTUNE_ADDR	0x0090
#define SSPADD_ADDR	0x0093
#define MSK_ADDR	0x0093
#define SSPMSK_ADDR	0x0093
#define SSPSTAT_ADDR	0x0094
#define WPU_ADDR	0x0095
#define WPUA_ADDR	0x0095
#define IOC_ADDR	0x0096
#define IOCA_ADDR	0x0096
#define WDTCON_ADDR	0x0097
#define TXSTA_ADDR	0x0098
#define SPBRG_ADDR	0x0099
#define SPBRGH_ADDR	0x009A
#define BAUDCTL_ADDR	0x009B
#define ADRESL_ADDR	0x009E
#define ADCON1_ADDR	0x009F
#define EEDATA_ADDR	0x010C
#define EEADR_ADDR	0x010D
#define EEDATH_ADDR	0x010E
#define EEADRH_ADDR	0x010F
#define WPUB_ADDR	0x0115
#define IOCB_ADDR	0x0116
#define VRCON_ADDR	0x0118
#define CM1CON0_ADDR	0x0119
#define CM2CON0_ADDR	0x011A
#define CM2CON1_ADDR	0x011B
#define ANSEL_ADDR	0x011E
#define ANSELH_ADDR	0x011F
#define EECON1_ADDR	0x018C
#define EECON2_ADDR	0x018D
#define SRCON_ADDR	0x019E

//
// Memory organization.
//

#pragma memmap INDF_ADDR INDF_ADDR SFR 0x000	// INDF
#pragma memmap TMR0_ADDR TMR0_ADDR SFR 0x000	// TMR0
#pragma memmap PCL_ADDR PCL_ADDR SFR 0x000	// PCL
#pragma memmap STATUS_ADDR STATUS_ADDR SFR 0x000	// STATUS
#pragma memmap FSR_ADDR FSR_ADDR SFR 0x000	// FSR
#pragma memmap PORTA_ADDR PORTA_ADDR SFR 0x000	// PORTA
#pragma memmap PORTB_ADDR PORTB_ADDR SFR 0x000	// PORTB
#pragma memmap PORTC_ADDR PORTC_ADDR SFR 0x000	// PORTC
#pragma memmap PCLATH_ADDR PCLATH_ADDR SFR 0x000	// PCLATH
#pragma memmap INTCON_ADDR INTCON_ADDR SFR 0x000	// INTCON
#pragma memmap PIR1_ADDR PIR1_ADDR SFR 0x000	// PIR1
#pragma memmap PIR2_ADDR PIR2_ADDR SFR 0x000	// PIR2
#pragma memmap TMR1L_ADDR TMR1L_ADDR SFR 0x000	// TMR1L
#pragma memmap TMR1H_ADDR TMR1H_ADDR SFR 0x000	// TMR1H
#pragma memmap T1CON_ADDR T1CON_ADDR SFR 0x000	// T1CON
#pragma memmap SSPBUF_ADDR SSPBUF_ADDR SFR 0x000	// SSPBUF
#pragma memmap SSPCON_ADDR SSPCON_ADDR SFR 0x000	// SSPCON
#pragma memmap RCSTA_ADDR RCSTA_ADDR SFR 0x000	// RCSTA
#pragma memmap TXREG_ADDR TXREG_ADDR SFR 0x000	// TXREG
#pragma memmap RCREG_ADDR RCREG_ADDR SFR 0x000	// RCREG
#pragma memmap ADRESH_ADDR ADRESH_ADDR SFR 0x000	// ADRESH
#pragma memmap ADCON0_ADDR ADCON0_ADDR SFR 0x000	// ADCON0
#pragma memmap OPTION_REG_ADDR OPTION_REG_ADDR SFR 0x000	// OPTION_REG
#pragma memmap TRISA_ADDR TRISA_ADDR SFR 0x000	// TRISA
#pragma memmap TRISB_ADDR TRISB_ADDR SFR 0x000	// TRISB
#pragma memmap TRISC_ADDR TRISC_ADDR SFR 0x000	// TRISC
#pragma memmap PIE1_ADDR PIE1_ADDR SFR 0x000	// PIE1
#pragma memmap PIE2_ADDR PIE2_ADDR SFR 0x000	// PIE2
#pragma memmap PCON_ADDR PCON_ADDR SFR 0x000	// PCON
#pragma memmap OSCCON_ADDR OSCCON_ADDR SFR 0x000	// OSCCON
#pragma memmap OSCTUNE_ADDR OSCTUNE_ADDR SFR 0x000	// OSCTUNE
#pragma memmap SSPADD_ADDR SSPADD_ADDR SFR 0x000	// SSPADD
#pragma memmap MSK_ADDR MSK_ADDR SFR 0x000	// MSK
#pragma memmap SSPMSK_ADDR SSPMSK_ADDR SFR 0x000	// SSPMSK
#pragma memmap SSPSTAT_ADDR SSPSTAT_ADDR SFR 0x000	// SSPSTAT
#pragma memmap WPU_ADDR WPU_ADDR SFR 0x000	// WPU
#pragma memmap WPUA_ADDR WPUA_ADDR SFR 0x000	// WPUA
#pragma memmap IOC_ADDR IOC_ADDR SFR 0x000	// IOC
#pragma memmap IOCA_ADDR IOCA_ADDR SFR 0x000	// IOCA
#pragma memmap WDTCON_ADDR WDTCON_ADDR SFR 0x000	// WDTCON
#pragma memmap TXSTA_ADDR TXSTA_ADDR SFR 0x000	// TXSTA
#pragma memmap SPBRG_ADDR SPBRG_ADDR SFR 0x000	// SPBRG
#pragma memmap SPBRGH_ADDR SPBRGH_ADDR SFR 0x000	// SPBRGH
#pragma memmap BAUDCTL_ADDR BAUDCTL_ADDR SFR 0x000	// BAUDCTL
#pragma memmap ADRESL_ADDR ADRESL_ADDR SFR 0x000	// ADRESL
#pragma memmap ADCON1_ADDR ADCON1_ADDR SFR 0x000	// ADCON1
#pragma memmap EEDATA_ADDR EEDATA_ADDR SFR 0x000	// EEDATA
#pragma memmap EEADR_ADDR EEADR_ADDR SFR 0x000	// EEADR
#pragma memmap EEDATH_ADDR EEDATH_ADDR SFR 0x000	// EEDATH
#pragma memmap EEADRH_ADDR EEADRH_ADDR SFR 0x000	// EEADRH
#pragma memmap WPUB_ADDR WPUB_ADDR SFR 0x000	// WPUB
#pragma memmap IOCB_ADDR IOCB_ADDR SFR 0x000	// IOCB
#pragma memmap VRCON_ADDR VRCON_ADDR SFR 0x000	// VRCON
#pragma memmap CM1CON0_ADDR CM1CON0_ADDR SFR 0x000	// CM1CON0
#pragma memmap CM2CON0_ADDR CM2CON0_ADDR SFR 0x000	// CM2CON0
#pragma memmap CM2CON1_ADDR CM2CON1_ADDR SFR 0x000	// CM2CON1
#pragma memmap ANSEL_ADDR ANSEL_ADDR SFR 0x000	// ANSEL
#pragma memmap ANSELH_ADDR ANSELH_ADDR SFR 0x000	// ANSELH
#pragma memmap EECON1_ADDR EECON1_ADDR SFR 0x000	// EECON1
#pragma memmap EECON2_ADDR EECON2_ADDR SFR 0x000	// EECON2
#pragma memmap SRCON_ADDR SRCON_ADDR SFR 0x000	// SRCON


//         LIST
// P16F689.INC  Standard Header File, Version 1.00    Microchip Technology, Inc.
//         NOLIST

// This header file defines configurations, registers, and other useful bits of
// information for the PIC16F689 microcontroller.  These names are taken to match 
// the data sheets as closely as possible.  

// Note that the processor must be selected before this file is 
// included.  The processor may be selected the following ways:

//       1. Command line switch:
//               C:\ MPASM MYFILE.ASM /PIC16F689
//       2. LIST directive in the source file
//               LIST   P=PIC16F689
//       3. Processor Type entry in the MPASM full-screen interface

//==========================================================================
//
//       Revision History
//
//==========================================================================
//1.00   10/12/04 Original
//==========================================================================
//
//       Verify Processor
//
//==========================================================================

//        IFNDEF __16F689
//            MESSG "Processor-header file mismatch.  Verify selected processor."
//         ENDIF

//==========================================================================
//
//       Register Definitions
//
//==========================================================================

#define W                    0x0000
#define F                    0x0001

//----- Register Files------------------------------------------------------

extern data __at (INDF_ADDR) volatile char      INDF;
extern sfr  __at (TMR0_ADDR)                    TMR0;
extern data __at (PCL_ADDR) volatile char       PCL;
extern sfr  __at (STATUS_ADDR)                  STATUS;
extern sfr  __at (FSR_ADDR)                     FSR;
extern sfr  __at (PORTA_ADDR)                   PORTA;
extern sfr  __at (PORTB_ADDR)                   PORTB;
extern sfr  __at (PORTC_ADDR)                   PORTC;

extern sfr  __at (PCLATH_ADDR)                  PCLATH;
extern sfr  __at (INTCON_ADDR)                  INTCON;
extern sfr  __at (PIR1_ADDR)                    PIR1;
extern sfr  __at (PIR2_ADDR)                    PIR2;
extern sfr  __at (TMR1L_ADDR)                   TMR1L;		
extern sfr  __at (TMR1H_ADDR)                   TMR1H;		
extern sfr  __at (T1CON_ADDR)                   T1CON;		


extern sfr  __at (SSPBUF_ADDR)                  SSPBUF;
extern sfr  __at (SSPCON_ADDR)                  SSPCON;


extern sfr  __at (RCSTA_ADDR)                   RCSTA;
extern sfr  __at (TXREG_ADDR)                   TXREG;		
extern sfr  __at (RCREG_ADDR)                   RCREG;

extern sfr  __at (ADRESH_ADDR)                  ADRESH;		
extern sfr  __at (ADCON0_ADDR)                  ADCON0;		


extern sfr  __at (OPTION_REG_ADDR)              OPTION_REG;

extern sfr  __at (TRISA_ADDR)                   TRISA;
extern sfr  __at (TRISB_ADDR)                   TRISB;
extern sfr  __at (TRISC_ADDR)                   TRISC;

extern sfr  __at (PIE1_ADDR)                    PIE1;
extern sfr  __at (PIE2_ADDR)                    PIE2;
extern sfr  __at (PCON_ADDR)                    PCON;
extern sfr  __at (OSCCON_ADDR)                  OSCCON;
extern sfr  __at (OSCTUNE_ADDR)                 OSCTUNE;

extern sfr  __at (SSPADD_ADDR)                  SSPADD;
extern sfr  __at (MSK_ADDR)                     MSK;
extern sfr  __at (SSPMSK_ADDR)                  SSPMSK;
extern sfr  __at (SSPSTAT_ADDR)                 SSPSTAT;
extern sfr  __at (WPU_ADDR)                     WPU;
extern sfr  __at (WPUA_ADDR)                    WPUA;
extern sfr  __at (IOC_ADDR)                     IOC;
extern sfr  __at (IOCA_ADDR)                    IOCA;
extern sfr  __at (WDTCON_ADDR)                  WDTCON;
extern sfr  __at (TXSTA_ADDR)                   TXSTA;
extern sfr  __at (SPBRG_ADDR)                   SPBRG;
extern sfr  __at (SPBRGH_ADDR)                  SPBRGH;	
extern sfr  __at (BAUDCTL_ADDR)                 BAUDCTL;


extern sfr  __at (ADRESL_ADDR)                  ADRESL;		
extern sfr  __at (ADCON1_ADDR)                  ADCON1;



extern sfr  __at (EEDATA_ADDR)                  EEDATA;
extern sfr  __at (EEADR_ADDR)                   EEADR;
extern sfr  __at (EEDATH_ADDR)                  EEDATH;
extern sfr  __at (EEADRH_ADDR)                  EEADRH;


extern sfr  __at (WPUB_ADDR)                    WPUB;
extern sfr  __at (IOCB_ADDR)                    IOCB;

extern sfr  __at (VRCON_ADDR)                   VRCON;
extern sfr  __at (CM1CON0_ADDR)                 CM1CON0;
extern sfr  __at (CM2CON0_ADDR)                 CM2CON0;
extern sfr  __at (CM2CON1_ADDR)                 CM2CON1;

extern sfr  __at (ANSEL_ADDR)                   ANSEL;
extern sfr  __at (ANSELH_ADDR)                  ANSELH;

extern sfr  __at (EECON1_ADDR)                  EECON1;
extern sfr  __at (EECON2_ADDR)                  EECON2;


extern sfr  __at (SRCON_ADDR)                   SRCON;



//----- BANK 0 REGISTER DEFINITIONS ----------------------------------------
//----- STATUS Bits --------------------------------------------------------


//----- INTCON Bits --------------------------------------------------------


//----- PIR1 Bits ----------------------------------------------------------




//----- PIR2 Bits ----------------------------------------------------------


//----- T1CON Bits ---------------------------------------------------------



//----- SSPCON Bits -------------------------------------------------------



//----- RCSTA Bits ---------------------------------------------------------



//----- ADCON0 Bits --------------------------------------------------------


//----- BANK 1 REGISTER DEFINITIONS ----------------------------------------
//----- OPTION Bits --------------------------------------------------------


//----- TRISA Bits --------------------------------------------------------


//----- TRISB Bits --------------------------------------------------------


//----- TRISC Bits --------------------------------------------------------


//----- PIE1 Bits ----------------------------------------------------------




//----- PIE2 Bits ----------------------------------------------------------


//----- PCON Bits ----------------------------------------------------------


//----- OSCCON Bits --------------------------------------------------------


//----- OSCTUNE Bits -------------------------------------------------------


//----- SSPSTAT Bits --------------------------------------------------------


//----- WPUA --------------------------------------------------------------



//----- IOC --------------------------------------------------------------


//----- IOCA --------------------------------------------------------------


//----- WDTCON Bits --------------------------------------------------------


//----- TXSTA Bits -------------------------------------------------------


//----- SPBRG Bits -------------------------------------------------------


//----- SPBRGH Bits -------------------------------------------------------


//----- BAUDCTL Bits -------------------------------------------------------




//----- ADCON1 -------------------------------------------------------------


//----- BANK 2 REGISTER DEFINITIONS ----------------------------------------
//----- WPUB Bits ----------------------------------------------------------


//----- IOCB --------------------------------------------------------------


//----- VRCON Bits ---------------------------------------------------------


//----- CM1CON0 Bits -------------------------------------------------------



//----- CM2CON0 Bits -------------------------------------------------------



//----- CM2CON1 Bits -------------------------------------------------------


//----- ANSEL --------------------------------------------------------------


//----- BANK 3 REGISTER DEFINITIONS ----------------------------------------
//----- EECON1 -------------------------------------------------------------


//----- SRCON ---------------------------------------------------------------


//==========================================================================
//
//       RAM Definition
//
//==========================================================================

//         __MAXRAM H'1FF'
//         __BADRAM H'08'-H'09', H'11'-H'12', H'15'-H'17', H'1B'-H'1D'
//         __BADRAM H'88'-H'89', H'91'-H'92', H'9C'-H'9D'
//         __BADRAM H'108'-H'109', H'110'-H'114', H'117', H'11C'-H'11D'
//         __BADRAM H'188'-H'189', H'18E'-H'19D', H'19F'-H'1EF'

//==========================================================================
//
//       Configuration Bits
//
//==========================================================================

#define _FCMEN_ON            0x3FFF
#define _FCMEN_OFF           0x37FF
#define _IESO_ON             0x3FFF
#define _IESO_OFF            0x3BFF
#define _BOD_ON              0x3FFF
#define _BOD_NSLEEP          0x3EFF
#define _BOD_SBODEN          0x3DFF
#define _BOD_OFF             0x3CFF
#define _CPD_ON              0x3F7F
#define _CPD_OFF             0x3FFF
#define _CP_ON               0x3FBF
#define _CP_OFF              0x3FFF
#define _MCLRE_ON            0x3FFF
#define _MCLRE_OFF           0x3FDF
#define _PWRTE_OFF           0x3FFF
#define _PWRTE_ON            0x3FEF
#define _WDT_ON              0x3FFF
#define _WDT_OFF             0x3FF7
#define _LP_OSC              0x3FF8
#define _XT_OSC              0x3FF9
#define _HS_OSC              0x3FFA
#define _EC_OSC              0x3FFB
#define _INTRC_OSC_NOCLKOUT  0x3FFC
#define _INTRC_OSC_CLKOUT    0x3FFD
#define _EXTRC_OSC_NOCLKOUT  0x3FFE
#define _EXTRC_OSC_CLKOUT    0x3FFF
#define _INTOSCIO            0x3FFC
#define _INTOSC              0x3FFD
#define _EXTRCIO             0x3FFE
#define _EXTRC               0x3FFF

//         LIST

// ----- ADCON0 bits --------------------
typedef union {
  struct {
    unsigned char ADON:1;
    unsigned char GO:1;
    unsigned char CHS0:1;
    unsigned char CHS1:1;
    unsigned char CHS2:1;
    unsigned char CHS3:1;
    unsigned char VCFG:1;
    unsigned char ADFM:1;
  };
  struct {
    unsigned char :1;
    unsigned char NOT_DONE:1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
  };
  struct {
    unsigned char :1;
    unsigned char GO_DONE:1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
  };
} __ADCON0_bits_t;
extern volatile __ADCON0_bits_t __at(ADCON0_ADDR) ADCON0_bits;

#define ADON                 ADCON0_bits.ADON
#define GO                   ADCON0_bits.GO
#define NOT_DONE             ADCON0_bits.NOT_DONE
#define GO_DONE              ADCON0_bits.GO_DONE
#define CHS0                 ADCON0_bits.CHS0
#define CHS1                 ADCON0_bits.CHS1
#define CHS2                 ADCON0_bits.CHS2
#define CHS3                 ADCON0_bits.CHS3
#define VCFG                 ADCON0_bits.VCFG
#define ADFM                 ADCON0_bits.ADFM

// ----- BAUDCTL bits --------------------
typedef union {
  struct {
    unsigned char ABDEN:1;
    unsigned char WUE:1;
    unsigned char :1;
    unsigned char BRG16:1;
    unsigned char CKTXP:1;
    unsigned char ADCS1:1;
    unsigned char RCIDL:1;
    unsigned char ABDOVF:1;
  };
  struct {
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char ADCS0:1;
    unsigned char :1;
    unsigned char ADCS2:1;
    unsigned char :1;
  };
} __BAUDCTL_bits_t;
extern volatile __BAUDCTL_bits_t __at(BAUDCTL_ADDR) BAUDCTL_bits;

#define ABDEN                BAUDCTL_bits.ABDEN
#define WUE                  BAUDCTL_bits.WUE
#define BRG16                BAUDCTL_bits.BRG16
#define CKTXP                BAUDCTL_bits.CKTXP
#define ADCS0                BAUDCTL_bits.ADCS0
#define ADCS1                BAUDCTL_bits.ADCS1
#define RCIDL                BAUDCTL_bits.RCIDL
#define ADCS2                BAUDCTL_bits.ADCS2
#define ABDOVF               BAUDCTL_bits.ABDOVF

// ----- CM1CON0 bits --------------------
typedef union {
  struct {
    unsigned char C1CH0:1;
    unsigned char C1CH1:1;
    unsigned char C1R:1;
    unsigned char :1;
    unsigned char C1POL:1;
    unsigned char C1OE:1;
    unsigned char C1OUT:1;
    unsigned char C1ON:1;
  };
} __CM1CON0_bits_t;
extern volatile __CM1CON0_bits_t __at(CM1CON0_ADDR) CM1CON0_bits;

#define C1CH0                CM1CON0_bits.C1CH0
#define C1CH1                CM1CON0_bits.C1CH1
#define C1R                  CM1CON0_bits.C1R
#define C1POL                CM1CON0_bits.C1POL
#define C1OE                 CM1CON0_bits.C1OE
#define C1OUT                CM1CON0_bits.C1OUT
#define C1ON                 CM1CON0_bits.C1ON

// ----- CM2CON0 bits --------------------
typedef union {
  struct {
    unsigned char C2CH0:1;
    unsigned char C2CH1:1;
    unsigned char C2R:1;
    unsigned char :1;
    unsigned char C2POL:1;
    unsigned char C2OE:1;
    unsigned char C2OUT:1;
    unsigned char C2ON:1;
  };
} __CM2CON0_bits_t;
extern volatile __CM2CON0_bits_t __at(CM2CON0_ADDR) CM2CON0_bits;

#define C2CH0                CM2CON0_bits.C2CH0
#define C2CH1                CM2CON0_bits.C2CH1
#define C2R                  CM2CON0_bits.C2R
#define C2POL                CM2CON0_bits.C2POL
#define C2OE                 CM2CON0_bits.C2OE
#define C2OUT                CM2CON0_bits.C2OUT
#define C2ON                 CM2CON0_bits.C2ON

// ----- CM2CON1 bits --------------------
typedef union {
  struct {
    unsigned char C2SYNC:1;
    unsigned char T1GSS:1;
    unsigned char ANS2:1;
    unsigned char ANS3:1;
    unsigned char ANS4:1;
    unsigned char ANS5:1;
    unsigned char MC2OUT:1;
    unsigned char MC1OUT:1;
  };
  struct {
    unsigned char ANS0:1;
    unsigned char ANS1:1;
    unsigned char WREN:1;
    unsigned char WRERR:1;
    unsigned char C2REN:1;
    unsigned char C1SEN:1;
    unsigned char ANS6:1;
    unsigned char ANS7:1;
  };
  struct {
    unsigned char RD:1;
    unsigned char WR:1;
    unsigned char PULSR:1;
    unsigned char PULSS:1;
    unsigned char :1;
    unsigned char :1;
    unsigned char SR0:1;
    unsigned char EEPGD:1;
  };
  struct {
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char SR1:1;
  };
} __CM2CON1_bits_t;
extern volatile __CM2CON1_bits_t __at(CM2CON1_ADDR) CM2CON1_bits;

#define C2SYNC               CM2CON1_bits.C2SYNC
#define ANS0                 CM2CON1_bits.ANS0
#define RD                   CM2CON1_bits.RD
#define T1GSS                CM2CON1_bits.T1GSS
#define ANS1                 CM2CON1_bits.ANS1
#define WR                   CM2CON1_bits.WR
#define ANS2                 CM2CON1_bits.ANS2
#define WREN                 CM2CON1_bits.WREN
#define PULSR                CM2CON1_bits.PULSR
#define ANS3                 CM2CON1_bits.ANS3
#define WRERR                CM2CON1_bits.WRERR
#define PULSS                CM2CON1_bits.PULSS
#define ANS4                 CM2CON1_bits.ANS4
#define C2REN                CM2CON1_bits.C2REN
#define ANS5                 CM2CON1_bits.ANS5
#define C1SEN                CM2CON1_bits.C1SEN
#define MC2OUT               CM2CON1_bits.MC2OUT
#define ANS6                 CM2CON1_bits.ANS6
#define SR0                  CM2CON1_bits.SR0
#define MC1OUT               CM2CON1_bits.MC1OUT
#define ANS7                 CM2CON1_bits.ANS7
#define EEPGD                CM2CON1_bits.EEPGD
#define SR1                  CM2CON1_bits.SR1

// ----- INTCON bits --------------------
typedef union {
  struct {
    unsigned char RABIF:1;
    unsigned char INTF:1;
    unsigned char T0IF:1;
    unsigned char RABIE:1;
    unsigned char INTE:1;
    unsigned char T0IE:1;
    unsigned char PEIE:1;
    unsigned char GIE:1;
  };
} __INTCON_bits_t;
extern volatile __INTCON_bits_t __at(INTCON_ADDR) INTCON_bits;

#define RABIF                INTCON_bits.RABIF
#define INTF                 INTCON_bits.INTF
#define T0IF                 INTCON_bits.T0IF
#define RABIE                INTCON_bits.RABIE
#define INTE                 INTCON_bits.INTE
#define T0IE                 INTCON_bits.T0IE
#define PEIE                 INTCON_bits.PEIE
#define GIE                  INTCON_bits.GIE

// ----- OPTION_REG bits --------------------
typedef union {
  struct {
    unsigned char PS0:1;
    unsigned char PS1:1;
    unsigned char PS2:1;
    unsigned char PSA:1;
    unsigned char T0SE:1;
    unsigned char T0CS:1;
    unsigned char INTEDG:1;
    unsigned char NOT_RABPU:1;
  };
} __OPTION_REG_bits_t;
extern volatile __OPTION_REG_bits_t __at(OPTION_REG_ADDR) OPTION_REG_bits;

#define PS0                  OPTION_REG_bits.PS0
#define PS1                  OPTION_REG_bits.PS1
#define PS2                  OPTION_REG_bits.PS2
#define PSA                  OPTION_REG_bits.PSA
#define T0SE                 OPTION_REG_bits.T0SE
#define T0CS                 OPTION_REG_bits.T0CS
#define INTEDG               OPTION_REG_bits.INTEDG
#define NOT_RABPU            OPTION_REG_bits.NOT_RABPU

// ----- OSCCON bits --------------------
typedef union {
  struct {
    unsigned char SCS:1;
    unsigned char LTS:1;
    unsigned char HTS:1;
    unsigned char OSTS:1;
    unsigned char IRCF0:1;
    unsigned char IRCF1:1;
    unsigned char IRCF2:1;
    unsigned char :1;
  };
} __OSCCON_bits_t;
extern volatile __OSCCON_bits_t __at(OSCCON_ADDR) OSCCON_bits;

#define SCS                  OSCCON_bits.SCS
#define LTS                  OSCCON_bits.LTS
#define HTS                  OSCCON_bits.HTS
#define OSTS                 OSCCON_bits.OSTS
#define IRCF0                OSCCON_bits.IRCF0
#define IRCF1                OSCCON_bits.IRCF1
#define IRCF2                OSCCON_bits.IRCF2

// ----- OSCTUNE bits --------------------
typedef union {
  struct {
    unsigned char TUN0:1;
    unsigned char TUN1:1;
    unsigned char TUN2:1;
    unsigned char TUN3:1;
    unsigned char TUN4:1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
  };
} __OSCTUNE_bits_t;
extern volatile __OSCTUNE_bits_t __at(OSCTUNE_ADDR) OSCTUNE_bits;

#define TUN0                 OSCTUNE_bits.TUN0
#define TUN1                 OSCTUNE_bits.TUN1
#define TUN2                 OSCTUNE_bits.TUN2
#define TUN3                 OSCTUNE_bits.TUN3
#define TUN4                 OSCTUNE_bits.TUN4

// ----- PCON bits --------------------
typedef union {
  struct {
    unsigned char NOT_BOD:1;
    unsigned char NOT_POR:1;
    unsigned char :1;
    unsigned char :1;
    unsigned char SBODEN:1;
    unsigned char ULPWUE:1;
    unsigned char :1;
    unsigned char :1;
  };
} __PCON_bits_t;
extern volatile __PCON_bits_t __at(PCON_ADDR) PCON_bits;

#define NOT_BOD              PCON_bits.NOT_BOD
#define NOT_POR              PCON_bits.NOT_POR
#define SBODEN               PCON_bits.SBODEN
#define ULPWUE               PCON_bits.ULPWUE

// ----- PIE1 bits --------------------
typedef union {
  struct {
    unsigned char T1IE:1;
    unsigned char :1;
    unsigned char :1;
    unsigned char SSPIE:1;
    unsigned char TXIE:1;
    unsigned char RCIE:1;
    unsigned char ADIE:1;
    unsigned char :1;
  };
  struct {
    unsigned char TMR1IE:1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
  };
} __PIE1_bits_t;
extern volatile __PIE1_bits_t __at(PIE1_ADDR) PIE1_bits;

#define T1IE                 PIE1_bits.T1IE
#define TMR1IE               PIE1_bits.TMR1IE
#define SSPIE                PIE1_bits.SSPIE
#define TXIE                 PIE1_bits.TXIE
#define RCIE                 PIE1_bits.RCIE
#define ADIE                 PIE1_bits.ADIE

// ----- PIE2 bits --------------------
typedef union {
  struct {
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char EEIE:1;
    unsigned char C1IE:1;
    unsigned char C2IE:1;
    unsigned char OSFIE:1;
  };
} __PIE2_bits_t;
extern volatile __PIE2_bits_t __at(PIE2_ADDR) PIE2_bits;

#define EEIE                 PIE2_bits.EEIE
#define C1IE                 PIE2_bits.C1IE
#define C2IE                 PIE2_bits.C2IE
#define OSFIE                PIE2_bits.OSFIE

// ----- PIR1 bits --------------------
typedef union {
  struct {
    unsigned char T1IF:1;
    unsigned char :1;
    unsigned char :1;
    unsigned char SSPIF:1;
    unsigned char TXIF:1;
    unsigned char RCIF:1;
    unsigned char ADIF:1;
    unsigned char :1;
  };
  struct {
    unsigned char TMR1IF:1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
  };
} __PIR1_bits_t;
extern volatile __PIR1_bits_t __at(PIR1_ADDR) PIR1_bits;

#define T1IF                 PIR1_bits.T1IF
#define TMR1IF               PIR1_bits.TMR1IF
#define SSPIF                PIR1_bits.SSPIF
#define TXIF                 PIR1_bits.TXIF
#define RCIF                 PIR1_bits.RCIF
#define ADIF                 PIR1_bits.ADIF

// ----- PIR2 bits --------------------
typedef union {
  struct {
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char EEIF:1;
    unsigned char C1IF:1;
    unsigned char C2IF:1;
    unsigned char OSFIF:1;
  };
} __PIR2_bits_t;
extern volatile __PIR2_bits_t __at(PIR2_ADDR) PIR2_bits;

#define EEIF                 PIR2_bits.EEIF
#define C1IF                 PIR2_bits.C1IF
#define C2IF                 PIR2_bits.C2IF
#define OSFIF                PIR2_bits.OSFIF

// ----- RCSTA bits --------------------
typedef union {
  struct {
    unsigned char RX9D:1;
    unsigned char OERR:1;
    unsigned char FERR:1;
    unsigned char ADDEN:1;
    unsigned char CREN:1;
    unsigned char SREN:1;
    unsigned char RX9:1;
    unsigned char SPEN:1;
  };
} __RCSTA_bits_t;
extern volatile __RCSTA_bits_t __at(RCSTA_ADDR) RCSTA_bits;

#define RX9D                 RCSTA_bits.RX9D
#define OERR                 RCSTA_bits.OERR
#define FERR                 RCSTA_bits.FERR
#define ADDEN                RCSTA_bits.ADDEN
#define CREN                 RCSTA_bits.CREN
#define SREN                 RCSTA_bits.SREN
#define RX9                  RCSTA_bits.RX9
#define SPEN                 RCSTA_bits.SPEN

// ----- SPBRG bits --------------------
typedef union {
  struct {
    unsigned char BRG0:1;
    unsigned char BRG1:1;
    unsigned char BRG2:1;
    unsigned char BRG3:1;
    unsigned char BRG4:1;
    unsigned char BRG5:1;
    unsigned char BRG6:1;
    unsigned char BRG7:1;
  };
} __SPBRG_bits_t;
extern volatile __SPBRG_bits_t __at(SPBRG_ADDR) SPBRG_bits;

#define BRG0                 SPBRG_bits.BRG0
#define BRG1                 SPBRG_bits.BRG1
#define BRG2                 SPBRG_bits.BRG2
#define BRG3                 SPBRG_bits.BRG3
#define BRG4                 SPBRG_bits.BRG4
#define BRG5                 SPBRG_bits.BRG5
#define BRG6                 SPBRG_bits.BRG6
#define BRG7                 SPBRG_bits.BRG7

// ----- SPBRGH bits --------------------
typedef union {
  struct {
    unsigned char BRG8:1;
    unsigned char BRG9:1;
    unsigned char BRG10:1;
    unsigned char BRG11:1;
    unsigned char BRG12:1;
    unsigned char BRG13:1;
    unsigned char BRG14:1;
    unsigned char BRG15:1;
  };
} __SPBRGH_bits_t;
extern volatile __SPBRGH_bits_t __at(SPBRGH_ADDR) SPBRGH_bits;

#define BRG8                 SPBRGH_bits.BRG8
#define BRG9                 SPBRGH_bits.BRG9
#define BRG10                SPBRGH_bits.BRG10
#define BRG11                SPBRGH_bits.BRG11
#define BRG12                SPBRGH_bits.BRG12
#define BRG13                SPBRGH_bits.BRG13
#define BRG14                SPBRGH_bits.BRG14
#define BRG15                SPBRGH_bits.BRG15

// ----- SSPCON bits --------------------
typedef union {
  struct {
    unsigned char SSPM0:1;
    unsigned char SSPM1:1;
    unsigned char SSPM2:1;
    unsigned char SSPM3:1;
    unsigned char CKP:1;
    unsigned char SSPEN:1;
    unsigned char SSPOV:1;
    unsigned char WCOL:1;
  };
} __SSPCON_bits_t;
extern volatile __SSPCON_bits_t __at(SSPCON_ADDR) SSPCON_bits;

#define SSPM0                SSPCON_bits.SSPM0
#define SSPM1                SSPCON_bits.SSPM1
#define SSPM2                SSPCON_bits.SSPM2
#define SSPM3                SSPCON_bits.SSPM3
#define CKP                  SSPCON_bits.CKP
#define SSPEN                SSPCON_bits.SSPEN
#define SSPOV                SSPCON_bits.SSPOV
#define WCOL                 SSPCON_bits.WCOL

// ----- SSPSTAT bits --------------------
typedef union {
  struct {
    unsigned char BF:1;
    unsigned char UA:1;
    unsigned char R_W_NOT:1;
    unsigned char S:1;
    unsigned char P:1;
    unsigned char D_A_NOT:1;
    unsigned char CKE:1;
    unsigned char SMP:1;
  };
  struct {
    unsigned char WPUA0:1;
    unsigned char WPUA1:1;
    unsigned char WPUA2:1;
    unsigned char IOC3:1;
    unsigned char WPUA4:1;
    unsigned char WPUA5:1;
    unsigned char :1;
    unsigned char :1;
  };
  struct {
    unsigned char IOC0:1;
    unsigned char IOC1:1;
    unsigned char IOC2:1;
    unsigned char IOCA3:1;
    unsigned char IOC4:1;
    unsigned char IOC5:1;
    unsigned char :1;
    unsigned char :1;
  };
  struct {
    unsigned char IOCA0:1;
    unsigned char IOCA1:1;
    unsigned char IOCA2:1;
    unsigned char :1;
    unsigned char IOCA4:1;
    unsigned char IOCA5:1;
    unsigned char :1;
    unsigned char :1;
  };
} __SSPSTAT_bits_t;
extern volatile __SSPSTAT_bits_t __at(SSPSTAT_ADDR) SSPSTAT_bits;

#define BF                   SSPSTAT_bits.BF
#define WPUA0                SSPSTAT_bits.WPUA0
#define IOC0                 SSPSTAT_bits.IOC0
#define IOCA0                SSPSTAT_bits.IOCA0
#define UA                   SSPSTAT_bits.UA
#define WPUA1                SSPSTAT_bits.WPUA1
#define IOC1                 SSPSTAT_bits.IOC1
#define IOCA1                SSPSTAT_bits.IOCA1
#define R_W_NOT              SSPSTAT_bits.R_W_NOT
#define WPUA2                SSPSTAT_bits.WPUA2
#define IOC2                 SSPSTAT_bits.IOC2
#define IOCA2                SSPSTAT_bits.IOCA2
#define S                    SSPSTAT_bits.S
#define IOC3                 SSPSTAT_bits.IOC3
#define IOCA3                SSPSTAT_bits.IOCA3
#define P                    SSPSTAT_bits.P
#define WPUA4                SSPSTAT_bits.WPUA4
#define IOC4                 SSPSTAT_bits.IOC4
#define IOCA4                SSPSTAT_bits.IOCA4
#define D_A_NOT              SSPSTAT_bits.D_A_NOT
#define WPUA5                SSPSTAT_bits.WPUA5
#define IOC5                 SSPSTAT_bits.IOC5
#define IOCA5                SSPSTAT_bits.IOCA5
#define CKE                  SSPSTAT_bits.CKE
#define SMP                  SSPSTAT_bits.SMP

// ----- STATUS bits --------------------
typedef union {
  struct {
    unsigned char C:1;
    unsigned char DC:1;
    unsigned char Z:1;
    unsigned char NOT_PD:1;
    unsigned char NOT_TO:1;
    unsigned char RP0:1;
    unsigned char RP1:1;
    unsigned char IRP:1;
  };
} __STATUS_bits_t;
extern volatile __STATUS_bits_t __at(STATUS_ADDR) STATUS_bits;

#define C                    STATUS_bits.C
#define DC                   STATUS_bits.DC
#define Z                    STATUS_bits.Z
#define NOT_PD               STATUS_bits.NOT_PD
#define NOT_TO               STATUS_bits.NOT_TO
#define RP0                  STATUS_bits.RP0
#define RP1                  STATUS_bits.RP1
#define IRP                  STATUS_bits.IRP

// ----- T1CON bits --------------------
typedef union {
  struct {
    unsigned char TMR1ON:1;
    unsigned char TMR1CS:1;
    unsigned char NOT_T1SYNC:1;
    unsigned char T1OSCEN:1;
    unsigned char T1CKPS0:1;
    unsigned char T1CKPS1:1;
    unsigned char TMR1GE:1;
    unsigned char T1GINV:1;
  };
} __T1CON_bits_t;
extern volatile __T1CON_bits_t __at(T1CON_ADDR) T1CON_bits;

#define TMR1ON               T1CON_bits.TMR1ON
#define TMR1CS               T1CON_bits.TMR1CS
#define NOT_T1SYNC           T1CON_bits.NOT_T1SYNC
#define T1OSCEN              T1CON_bits.T1OSCEN
#define T1CKPS0              T1CON_bits.T1CKPS0
#define T1CKPS1              T1CON_bits.T1CKPS1
#define TMR1GE               T1CON_bits.TMR1GE
#define T1GINV               T1CON_bits.T1GINV

// ----- TRISA bits --------------------
typedef union {
  struct {
    unsigned char TRISA0:1;
    unsigned char TRISA1:1;
    unsigned char TRISA2:1;
    unsigned char TRISA3:1;
    unsigned char TRISA4:1;
    unsigned char TRISA5:1;
    unsigned char :1;
    unsigned char :1;
  };
} __TRISA_bits_t;
extern volatile __TRISA_bits_t __at(TRISA_ADDR) TRISA_bits;

#define TRISA0               TRISA_bits.TRISA0
#define TRISA1               TRISA_bits.TRISA1
#define TRISA2               TRISA_bits.TRISA2
#define TRISA3               TRISA_bits.TRISA3
#define TRISA4               TRISA_bits.TRISA4
#define TRISA5               TRISA_bits.TRISA5

// ----- TRISB bits --------------------
typedef union {
  struct {
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char TRISB4:1;
    unsigned char TRISB5:1;
    unsigned char TRISB6:1;
    unsigned char TRISB7:1;
  };
} __TRISB_bits_t;
extern volatile __TRISB_bits_t __at(TRISB_ADDR) TRISB_bits;

#define TRISB4               TRISB_bits.TRISB4
#define TRISB5               TRISB_bits.TRISB5
#define TRISB6               TRISB_bits.TRISB6
#define TRISB7               TRISB_bits.TRISB7

// ----- TRISC bits --------------------
typedef union {
  struct {
    unsigned char TRISC0:1;
    unsigned char TRISC1:1;
    unsigned char TRISC2:1;
    unsigned char TRISC3:1;
    unsigned char TRISC4:1;
    unsigned char TRISC5:1;
    unsigned char TRISC6:1;
    unsigned char TRISC7:1;
  };
} __TRISC_bits_t;
extern volatile __TRISC_bits_t __at(TRISC_ADDR) TRISC_bits;

#define TRISC0               TRISC_bits.TRISC0
#define TRISC1               TRISC_bits.TRISC1
#define TRISC2               TRISC_bits.TRISC2
#define TRISC3               TRISC_bits.TRISC3
#define TRISC4               TRISC_bits.TRISC4
#define TRISC5               TRISC_bits.TRISC5
#define TRISC6               TRISC_bits.TRISC6
#define TRISC7               TRISC_bits.TRISC7

// ----- TXSTA bits --------------------
typedef union {
  struct {
    unsigned char TX9D:1;
    unsigned char TRMT:1;
    unsigned char BRGH:1;
    unsigned char SENB:1;
    unsigned char SYNC:1;
    unsigned char TXEN:1;
    unsigned char TX9:1;
    unsigned char CSRC:1;
  };
} __TXSTA_bits_t;
extern volatile __TXSTA_bits_t __at(TXSTA_ADDR) TXSTA_bits;

#define TX9D                 TXSTA_bits.TX9D
#define TRMT                 TXSTA_bits.TRMT
#define BRGH                 TXSTA_bits.BRGH
#define SENB                 TXSTA_bits.SENB
#define SYNC                 TXSTA_bits.SYNC
#define TXEN                 TXSTA_bits.TXEN
#define TX9                  TXSTA_bits.TX9
#define CSRC                 TXSTA_bits.CSRC

// ----- VRCON bits --------------------
typedef union {
  struct {
    unsigned char VR0:1;
    unsigned char VR1:1;
    unsigned char VR2:1;
    unsigned char VR3:1;
    unsigned char VP6EN:1;
    unsigned char VRR:1;
    unsigned char C2VREN:1;
    unsigned char C1VREN:1;
  };
} __VRCON_bits_t;
extern volatile __VRCON_bits_t __at(VRCON_ADDR) VRCON_bits;

#define VR0                  VRCON_bits.VR0
#define VR1                  VRCON_bits.VR1
#define VR2                  VRCON_bits.VR2
#define VR3                  VRCON_bits.VR3
#define VP6EN                VRCON_bits.VP6EN
#define VRR                  VRCON_bits.VRR
#define C2VREN               VRCON_bits.C2VREN
#define C1VREN               VRCON_bits.C1VREN

// ----- WDTCON bits --------------------
typedef union {
  struct {
    unsigned char SWDTEN:1;
    unsigned char WDTPS0:1;
    unsigned char WDTPS1:1;
    unsigned char WDTPS2:1;
    unsigned char WDTPS3:1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
  };
} __WDTCON_bits_t;
extern volatile __WDTCON_bits_t __at(WDTCON_ADDR) WDTCON_bits;

#define SWDTEN               WDTCON_bits.SWDTEN
#define WDTPS0               WDTCON_bits.WDTPS0
#define WDTPS1               WDTCON_bits.WDTPS1
#define WDTPS2               WDTCON_bits.WDTPS2
#define WDTPS3               WDTCON_bits.WDTPS3

// ----- WPUB bits --------------------
typedef union {
  struct {
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char WPUB4:1;
    unsigned char WPUB5:1;
    unsigned char WPUB6:1;
    unsigned char WPUB7:1;
  };
  struct {
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char IOCB4:1;
    unsigned char IOCB5:1;
    unsigned char IOCB6:1;
    unsigned char IOCB7:1;
  };
} __WPUB_bits_t;
extern volatile __WPUB_bits_t __at(WPUB_ADDR) WPUB_bits;

#define WPUB4                WPUB_bits.WPUB4
#define IOCB4                WPUB_bits.IOCB4
#define WPUB5                WPUB_bits.WPUB5
#define IOCB5                WPUB_bits.IOCB5
#define WPUB6                WPUB_bits.WPUB6
#define IOCB6                WPUB_bits.IOCB6
#define WPUB7                WPUB_bits.WPUB7
#define IOCB7                WPUB_bits.IOCB7

#endif
