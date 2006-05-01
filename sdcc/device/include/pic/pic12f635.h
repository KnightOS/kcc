//
// Register Declarations for Microchip 12F635 Processor
//
//
// This header file was automatically generated by:
//
//	inc2h.pl V1.7
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
#ifndef P12F635_H
#define P12F635_H

//
// Register addresses.
//
#define INDF_ADDR	0x0000
#define TMR0_ADDR	0x0001
#define PCL_ADDR	0x0002
#define STATUS_ADDR	0x0003
#define FSR_ADDR	0x0004
#define PORTA_ADDR	0x0005
#define GPIO_ADDR	0x0005
#define PCLATH_ADDR	0x000A
#define INTCON_ADDR	0x000B
#define PIR1_ADDR	0x000C
#define TMR1L_ADDR	0x000E
#define TMR1H_ADDR	0x000F
#define T1CON_ADDR	0x0010
#define WDTCON_ADDR	0x0018
#define CMCON0_ADDR	0x0019
#define CMCON1_ADDR	0x001A
#define OPTION_REG_ADDR	0x0081
#define TRISA_ADDR	0x0085
#define TRISIO_ADDR	0x0085
#define PIE1_ADDR	0x008C
#define PCON_ADDR	0x008E
#define OSCCON_ADDR	0x008F
#define OSCTUNE_ADDR	0x0090
#define LVDCON_ADDR	0x0094
#define WPUDA_ADDR	0x0095
#define IOCA_ADDR	0x0096
#define WDA_ADDR	0x0097
#define VRCON_ADDR	0x0099
#define EEDAT_ADDR	0x009A
#define EEDATA_ADDR	0x009A
#define EEADR_ADDR	0x009B
#define EECON1_ADDR	0x009C
#define EECON2_ADDR	0x009D
#define CRCON_ADDR	0x0110
#define CRDAT0_ADDR	0x0111
#define CRDAT1_ADDR	0x0112
#define CRDAT2_ADDR	0x0113
#define CRDAT3_ADDR	0x0114

//
// Memory organization.
//

#pragma memmap INDF_ADDR INDF_ADDR SFR 0x000	// INDF
#pragma memmap TMR0_ADDR TMR0_ADDR SFR 0x000	// TMR0
#pragma memmap PCL_ADDR PCL_ADDR SFR 0x000	// PCL
#pragma memmap STATUS_ADDR STATUS_ADDR SFR 0x000	// STATUS
#pragma memmap FSR_ADDR FSR_ADDR SFR 0x000	// FSR
#pragma memmap PORTA_ADDR PORTA_ADDR SFR 0x000	// PORTA
#pragma memmap GPIO_ADDR GPIO_ADDR SFR 0x000	// GPIO
#pragma memmap PCLATH_ADDR PCLATH_ADDR SFR 0x000	// PCLATH
#pragma memmap INTCON_ADDR INTCON_ADDR SFR 0x000	// INTCON
#pragma memmap PIR1_ADDR PIR1_ADDR SFR 0x000	// PIR1
#pragma memmap TMR1L_ADDR TMR1L_ADDR SFR 0x000	// TMR1L
#pragma memmap TMR1H_ADDR TMR1H_ADDR SFR 0x000	// TMR1H
#pragma memmap T1CON_ADDR T1CON_ADDR SFR 0x000	// T1CON
#pragma memmap WDTCON_ADDR WDTCON_ADDR SFR 0x000	// WDTCON
#pragma memmap CMCON0_ADDR CMCON0_ADDR SFR 0x000	// CMCON0
#pragma memmap CMCON1_ADDR CMCON1_ADDR SFR 0x000	// CMCON1
#pragma memmap OPTION_REG_ADDR OPTION_REG_ADDR SFR 0x000	// OPTION_REG
#pragma memmap TRISA_ADDR TRISA_ADDR SFR 0x000	// TRISA
#pragma memmap TRISIO_ADDR TRISIO_ADDR SFR 0x000	// TRISIO
#pragma memmap PIE1_ADDR PIE1_ADDR SFR 0x000	// PIE1
#pragma memmap PCON_ADDR PCON_ADDR SFR 0x000	// PCON
#pragma memmap OSCCON_ADDR OSCCON_ADDR SFR 0x000	// OSCCON
#pragma memmap OSCTUNE_ADDR OSCTUNE_ADDR SFR 0x000	// OSCTUNE
#pragma memmap LVDCON_ADDR LVDCON_ADDR SFR 0x000	// LVDCON
#pragma memmap WPUDA_ADDR WPUDA_ADDR SFR 0x000	// WPUDA
#pragma memmap IOCA_ADDR IOCA_ADDR SFR 0x000	// IOCA
#pragma memmap WDA_ADDR WDA_ADDR SFR 0x000	// WDA
#pragma memmap VRCON_ADDR VRCON_ADDR SFR 0x000	// VRCON
#pragma memmap EEDAT_ADDR EEDAT_ADDR SFR 0x000	// EEDAT
#pragma memmap EEDATA_ADDR EEDATA_ADDR SFR 0x000	// EEDATA
#pragma memmap EEADR_ADDR EEADR_ADDR SFR 0x000	// EEADR
#pragma memmap EECON1_ADDR EECON1_ADDR SFR 0x000	// EECON1
#pragma memmap EECON2_ADDR EECON2_ADDR SFR 0x000	// EECON2
#pragma memmap CRCON_ADDR CRCON_ADDR SFR 0x000	// CRCON
#pragma memmap CRDAT0_ADDR CRDAT0_ADDR SFR 0x000	// CRDAT0
#pragma memmap CRDAT1_ADDR CRDAT1_ADDR SFR 0x000	// CRDAT1
#pragma memmap CRDAT2_ADDR CRDAT2_ADDR SFR 0x000	// CRDAT2
#pragma memmap CRDAT3_ADDR CRDAT3_ADDR SFR 0x000	// CRDAT3


