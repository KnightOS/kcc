
/*
 * pic18f1220.h - PIC18F1220 Device Library Header
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

#ifndef __PIC18F1220_H__
#define __PIC18F1220_H__

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
		unsigned :1;
		unsigned TMR3IE:1;
		unsigned LVDIE:1;
		unsigned :1;
		unsigned EEIE:1;
		unsigned :1;
		unsigned :1;
		unsigned OSCFIE:1;
	};
} __PIE2bits_t;

extern volatile __PIE2bits_t at 0xfa0 PIE2bits;

extern sfr at 0xfa1 PIR2;
typedef union {
	struct {
		unsigned :1;
		unsigned TMR3IF:1;
		unsigned LVDIF:1;
		unsigned :1;
		unsigned EEIF:1;
		unsigned :1;
		unsigned :1;
		unsigned OSCFIF:1;
	};
} __PIR2bits_t;

extern volatile __PIR2bits_t at 0xfa1 PIR2bits;

extern sfr at 0xfa2 IPR2;
typedef union {
	struct {
		unsigned :1;
		unsigned TMR3IP:1;
		unsigned LVDIP:1;
		unsigned :1;
		unsigned EEIP:1;
		unsigned :1;
		unsigned :1;
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
extern sfr at 0xfaa BAUDCTL;
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
extern sfr at 0xfb0 SPBRGH;
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

/* MCLR enable 2H options */
#define _MCLRE_MCLR_enabled_RA5_input_dis_2H	0xFF	/* MCLR enabled__RA5_input_disabled */
#define _MCLRE_MCLR_disabled_RA5_input_en_2H	0x7F	/* MCLR disabled__RA5_input_enabled */

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

/* Table Read Protect 000800-00FFF 6L options */
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
