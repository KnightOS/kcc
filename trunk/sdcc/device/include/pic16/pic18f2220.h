
/*
 * pic18f2220.h - PIC18F2220 Device Library Header
 *
 * This file is part of the GNU PIC Library.
 *
 * January, 2004
 * The GNU PIC Library is maintained by,
 * 	Vangelis Rokas <vrokas@otenet.gr>
 *
 * $Id$
 *
 */

#ifndef __PIC18F2220_H__
#define __PIC18F2220_H__

extern sfr at 0xf80 PORTA;
typedef union {
	struct {
		unsigned RA0:1;
		unsigned RA1:1;
		unsigned RA2:1;
		unsigned RA3:1;
		unsigned RA4:1;
		unsigned RA5:1;
		unsigned RA6:1;
		unsigned :1;
	};

	struct {
		unsigned AN0:1;
		unsigned AN1:1;
		unsigned AN2:1;
		unsigned AN3:1;
		unsigned :1;
		unsigned AN4:1;
		unsigned OSC2:1;
		unsigned :1;
	};

	struct {
		unsigned :1;
		unsigned :1;
		unsigned VREFM:1;
		unsigned VREFP:1;
		unsigned T0CKI:1;
		unsigned SS:1;
		unsigned CLK0:1;
		unsigned :1;
	};

	struct {
		unsigned :1;
		unsigned :1;
		unsigned :1;
		unsigned :1;
		unsigned :1;
		unsigned LVDIN:1;
		unsigned :1;
		unsigned :1;
	};
} __PORTAbits_t;

extern volatile __PORTAbits_t at 0xf80 PORTAbits;

extern sfr at 0xf81 PORTB;
typedef union {
	struct {
		unsigned RB0:1;
		unsigned RB1:1;
		unsigned RB2:1;
		unsigned RB3:1;
		unsigned RB4:1;
		unsigned RB5:1;
		unsigned RB6:1;
		unsigned RB7:1;
	};

	struct {
		unsigned INT0:1;
		unsigned INT1:1;
		unsigned INT2:1;
		unsigned INT3:1;
		unsigned :1;
		unsigned :1;
		unsigned :1;
		unsigned :1;
	};
} __PORTBbits_t;

extern volatile __PORTBbits_t at 0xf81 PORTBbits;

extern sfr at 0xf82 PORTC;
typedef union {
	struct {
		unsigned RC0:1;
		unsigned RC1:1;
		unsigned RC2:1;
		unsigned RC3:1;
		unsigned RC4:1;
		unsigned RC5:1;
		unsigned RC6:1;
		unsigned RC7:1;
	};

	struct {
		unsigned T1OSO:1;
		unsigned T1OSI:1;
		unsigned :1;
		unsigned SCK:1;
		unsigned SDI:1;
		unsigned SDO:1;
		unsigned TX:1;
		unsigned RX:1;
	};

	struct {
		unsigned T1CKI:1;
		unsigned CCP2:1;
		unsigned CCP1:1;
		unsigned SCL:1;
		unsigned SDA:1;
		unsigned :1;
		unsigned CK:1;
		unsigned DT:1;
	};
} __PORTCbits_t;

extern volatile __PORTCbits_t at 0xf82 PORTCbits;

extern sfr at 0xf89 LATA;
typedef union {
	struct {
		unsigned LATA0:1;
		unsigned LATA1:1;
		unsigned LATA2:1;
		unsigned LATA3:1;
		unsigned LATA4:1;
		unsigned LATA5:1;
		unsigned LATA6:1;
		unsigned :1;
	};
} __LATAbits_t;

extern volatile __LATAbits_t at 0xf89 LATAbits;

extern sfr at 0xf8a LATB;
typedef union {
	struct {
		unsigned LATB0:1;
		unsigned LATB1:1;
		unsigned LATB2:1;
		unsigned LATB3:1;
		unsigned LATB4:1;
		unsigned LATB5:1;
		unsigned LATB6:1;
		unsigned LATB7:1;
	};
} __LATBbits_t;