//         LIST
// P12F635.INC  Standard Header File, Version 1.00    Microchip Technology, Inc.
//         NOLIST

// This header file defines configurations, registers, and other useful bits of
// information for the PIC12F635 microcontroller.  These names are taken to match 
// the data sheets as closely as possible.  

// Note that the processor must be selected before this file is 
// included.  The processor may be selected the following ways:

//       1. Command line switch:
//               C:\ MPASM MYFILE.ASM /PIC12F635
//       2. LIST directive in the source file
//               LIST   P=PIC12F635
//       3. Processor Type entry in the MPASM full-screen interface

//==========================================================================
//
//       Revision History
//
//==========================================================================
//1.00   12/07/03 Original
//1.10   04/19/04 Release to match first revision datasheet --kjd
//1.20	06/07/04 Update and correct badram definitions  --kjd
//==========================================================================
//
//       Verify Processor
//
//==========================================================================

//        IFNDEF __12F635
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
//Bank 0
extern __data __at (INDF_ADDR) volatile char      INDF;
extern __sfr  __at (TMR0_ADDR)                    TMR0;
extern __data __at (PCL_ADDR) volatile char       PCL;
extern __sfr  __at (STATUS_ADDR)                  STATUS;
extern __sfr  __at (FSR_ADDR)                     FSR;
extern __sfr  __at (PORTA_ADDR)                   PORTA;
extern __sfr  __at (GPIO_ADDR)                    GPIO;

extern __sfr  __at (PCLATH_ADDR)                  PCLATH;
extern __sfr  __at (INTCON_ADDR)                  INTCON;
extern __sfr  __at (PIR1_ADDR)                    PIR1;

extern __sfr  __at (TMR1L_ADDR)                   TMR1L;		
extern __sfr  __at (TMR1H_ADDR)                   TMR1H;		
extern __sfr  __at (T1CON_ADDR)                   T1CON;		

extern __sfr  __at (WDTCON_ADDR)                  WDTCON;
extern __sfr  __at (CMCON0_ADDR)                  CMCON0;		
extern __sfr  __at (CMCON1_ADDR)                  CMCON1;		

//Bank 1
extern __sfr  __at (OPTION_REG_ADDR)              OPTION_REG;

extern __sfr  __at (TRISA_ADDR)                   TRISA;
extern __sfr  __at (TRISIO_ADDR)                  TRISIO;

extern __sfr  __at (PIE1_ADDR)                    PIE1;

extern __sfr  __at (PCON_ADDR)                    PCON;
extern __sfr  __at (OSCCON_ADDR)                  OSCCON;
extern __sfr  __at (OSCTUNE_ADDR)                 OSCTUNE;

