;--------------------------------------------------------
; File Created by SDCC : FreeWare ANSI-C Compiler
; Version 2.1.9Ga Sun Jan 16 17:31:30 2000

;--------------------------------------------------------
	.module _long2fs
;--------------------------------------------------------
; publics variables in this module
;--------------------------------------------------------
	.globl ___long2fs
;--------------------------------------------------------
; special function registers
;--------------------------------------------------------
;--------------------------------------------------------
; special function bits 
;--------------------------------------------------------
;--------------------------------------------------------
; internal ram data
;--------------------------------------------------------
	.area	DSEG    (DATA)
___long2fs_sloc0_1_0:
	.ds	0x0002
___long2fs_sloc1_1_0:
	.ds	0x0004
;--------------------------------------------------------
; overlayable items in internal ram 
;--------------------------------------------------------
	.area	OSEG	(OVR,DATA)
;--------------------------------------------------------
; indirectly addressable internal ram data
;--------------------------------------------------------
	.area	ISEG    (DATA)
;--------------------------------------------------------
; bit data
;--------------------------------------------------------
	.area	BSEG    (BIT)
;--------------------------------------------------------
; external ram data
;--------------------------------------------------------
	.area	XSEG    (XDATA)
___long2fs_a1_1_1:
	.ds	0x0004
___long2fs_sign_1_1:
	.ds	0x0004
___long2fs_fl_1_1:
	.ds	0x0004
;--------------------------------------------------------
; global & static initialisations
;--------------------------------------------------------
	.area GSINIT (CODE)
;--------------------------------------------------------
; code
;--------------------------------------------------------
	.area CSEG (CODE)
	G$__long2fs$0$0 ==.
;	_long2fs.c 80
;	-----------------------------------------
;	 function __long2fs
;	-----------------------------------------
___long2fs:
	ar2 = 0x02
	ar3 = 0x03
	ar4 = 0x04
	ar5 = 0x05
	ar6 = 0x06
	ar7 = 0x07
	ar0 = 0x00
	ar1 = 0x01
;	_long2fs.c 108
	push	acc
	push	b
	push	dph
	push	dpl
	mov	dptr,#___long2fs_a1_1_1
	pop	acc
	movx	@dptr,a
	pop	acc
	inc	dptr
	movx	@dptr,a
	pop	acc
	inc	dptr
	movx	@dptr,a
	pop	acc
	inc	dptr
	movx	@dptr,a
;	_long2fs.c 82
	mov	dptr,#___long2fs_sign_1_1
	clr	a
	inc	dptr
	inc	dptr
	inc	dptr
	movx	@dptr,a
	lcall	__decdptr
	movx	@dptr,a
	lcall	__decdptr
	movx	@dptr,a
	lcall	__decdptr
	movx	@dptr,a
;	_long2fs.c 86
	mov	dptr,#___long2fs_a1_1_1
	movx	a,@dptr
	mov	r2,a
	inc	dptr
	movx	a,@dptr
	mov	r3,a
	inc	dptr
	movx	a,@dptr
	mov	r4,a
	inc	dptr
	movx	a,@dptr
	mov	r5,a
	mov	a,r2
	orl	a,r3
	orl	a,r4
	orl	a,r5
; Peephole 109   removed ljmp by inverse jump logic
	jnz  00102$
00114$:
;	_long2fs.c 88
	mov	dptr,#(___long2fs_fl_1_1)
; Peephole 101   removed redundant mov
; Peephole 180   changed mov to clr
	clr  a
	movx @dptr,a
	inc  dptr
	movx @dptr,a
	inc	dptr
; Peephole 101   removed redundant mov
; Peephole 180   changed mov to clr
	clr  a
	movx @dptr,a
	inc  dptr
	movx @dptr,a
;	_long2fs.c 89
	mov	dptr,#(___long2fs_fl_1_1)
	movx	a,@dptr
	mov	r6,a
	inc	dptr
	movx	a,@dptr
	mov	r7,a
	inc	dptr
	movx	a,@dptr
	mov	r0,a
	inc	dptr
	movx	a,@dptr
; Peephole 191   removed redundant mov
	mov  r1,a
	mov  dpl,r6
	mov  dph,r7
	mov  b,r0
	ljmp	00108$
00102$:
;	_long2fs.c 92
	mov	a,r5
; Peephole 111   removed ljmp by inverse jump logic
	jnb  acc.7,00113$
00115$:
;	_long2fs.c 94
	mov	dptr,#___long2fs_sign_1_1
; Peephole 101   removed redundant mov
; Peephole 180   changed mov to clr
	clr  a
	movx @dptr,a
	inc  dptr
	movx @dptr,a
	inc	dptr
; Peephole 180   changed mov to clr
	clr  a
	movx	@dptr,a
	inc	dptr
	mov	a,#0x80
	movx	@dptr,a