extern volatile __LATBbits_t at 0xf8a LATBbits;

extern sfr at 0xf8b LATC;
typedef union {
	struct {
		unsigned LATC0:1;
		unsigned LATC1:1;
		unsigned LATC2:1;
		unsigned LATC3:1;
		unsigned LATC4:1;
		unsigned LATC5:1;
		unsigned LATC6:1;
		unsigned LATC7:1;
	};
} __LATCbits_t;

extern volatile __LATCbits_t at 0xf8b LATCbits;

extern sfr at 0xf92 TRISA;
typedef union {
	struct {
		unsigned TRISA0:1;
		unsigned TRISA1:1;
		unsigned TRISA2:1;
		unsigned TRISA3:1;
		unsigned TRISA4:1;
		unsigned TRISA5:1;
		unsigned TRISA6:1;
		unsigned :1;
	};
} __TRISAbits_t;

extern volatile __TRISAbits_t at 0xf92 TRISAbits;

extern sfr at 0xf93 TRISB;
typedef union {
	struct {
		unsigned TRISB0:1;
		unsigned TRISB1:1;
		unsigned TRISB2:1;
		unsigned TRISB3:1;
		unsigned TRISB4:1;
		unsigned TRISB5:1;
		unsigned TRISB6:1;
		unsigned TRISB7:1;
	};
} __TRISBbits_t;

extern volatile __TRISBbits_t at 0xf93 TRISBbits;

extern sfr at 0xf94 TRISC;
typedef union {
	struct {
		unsigned TRISC0:1;
		unsigned TRISC1:1;
		unsigned TRISC2:1;
		unsigned TRISC3:1;
		unsigned TRISC4:1;
		unsigned TRISC5:1;
		unsigned TRISC6:1;
		unsigned TRISC7:1;
	};
} __TRISCbits_t;

extern volatile __TRISCbits_t at 0xf94 TRISCbits;

extern sfr at 0xf9b OSCTUNE;
typedef union {
	struct {
		unsigned TUN0:1;
		unsigned TUN1:1;
		unsigned TUN2:1;
		unsigned TUN3:1;
		unsigned TUN4:1;
		unsigned TUN5:1;
		unsigned :1;
		unsigned :1;
	};
} __OSCTUNEbits_t;

extern volatile __OSCTUNEbits_t at 0xf9b OSCTUNEbits;

extern sfr at 0xf9d PIE1;
typedef union {
	struct {
		unsigned TMR1IE:1;
		unsigned TMR2IE:1;
		unsigned CCP1IE:1;
		unsigned SSPIE:1;
		unsigned TXIE:1;
		unsigned RCIE:1;
		unsigned ADIE:1;
		unsigned PSPIE:1;
	};
} __PIE1bits_t;

extern volatile __PIE1bits_t at 0xf9d PIE1bits;

extern sfr at 0xf9e PIR1;
typedef union {
	struct {
		unsigned TMR1IF:1;
		unsigned TMR2IF:1;
		unsigned CCP1IF:1;
		unsigned SSPIF:1;
		unsigned TXIF:1;
		unsigned RCIF:1;
		unsigned ADIF:1;
		unsigned PSPIF:1;
	};
} __PIR1bits_t;

extern volatile __PIR1bits_t at 0xf9e PIR1bits;

extern sfr at 0xf9f IPR1;
typedef union {
	struct {
		unsigned TMR1IP:1;
		unsigned TMR2IP:1;
		unsigned CCP1IP:1;
		unsigned SSPIP:1;
		unsigned TXIP:1;
		unsigned RCIP:1;
		unsigned ADIP:1;
		unsigned PSPIP:1;
	};
} __IPR1bits_t;

extern volatile __IPR1bits_t at 0xf9f IPR1bits;

extern sfr at 0xfa0 PIE2;
typedef union {
	struct {
		unsigned CCP2IE:1;
		unsigned TMR3IE:1;
		unsigned LVDIE:1;
		unsigned BCLIE:1;
		unsigned EEIE:1;
		unsigned :1;
		unsigned CMIE:1;
		unsigned OSCFIE:1;
	};
} __PIE2bits_t;