extern __sfr  __at (LVDCON_ADDR)                  LVDCON;
extern __sfr  __at (WPUDA_ADDR)                   WPUDA;
extern __sfr  __at (IOCA_ADDR)                    IOCA;
extern __sfr  __at (WDA_ADDR)                     WDA;

extern __sfr  __at (VRCON_ADDR)                   VRCON;
extern __sfr  __at (EEDAT_ADDR)                   EEDAT;	
extern __sfr  __at (EEDATA_ADDR)                  EEDATA;	
extern __sfr  __at (EEADR_ADDR)                   EEADR;	
extern __sfr  __at (EECON1_ADDR)                  EECON1;
extern __sfr  __at (EECON2_ADDR)                  EECON2;

//Bank 2
extern __sfr  __at (CRCON_ADDR)                   CRCON;
extern __sfr  __at (CRDAT0_ADDR)                  CRDAT0;
extern __sfr  __at (CRDAT1_ADDR)                  CRDAT1;
extern __sfr  __at (CRDAT2_ADDR)                  CRDAT2;
extern __sfr  __at (CRDAT3_ADDR)                  CRDAT3;

//----- STATUS Bits --------------------------------------------------------


//----- INTCON Bits --------------------------------------------------------


//----- PIR1 Bits ----------------------------------------------------------


//----- T1CON Bits ---------------------------------------------------------


//----- WDTCON Bits --------------------------------------------------------


//----- CMCON0 Bits -------------------------------------------------------


//----- CMCON1 Bits -------------------------------------------------------


//----- OPTION Bits --------------------------------------------------------


//----- PIE1 Bits ----------------------------------------------------------


//----- PCON Bits ----------------------------------------------------------


//----- OSCCON Bits --------------------------------------------------------


//----- OSCTUNE Bits -------------------------------------------------------


//----- IOCA --------------------------------------------------------------


//----- EECON1 -------------------------------------------------------------


//----- VRCON ---------------------------------------------------------



//-----  CRCON -------------------------------------------------------------


//-----  LVDCON -------------------------------------------------------------


//----- WDA    -------------------------------------------------------------


//----- WPUDA    -------------------------------------------------------------


//----- PORTA    -------------------------------------------------------------


//----- GPIO    -------------------------------------------------------------


//==========================================================================
//
//       RAM Definition
//
//==========================================================================

//         __MAXRAM H'1FF'
//         __BADRAM H'06'-H'09', H'0D', H'11'-H'17', H'1B'-H'1F', H'20'-H'3F'
//         __BADRAM H'86'-H'89', H'8D', H'91'-H'93', H'98', H'9E'-H'9F', H'A0'-H'EF'
// 		__BADRAM H'10C'-H'10F', H'115'-H'16F', H'106'-H'109', H'186'-H'189'
// 		__BADRAM H'18C'-H'1EF'

//==========================================================================
//
//       Configuration Bits
//
//==========================================================================
#define _WUREN_ON            0x2FFF
#define _WUREN_OFF           0x3FFF
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

//         LIST

// ----- CMCON0 bits --------------------
typedef union {
  struct {
    unsigned char CM0:1;
    unsigned char CM1:1;
    unsigned char CM2:1;
    unsigned char CIS:1;
    unsigned char C1INV:1;
    unsigned char :1;
    unsigned char C1OUT:1;
    unsigned char :1;
  };
} __CMCON0_bits_t;
extern volatile __CMCON0_bits_t __at(CMCON0_ADDR) CMCON0_bits;

#define CM0                  CMCON0_bits.CM0
#define CM1                  CMCON0_bits.CM1
#define CM2                  CMCON0_bits.CM2
#define CIS                  CMCON0_bits.CIS
#define C1INV                CMCON0_bits.C1INV
#define C1OUT                CMCON0_bits.C1OUT

// ----- CMCON1 bits --------------------
typedef union {
  struct {
    unsigned char C1SYNC:1;
    unsigned char T1GSS:1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
  };
} __CMCON1_bits_t;
extern volatile __CMCON1_bits_t __at(CMCON1_ADDR) CMCON1_bits;

#define C1SYNC               CMCON1_bits.C1SYNC
#define T1GSS                CMCON1_bits.T1GSS