;	_long2fs.c 95
	mov	dptr,#___long2fs_a1_1_1
	clr	c
	clr	a
	subb	a,r2
	movx	@dptr,a
	clr	a
	subb	a,r3
	inc	dptr
	movx	@dptr,a
	clr	a
	subb	a,r4
	inc	dptr
	movx	@dptr,a
	clr	a
	subb	a,r5
	inc	dptr
	movx	@dptr,a
;	_long2fs.c 98
00113$:
	clr	a
	mov	(___long2fs_sloc0_1_0 + 1),a
	mov	___long2fs_sloc0_1_0,#0x96
00105$:
	mov	dptr,#___long2fs_a1_1_1
	movx	a,@dptr
	mov	r4,a
	inc	dptr
	movx	a,@dptr
	mov	r5,a
	inc	dptr
	movx	a,@dptr
	mov	r6,a
	inc	dptr
	movx	a,@dptr
	mov	r7,a
	clr	c
	mov	a,r4
	subb	a,#0x00
	mov	a,r5
	subb	a,#0x00
	mov	a,r6
	subb	a,#0x80
	mov	a,r7
	subb	a,#0x00
; Peephole 108   removed ljmp by inverse jump logic
	jnc  00107$
00116$:
;	_long2fs.c 100
	mov	a,r4
	add	a,acc
	mov	r0,a
	mov	a,r5
	rlc	a
	mov	r1,a
	mov	a,r6
	rlc	a
	mov	r2,a
	mov	a,r7
	rlc	a
	mov	r3,a
	mov	dptr,#___long2fs_a1_1_1
	mov	a,r0
	movx	@dptr,a
	inc	dptr
	mov	a,r1
	movx	@dptr,a
	inc	dptr
	mov	a,r2
	movx	@dptr,a
	inc	dptr
	mov	a,r3
	movx	@dptr,a
;	_long2fs.c 101
	dec	___long2fs_sloc0_1_0
	mov	a,#0xff
	cjne	a,___long2fs_sloc0_1_0,00117$
	dec	(___long2fs_sloc0_1_0 + 1)
00117$:
	ljmp	00105$
00107$:
;	_long2fs.c 104
	anl	ar6,#0x7f
	mov	dptr,#___long2fs_a1_1_1
	mov	a,r4
	movx	@dptr,a
	inc	dptr
	mov	a,r5
	movx	@dptr,a
	inc	dptr
	mov	a,r6
	movx	@dptr,a
	inc	dptr
	mov	a,r7
	movx	@dptr,a
;	_long2fs.c 106
	mov	r2,___long2fs_sloc0_1_0
	mov	r3,(___long2fs_sloc0_1_0 + 1)
	mov	r0,#0x00
	mov	r1,#0x00
	mov	(___long2fs_sloc1_1_0 + 2),r2
	mov	a,r3
	anl	a,#0x01
	mov	c,acc.0
	xch	a,(___long2fs_sloc1_1_0 + 2)
	rrc	a
	xch	a,(___long2fs_sloc1_1_0 + 2)
	rrc	a
	xch	a,(___long2fs_sloc1_1_0 + 2)
	mov	(___long2fs_sloc1_1_0 + 3),a
	mov	(___long2fs_sloc1_1_0 + 1),#0x00
	mov	___long2fs_sloc1_1_0,#0x00
	mov	dptr,#___long2fs_sign_1_1
	movx	a,@dptr
	mov	r2,a
	inc	dptr
	movx	a,@dptr
	mov	r3,a
	inc	dptr
	movx	a,@dptr
	mov	r0,a
	inc	dptr
	movx	a,@dptr
	mov	r1,a
	mov	a,___long2fs_sloc1_1_0
	orl	ar2,a
	mov	a,(___long2fs_sloc1_1_0 + 1)
	orl	ar3,a
	mov	a,(___long2fs_sloc1_1_0 + 2)
	orl	ar0,a
	mov	a,(___long2fs_sloc1_1_0 + 3)
	orl	ar1,a
	mov	a,r2
	orl	ar4,a
	mov	a,r3
	orl	ar5,a
	mov	a,r0
	orl	ar6,a
	mov	a,r1
	orl	ar7,a
	mov	dptr,#(___long2fs_fl_1_1)
	mov	a,r4
	movx	@dptr,a
	inc	dptr
	mov	a,r5
	movx	@dptr,a
	inc	dptr
	mov	a,r6
	movx	@dptr,a
	inc	dptr
	mov	a,r7
	movx	@dptr,a
;	_long2fs.c 108
	mov	dptr,#(___long2fs_fl_1_1)
	movx	a,@dptr
	mov	r2,a
	inc	dptr
	movx	a,@dptr
	mov	r3,a
	inc	dptr
	movx	a,@dptr
	mov	r4,a
	inc	dptr
	movx	a,@dptr
; Peephole 191   removed redundant mov
	mov  r5,a
	mov  dpl,r2
	mov  dph,r3
	mov  b,r4
00108$:
	C$_long2fs.c$109$1$1 ==.
	XG$__long2fs$0$0 ==.
	ret
	.area	CSEG    (CODE)