extern volatile __PIE2bits_t at 0xfa0 PIE2bits;

extern sfr at 0xfa1 PIR2;
typedef union {
	struct {
		unsigned CCP2IF:1;
		unsigned TMR3IF:1;
		unsigned LVDIF:1;
		unsigned BCLIF:1;
		unsigned EEIF:1;
		unsigned :1;
		unsigned CMIF:1;
		unsigned OSCFIF:1;
	};
} __PIR2bits_t;

extern volatile __PIR2bits_t at 0xfa1 PIR2bits;

extern sfr at 0xfa2 IPR2;
typedef union {
	struct {
		unsigned CCP2IP:1;
		unsigned TMR3IP:1;
		unsigned LVDIP:1;
		unsigned BCLIP:1;
		unsigned EEIP:1;
		unsigned :1;
		unsigned CMIP:1;
		unsigned OSCFIP:1;
	};
} __IPR2bits_t;

extern volatile __IPR2bits_t at 0xfa2 IPR2bits;

extern sfr at 0xfa6 EECON1;
typedef union {
	struct {
		unsigned RD:1;
		unsigned WR:1;
		unsigned WREN:1;
		unsigned WRERR:1;
		unsigned FREE:1;
		unsigned :1;
		unsigned CFGS:1;
		unsigned EEPGD:1;
	};
} __EECON1bits_t;

extern volatile __EECON1bits_t at 0xfa6 EECON1bits;

extern sfr at 0xfa7 EECON2;
extern sfr at 0xfa8 EEDATA;
extern sfr at 0xfa9 EEADR;
extern sfr at 0xfab RCSTA;
typedef union {
	struct {
		unsigned RX9D:1;
		unsigned OERR:1;
		unsigned FERR:1;
		unsigned ADDEN:1;
		unsigned CREN:1;
		unsigned SREN:1;
		unsigned RX9:1;
		unsigned SPEN:1;
	};
} __RCSTAbits_t;

extern volatile __RCSTAbits_t at 0xfab RCSTAbits;

extern sfr at 0xfac TXSTA;
typedef union {
	struct {
		unsigned TX9D:1;
		unsigned TRMT:1;
		unsigned BRGH:1;
		unsigned :1;
		unsigned SYNC:1;
		unsigned TXEN:1;
		unsigned TX9:1;
		unsigned CSRC:1;
	};
} __TXSTAbits_t;

extern volatile __TXSTAbits_t at 0xfac TXSTAbits;

extern sfr at 0xfad TXREG;
extern sfr at 0xfae RCREG;
extern sfr at 0xfaf SPBRG;
extern sfr at 0xfb1 T3CON;
typedef union {
	struct {
		unsigned TMR3ON:1;
		unsigned TMR3CS:1;
		unsigned T3SYNC:1;
		unsigned T3CCP1:1;
		unsigned T3CKPS0:1;
		unsigned T3CKPS1:1;
		unsigned T3CCP2:1;
		unsigned RD16:1;
	};
} __T3CONbits_t;

extern volatile __T3CONbits_t at 0xfb1 T3CONbits;

extern sfr at 0xfb2 TMR3L;
extern sfr at 0xfb3 TMR3H;
extern sfr at 0xfb4 CMCON;
typedef union {
	struct {
		unsigned CM0:1;
		unsigned CM1:1;
		unsigned CM2:1;
		unsigned CIS:1;
		unsigned C1INV:1;
		unsigned C2INV:1;
		unsigned C1OUT:1;
		unsigned C2OUT:1;
	};
} __CMCONbits_t;

extern volatile __CMCONbits_t at 0xfb4 CMCONbits;

extern sfr at 0xfb5 CVRCON;
typedef union {
	struct {
		unsigned CVR0:1;
		unsigned CVR1:1;
		unsigned CVR2:1;
		unsigned CVR3:1;
		unsigned CVREF:1;
		unsigned CVRR:1;
		unsigned CVROE:1;
		unsigned CVREN:1;
	};
} __CVRCONbits_t;