// ----- INTCON bits --------------------
typedef union {
  struct {
    unsigned char RAIF:1;
    unsigned char INTF:1;
    unsigned char T0IF:1;
    unsigned char RAIE:1;
    unsigned char INTE:1;
    unsigned char T0IE:1;
    unsigned char PEIE:1;
    unsigned char GIE:1;
  };
} __INTCON_bits_t;
extern volatile __INTCON_bits_t __at(INTCON_ADDR) INTCON_bits;

#define RAIF                 INTCON_bits.RAIF
#define INTF                 INTCON_bits.INTF
#define T0IF                 INTCON_bits.T0IF
#define RAIE                 INTCON_bits.RAIE
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
    unsigned char NOT_RAPU:1;
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
#define NOT_RAPU             OPTION_REG_bits.NOT_RAPU

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
    unsigned char IOCA5:1;
    unsigned char ENC_DEC:1;
    unsigned char VREN:1;
  };
  struct {
    unsigned char IOCA0:1;
    unsigned char IOCA1:1;
    unsigned char IOCA2:1;
    unsigned char IOCA3:1;
    unsigned char IOCA4:1;
    unsigned char VRR:1;
    unsigned char :1;
    unsigned char GO:1;
  };
  struct {
    unsigned char RD:1;
    unsigned char WR:1;
    unsigned char WREN:1;
    unsigned char WRERR:1;
    unsigned char PLVDEN:1;
    unsigned char IRVST:1;
    unsigned char :1;
    unsigned char :1;
  };
  struct {
    unsigned char VR0:1;
    unsigned char VR1:1;
    unsigned char VR2:1;
    unsigned char VR3:1;
    unsigned char WDA4:1;
    unsigned char WDA5:1;
    unsigned char :1;
    unsigned char :1;
  };
  struct {
    unsigned char CRREG0:1;
    unsigned char CRREG1:1;
    unsigned char LVDL2:1;
    unsigned char RA3:1;
    unsigned char WPUDA4:1;
    unsigned char WPUDA5:1;
    unsigned char :1;
    unsigned char :1;
  };
  struct {
    unsigned char LVDL0:1;
    unsigned char LVDL1:1;
    unsigned char WDA2:1;
    unsigned char GP3:1;
    unsigned char RA4:1;
    unsigned char RA5:1;
    unsigned char :1;
    unsigned char :1;
  };
  struct {
    unsigned char WDA0:1;
    unsigned char WDA1:1;
    unsigned char WPUDA2:1;
    unsigned char :1;
    unsigned char GP4:1;
    unsigned char GP5:1;
    unsigned char :1;
    unsigned char :1;
  };
  struct {
    unsigned char WPUDA0:1;
    unsigned char WPUDA1:1;
    unsigned char RA2:1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
  };
  struct {
    unsigned char RA0:1;
    unsigned char RA1:1;
    unsigned char GP2:1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
  };
  struct {
    unsigned char GP0:1;
    unsigned char GP1:1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
    unsigned char :1;
  };
} __OSCTUNE_bits_t;
extern volatile __OSCTUNE_bits_t __at(OSCTUNE_ADDR) OSCTUNE_bits;