extern volatile __CVRCONbits_t at 0xfb5 CVRCONbits;

extern sfr at 0xfba CCP2CON;
typedef union {
	struct {
		unsigned CCP2M0:1;
		unsigned CCP2M1:1;
		unsigned CCP2M2:1;
		unsigned CCP2M3:1;
		unsigned DCCP2Y:1;
		unsigned DCCP2X:1;
		unsigned :1;
		unsigned :1;
	};
} __CCP2CONbits_t;

extern volatile __CCP2CONbits_t at 0xfba CCP2CONbits;

extern sfr at 0xfbb CCPR2L;
extern sfr at 0xfbc CCPR2H;
extern sfr at 0xfbd CCP1CON;
typedef union {
	struct {
		unsigned CCP1M0:1;
		unsigned CCP1M1:1;
		unsigned CCP1M2:1;
		unsigned CCP1M3:1;
		unsigned DCCP1Y:1;
		unsigned DCCP1X:1;
		unsigned :1;
		unsigned :1;
	};
} __CCP1CONbits_t;

extern volatile __CCP1CONbits_t at 0xfbd CCP1CONbits;

extern sfr at 0xfbe CCPR1L;
extern sfr at 0xfbf CCPR1H;
extern sfr at 0xfc0 ADCON2;
typedef union {
	struct {
		unsigned ADCS0:1;
		unsigned ADCS1:1;
		unsigned ADCS2:1;
		unsigned :1;
		unsigned :1;
		unsigned :1;
		unsigned :1;
		unsigned ADFM:1;
	};
} __ADCON2bits_t;

extern volatile __ADCON2bits_t at 0xfc0 ADCON2bits;

extern sfr at 0xfc1 ADCON1;
typedef union {
	struct {
		unsigned PCFG0:1;
		unsigned PCFG1:1;
		unsigned PCFG2:1;
		unsigned PCFG3:1;
		unsigned VCFG0:1;
		unsigned VCFG1:1;
		unsigned :1;
		unsigned :1;
	};
} __ADCON1bits_t;

extern volatile __ADCON1bits_t at 0xfc1 ADCON1bits;

extern sfr at 0xfc2 ADCON0;
typedef union {
	struct {
		unsigned ADON:1;
		unsigned GO:1;
		unsigned CHS0:1;
		unsigned CHS1:1;
		unsigned CHS2:1;
		unsigned CHS3:1;
		unsigned :1;
		unsigned :1;
	};
} __ADCON0bits_t;

extern volatile __ADCON0bits_t at 0xfc2 ADCON0bits;

extern sfr at 0xfc3 ADRESL;
extern sfr at 0xfc4 ADRESH;
extern sfr at 0xfc5 SSPCON2;
typedef union {
	struct {
		unsigned SEN:1;
		unsigned RSEN:1;
		unsigned PEN:1;
		unsigned RCEN:1;
		unsigned ACKEN:1;
		unsigned ACKDT:1;
		unsigned ACKSTAT:1;
		unsigned GCEN:1;
	};
} __SSPCON2bits_t;

extern volatile __SSPCON2bits_t at 0xfc5 SSPCON2bits;

extern sfr at 0xfc6 SSPCON1;
typedef union {
	struct {
		unsigned SSPM0:1;
		unsigned SSPM1:1;
		unsigned SSPM2:1;
		unsigned SSPM3:1;
		unsigned CKP:1;
		unsigned SSPEN:1;
		unsigned SSPOV:1;
		unsigned WCOL:1;
	};
} __SSPCON1bits_t;

extern volatile __SSPCON1bits_t at 0xfc6 SSPCON1bits;

extern sfr at 0xfc7 SSPSTAT;
typedef union {
	struct {
		unsigned BF:1;
		unsigned UA:1;
		unsigned R_W:1;
		unsigned S:1;
		unsigned P:1;
		unsigned D_A:1;
		unsigned CKE:1;
		unsigned SMP:1;
	};
} __SSPSTATbits_t;

extern volatile __SSPSTATbits_t at 0xfc7 SSPSTATbits;

extern sfr at 0xfc8 SSPADD;
extern sfr at 0xfc9 SSPBUF;
extern sfr at 0xfca T2CON;
typedef union {
	struct {
		unsigned T2CKPS0:1;
		unsigned T2CKPS1:1;
		unsigned TMR2ON:1;
		unsigned TOUTPS0:1;
		unsigned TOUTPS1:1;
		unsigned TOUTPS2:1;
		unsigned TOUTPS3:1;
		unsigned :1;
	};
} __T2CONbits_t;

extern volatile __T2CONbits_t at 0xfca T2CONbits;

extern sfr at 0xfcb PR2;
extern sfr at 0xfcc TMR2;
extern sfr at 0xfcd T1CON;
typedef union {
	struct {
		unsigned TMR1ON:1;
		unsigned TMR1CS:1;
		unsigned NOT_T1SYNC:1;
		unsigned T1OSCEN:1;
		unsigned T1CKPS0:1;
		unsigned T1CKPS1:1;
		unsigned :1;
		unsigned RD16:1;
	};
} __T1CONbits_t;

extern volatile __T1CONbits_t at 0xfcd T1CONbits;

extern sfr at 0xfce TMR1L;
extern sfr at 0xfcf TMR1H;
extern sfr at 0xfd0 RCON;
typedef union {
	struct {
		unsigned BOR:1;
		unsigned POR:1;
		unsigned PD:1;
		unsigned TO:1;
		unsigned RI:1;
		unsigned :1;
		unsigned :1;
		unsigned IPEN:1;
	};
} __RCONbits_t;

extern volatile __RCONbits_t at 0xfd0 RCONbits;

extern sfr at 0xfd1 WDTCON;
typedef union {
	struct {
		unsigned SWDTEN:1;
		unsigned :1;
		unsigned :1;
		unsigned :1;
		unsigned :1;
		unsigned :1;
		unsigned :1;
		unsigned :1;
	};

	struct {
		unsigned SWDTE:1;
		unsigned :1;
		unsigned :1;
		unsigned :1;
		unsigned :1;
		unsigned :1;
		unsigned :1;
		unsigned :1;
	};
} __WDTCONbits_t;

extern volatile __WDTCONbits_t at 0xfd1 WDTCONbits;

extern sfr at 0xfd2 LVDCON;
typedef union {
	struct {
		unsigned LVDL0:1;
		unsigned LVDL1:1;
		unsigned LVDL2:1;
		unsigned LVDL3:1;
		unsigned LVDEN:1;
		unsigned VRST:1;
		unsigned :1;
		unsigned :1;
	};

	struct {
		unsigned LVV0:1;
		unsigned LVV1:1;
		unsigned LVV2:1;
		unsigned LVV3:1;
		unsigned :1;
		unsigned BGST:1;
		unsigned :1;
		unsigned :1;
	};
} __LVDCONbits_t;

extern volatile __LVDCONbits_t at 0xfd2 LVDCONbits;

extern sfr at 0xfd3 OSCCON;
typedef union {
	struct {
		unsigned SCS:1;
		unsigned :1;
		unsigned :1;
		unsigned :1;
		unsigned :1;
		unsigned :1;
		unsigned :1;
		unsigned :1;
	};
} __OSCCONbits_t;

extern volatile __OSCCONbits_t at 0xfd3 OSCCONbits;

extern sfr at 0xfd5 T0CON;
extern sfr at 0xfd6 TMR0L;
extern sfr at 0xfd7 TMR0H;
extern sfr at 0xfd8 STATUS;
typedef union {
	struct {
		unsigned C:1;
		unsigned DC:1;
		unsigned Z:1;
		unsigned OV:1;
		unsigned N:1;
		unsigned :1;
		unsigned :1;
		unsigned :1;
	};
} __STATUSbits_t;

extern volatile __STATUSbits_t at 0xfd8 STATUSbits;