#define TUN0                 OSCTUNE_bits.TUN0
#define IOCA0                OSCTUNE_bits.IOCA0
#define RD                   OSCTUNE_bits.RD
#define VR0                  OSCTUNE_bits.VR0
#define CRREG0               OSCTUNE_bits.CRREG0
#define LVDL0                OSCTUNE_bits.LVDL0
#define WDA0                 OSCTUNE_bits.WDA0
#define WPUDA0               OSCTUNE_bits.WPUDA0
#define RA0                  OSCTUNE_bits.RA0
#define GP0                  OSCTUNE_bits.GP0
#define TUN1                 OSCTUNE_bits.TUN1
#define IOCA1                OSCTUNE_bits.IOCA1
#define WR                   OSCTUNE_bits.WR
#define VR1                  OSCTUNE_bits.VR1
#define CRREG1               OSCTUNE_bits.CRREG1
#define LVDL1                OSCTUNE_bits.LVDL1
#define WDA1                 OSCTUNE_bits.WDA1
#define WPUDA1               OSCTUNE_bits.WPUDA1
#define RA1                  OSCTUNE_bits.RA1
#define GP1                  OSCTUNE_bits.GP1
#define TUN2                 OSCTUNE_bits.TUN2
#define IOCA2                OSCTUNE_bits.IOCA2
#define WREN                 OSCTUNE_bits.WREN
#define VR2                  OSCTUNE_bits.VR2
#define LVDL2                OSCTUNE_bits.LVDL2
#define WDA2                 OSCTUNE_bits.WDA2
#define WPUDA2               OSCTUNE_bits.WPUDA2
#define RA2                  OSCTUNE_bits.RA2
#define GP2                  OSCTUNE_bits.GP2
#define TUN3                 OSCTUNE_bits.TUN3
#define IOCA3                OSCTUNE_bits.IOCA3
#define WRERR                OSCTUNE_bits.WRERR
#define VR3                  OSCTUNE_bits.VR3
#define RA3                  OSCTUNE_bits.RA3
#define GP3                  OSCTUNE_bits.GP3
#define TUN4                 OSCTUNE_bits.TUN4
#define IOCA4                OSCTUNE_bits.IOCA4
#define PLVDEN               OSCTUNE_bits.PLVDEN
#define WDA4                 OSCTUNE_bits.WDA4
#define WPUDA4               OSCTUNE_bits.WPUDA4
#define RA4                  OSCTUNE_bits.RA4
#define GP4                  OSCTUNE_bits.GP4
#define IOCA5                OSCTUNE_bits.IOCA5
#define VRR                  OSCTUNE_bits.VRR
#define IRVST                OSCTUNE_bits.IRVST
#define WDA5                 OSCTUNE_bits.WDA5
#define WPUDA5               OSCTUNE_bits.WPUDA5
#define RA5                  OSCTUNE_bits.RA5
#define GP5                  OSCTUNE_bits.GP5
#define ENC_DEC              OSCTUNE_bits.ENC_DEC
#define VREN                 OSCTUNE_bits.VREN
#define GO                   OSCTUNE_bits.GO

// ----- PCON bits --------------------
typedef union {
  struct {
    unsigned char NOT_BOD:1;
    unsigned char NOT_POR:1;
    unsigned char :1;
    unsigned char NOT_WUR:1;
    unsigned char SBODEN:1;
    unsigned char ULPWUE:1;
    unsigned char :1;
    unsigned char :1;
  };
} __PCON_bits_t;
extern volatile __PCON_bits_t __at(PCON_ADDR) PCON_bits;

#define NOT_BOD              PCON_bits.NOT_BOD
#define NOT_POR              PCON_bits.NOT_POR
#define NOT_WUR              PCON_bits.NOT_WUR
#define SBODEN               PCON_bits.SBODEN
#define ULPWUE               PCON_bits.ULPWUE

// ----- PIE1 bits --------------------
typedef union {
  struct {
    unsigned char TMR1IE:1;
    unsigned char :1;
    unsigned char OSFIE:1;
    unsigned char C1IE:1;
    unsigned char :1;
    unsigned char CRIE:1;
    unsigned char LVDIE:1;
    unsigned char EEIE:1;
  };
} __PIE1_bits_t;
extern volatile __PIE1_bits_t __at(PIE1_ADDR) PIE1_bits;

#define TMR1IE               PIE1_bits.TMR1IE
#define OSFIE                PIE1_bits.OSFIE
#define C1IE                 PIE1_bits.C1IE
#define CRIE                 PIE1_bits.CRIE
#define LVDIE                PIE1_bits.LVDIE
#define EEIE                 PIE1_bits.EEIE

// ----- PIR1 bits --------------------
typedef union {
  struct {
    unsigned char TMR1IF:1;
    unsigned char :1;
    unsigned char OSFIF:1;
    unsigned char C1IF:1;
    unsigned char :1;
    unsigned char CRIF:1;
    unsigned char LVDIF:1;
    unsigned char EEIF:1;
  };
} __PIR1_bits_t;
extern volatile __PIR1_bits_t __at(PIR1_ADDR) PIR1_bits;

#define TMR1IF               PIR1_bits.TMR1IF
#define OSFIF                PIR1_bits.OSFIF
#define C1IF                 PIR1_bits.C1IF
#define CRIF                 PIR1_bits.CRIF
#define LVDIF                PIR1_bits.LVDIF
#define EEIF                 PIR1_bits.EEIF

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

#endif