extern sfr at 0xfd9 FSR2L;
extern sfr at 0xfda FSR2H;
extern sfr at 0xfdb PLUSW2;
extern sfr at 0xfdc PREINC2;
extern sfr at 0xfdd POSTDEC2;
extern sfr at 0xfde POSTINC2;
extern sfr at 0xfdf INDF2;
extern sfr at 0xfe0 BSR;
extern sfr at 0xfe1 FSR1L;
extern sfr at 0xfe2 FSR1H;
extern sfr at 0xfe3 PLUSW1;
extern sfr at 0xfe4 PREINC1;
extern sfr at 0xfe5 POSTDEC1;
extern sfr at 0xfe6 POSTINC1;
extern sfr at 0xfe7 INDF1;
extern sfr at 0xfe8 WREG;
extern sfr at 0xfe9 FSR0L;
extern sfr at 0xfea FSR0H;
extern sfr at 0xfeb PLUSW0;
extern sfr at 0xfec PREINC0;
extern sfr at 0xfed POSTDEC0;
extern sfr at 0xfee POSTINC0;
extern sfr at 0xfef INDF0;
extern sfr at 0xff0 INTCON3;
typedef union {
	struct {
		unsigned INT1F:1;
		unsigned INT2F:1;
		unsigned :1;
		unsigned INT1E:1;
		unsigned INT2E:1;
		unsigned :1;
		unsigned INT1P:1;
		unsigned INT2P:1;
	};

	struct {
		unsigned INT1IF:1;
		unsigned INT2IF:1;
		unsigned :1;
		unsigned INT1IE:1;
		unsigned INT2IE:1;
		unsigned :1;
		unsigned INT1IP:1;
		unsigned INT2IP:1;
	};
} __INTCON3bits_t;

extern volatile __INTCON3bits_t at 0xff0 INTCON3bits;

extern sfr at 0xff1 INTCON2;
typedef union {
	struct {
		unsigned RBIP:1;
		unsigned :1;
		unsigned T0IP:1;
		unsigned :1;
		unsigned INTEDG2:1;
		unsigned INTEDG1:1;
		unsigned INTEDG0:1;
		unsigned RBPU:1;
	};
} __INTCON2bits_t;

extern volatile __INTCON2bits_t at 0xff1 INTCON2bits;

extern sfr at 0xff2 INTCON;
typedef union {
	struct {
		unsigned RBIF:1;
		unsigned INT0F:1;
		unsigned T0IF:1;
		unsigned RBIE:1;
		unsigned INT0E:1;
		unsigned T0IE:1;
		unsigned PEIE:1;
		unsigned GIE:1;
	};
} __INTCONbits_t;

extern volatile __INTCONbits_t at 0xff2 INTCONbits;

extern sfr at 0xff3 PRODL;
extern sfr at 0xff4 PRODH;
extern sfr at 0xff5 TABLAT;
extern sfr at 0xff6 TBLPTRL;
extern sfr at 0xff7 TBLPTRH;
extern sfr at 0xff8 TBLPTRU;
extern sfr at 0xff9 PCL;
extern sfr at 0xffa PCLATH;
extern sfr at 0xffb PCLATU;
extern sfr at 0xffc STKPTR;
typedef union {
	struct {
		unsigned STKPTR0:1;
		unsigned STKPTR1:1;
		unsigned STKPTR2:1;
		unsigned STKPTR3:1;
		unsigned STKPTR4:1;
		unsigned :1;
		unsigned STKUNF:1;
		unsigned STKFUL:1;
	};
} __STKPTRbits_t;

extern volatile __STKPTRbits_t at 0xffc STKPTRbits;

extern sfr at 0xffd TOSL;
extern sfr at 0xffe TOSH;
extern sfr at 0xfff TOSU;


/* Configuration registers locations */
#define __CONFIG0H	0x300001
#define __CONFIG1L	0x300002
#define __CONFIG1H	0x300003
#define __CONFIG2H	0x300005
#define __CONFIG3L	0x300006
#define __CONFIG4L	0x300008
#define __CONFIG4H	0x300009
#define __CONFIG5L	0x30000A
#define __CONFIG5H	0x30000B
#define __CONFIG6L	0x30000C
#define __CONFIG6H	0x30000D



/* Oscillator 0H options */
#define _OSC_11XX_0H	0xFC	/* 11XX EXT RC-CLKOUT on RA6 */
#define _OSC_101X_0H	0xFA	/* 101X EXT RC-CLKOUT on RA6 */
#define _OSC_INT_CLKOUT_on_RA6_Port_on_RA7_0H	0xF9	/* INT RC-CLKOUT_on_RA6_Port_on_RA7 */
#define _OSC_INT_Port_on_RA6_Port_on_RA7_0H	0xF8	/* INT RC-Port_on_RA6_Port_on_RA7 */
#define _OSC_EXT_Port_on_RA6_0H	0xF7	/* EXT RC-Port_on_RA6 */
#define _OSC_HS_PLL_0H	0xF6	/* HS-PLL enabled freq=4xFosc1 */
#define _OSC_EC_PORT_0H	0xF5	/* EC-Port on RA6 */
#define _OSC_EC_CLKOUT_0H	0xF4	/* EC-CLKOUT on RA6 */
#define _OSC_EXT_CLKOUT_on_RA6_0H	0xF3	/* EXT RC-CLKOUT_on_RA6 */
#define _OSC_HS_0H	0xF2	/* HS */
#define _OSC_XT_0H	0xF1	/* XT */
#define _OSC_LP_0H	0xF0	/* LP */

/* Fail Safe Clock Monitor Enable 0H options */
#define _FCMEN_OFF_0H	0xBF	/* Disabled */
#define _FCMEN_ON_0H	0xFF	/* Enabled */

/* Internal External Switch Over 0H options */
#define _IESO_OFF_0H	0x7F	/* Disabled */
#define _IESO_ON_0H	0xFF	/* Enabled */

/* Power Up Timer 1L options */
#define _PUT_OFF_1L	0xFF	/* Disabled */
#define _PUT_ON_1L	0xFE	/* Enabled */

/* Brown Out Detect 1L options */
#define _BODEN_ON_1L	0xFF	/* Enabled */
#define _BODEN_OFF_1L	0xFD	/* Disabled */

/* Brown Out Voltage 1L options */
#define _BODENV_2_0V_1L	0xFF	/* 2.0V */
#define _BODENV_2_7V_1L	0xFB	/* 2.7V */
#define _BODENV_4_2V_1L	0xF7	/* 4.2V */
#define _BODENV_4_5V_1L	0xF3	/* 4.5V */

/* Watchdog Timer 1H options */
#define _WDT_ON_1H	0xFF	/* Enabled */
#define _WDT_DISABLED_CONTROLLED_1H	0xFE	/* Disabled-Controlled by SWDTEN bit */

/* Watchdog Postscaler 1H options */
#define _WDTPS_1_32768_1H	0xFF	/* 1:32768 */
#define _WDTPS_1_16384_1H	0xFD	/* 1:16384 */
#define _WDTPS_1_8192_1H	0xFB	/* 1:8192 */
#define _WDTPS_1_4096_1H	0xF9	/* 1:4096 */
#define _WDTPS_1_2048_1H	0xF7	/* 1:2048 */
#define _WDTPS_1_1024_1H	0xF5	/* 1:1024 */
#define _WDTPS_1_512_1H	0xF3	/* 1:512 */
#define _WDTPS_1_256_1H	0xF1	/* 1:256 */
#define _WDTPS_1_128_1H	0xEF	/* 1:128 */
#define _WDTPS_1_64_1H	0xED	/* 1:64 */
#define _WDTPS_1_32_1H	0xEB	/* 1:32 */
#define _WDTPS_1_16_1H	0xE9	/* 1:16 */
#define _WDTPS_1_8_1H	0xE7	/* 1:8 */
#define _WDTPS_1_4_1H	0xE5	/* 1:4 */
#define _WDTPS_1_2_1H	0xE3	/* 1:2 */
#define _WDTPS_1_1_1H	0xE1	/* 1:1 */

/* CCP2 Mux 2H options */
#define _CCP2MUX_RC1_2H	0xFF	/* RC1 */
#define _CCP2MUX_RB3_2H	0xFE	/* RB3 */

/* PortB A/D Enable 2H options */
#define _PBADEN_PORTB_4_0__analog_inputs_on_RSET_2H	0xFF	/* PORTB<4:0> configured as analog_inputs_on_RESET */
#define _PBADEN_PORTB_4_0__digital_I_O_on_REST_2H	0xFD	/* PORTB<4:0> configured as digital_I_O_on_RESET */

/* MCLR enable 2H options */
#define _MCLRE_MCLR_Enabled_RE3_Disabled_2H	0xFF	/* MCLR Enabled_RE3_Disabled */
#define _MCLRE_MCLR_Disabled_RE3_Enabled_2H	0x7F	/* MCLR Disabled__RE3_Enabled */

/* Stack Overflow Reset 3L options */
#define _STVR_ON_3L	0xFF	/* Enabled */
#define _STVR_OFF_3L	0xFE	/* Disabled */

/* Low Voltage Program 3L options */
#define _LVP_ON_3L	0xFF	/* Enabled */
#define _LVP_OFF_3L	0xFB	/* Disabled */

/* Background Debug 3L options */
#define _BACKBUG_OFF_3L	0xFF	/* Disabled */
#define _BACKBUG_ON_3L	0x7F	/* Enabled */

/* Code Protect 000200-0007FF 4L options */
#define _CP_0_OFF_4L	0xFF	/* Disabled */
#define _CP_0_ON_4L	0xFE	/* Enabled */

/* Code Protect 000800-000FFF 4L options */
#define _CP_1_OFF_4L	0xFF	/* Disabled */
#define _CP_1_ON_4L	0xFD	/* Enabled */

/* Data EE Read Protect 4H options */
#define _CPD_OFF_4H	0xFF	/* Disabled */
#define _CPD_ON_4H	0x7F	/* Enabled */

/* Code Protect Boot 4H options */
#define _CPB_OFF_4H	0xFF	/* Disabled */
#define _CPB_ON_4H	0xBF	/* Enabled */

/* Table Write Protect 00200-007FF 5L options */
#define _WRT_0_OFF_5L	0xFF	/* Disabled */
#define _WRT_0_ON_5L	0xFE	/* Enabled */

/* Table Write Protect 00800-00FFF 5L options */
#define _WRT_1_OFF_5L	0xFF	/* Disabled */
#define _WRT_1_ON_5L	0xFD	/* Enabled */

/* Data EE Write Protect 5H options */
#define _WRTD_OFF_5H	0xFF	/* Disabled */
#define _WRTD_ON_5H	0x7F	/* Enabled */

/* Table Write Protect Boot 5H options */
#define _WRTB_OFF_5H	0xFF	/* Disabled */
#define _WRTB_ON_5H	0xBF	/* Enabled */

/* Config. Write Protect 5H options */
#define _WRTC_OFF_5H	0xFF	/* Disabled */
#define _WRTC_ON_5H	0xDF	/* Enabled */

/* Table Read Protect 00200-007FF 6L options */
#define _EBTR_0_OFF_6L	0xFF	/* Disabled */
#define _EBTR_0_ON_6L	0xFE	/* Enabled */

/* Table Read Protect 00800-00FFF 6L options */
#define _EBTR_1_OFF_6L	0xFF	/* Disabled */
#define _EBTR_1_ON_6L	0xFD	/* Enabled */

/* Table Read Protect Boot 6H options */
#define _EBTRB_OFF_6H	0xFF	/* Disabled */
#define _EBTRB_ON_6H	0xBF	/* Enabled */


/* Device ID locations */
#define __IDLOC0	0x200000
#define __IDLOC1	0x200001
#define __IDLOC2	0x200002
#define __IDLOC3	0x200003
#define __IDLOC4	0x200004
#define __IDLOC5	0x200005
#define __IDLOC6	0x200006
#define __IDLOC7	0x200007


#endif
