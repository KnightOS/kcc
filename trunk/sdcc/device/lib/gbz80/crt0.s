	;; Generic crt0.s for a Z80
	.globl	__main

	.area _HEADER (ABS)
	;; Reset vector
	.org 	0
	jp	init

	.org	0x08
	reti
	.org	0x10
	reti
	.org	0x18
	reti
	.org	0x20
	reti
	.org	0x28
	reti
	.org	0x30
	reti
	.org	0x38
	reti

	.org	0x100
	jp	0x150
		
	.org	0x150
init:
	di
	;; Stack at the top of memory.
	ld	sp,#0xdfff        

	;; Use _main instead of main to bypass sdcc's intelligence
	call	__main
	jp	_exit

	;; Ordering of segments for the linker.
	.area	_CODE
	.area	_DATA

__clock::
	ld	a,#2
	rst	0x00
	ret
	
_getsp::
	ld	hl,#0
	add	hl,sp
	ret

__printTStates::	
	ld	a,#3
	rst	0x00
	ret
		
_exit::
	;; Exit - special code to the emulator
	ld	a,#1
	rst	0x00
1$:
	halt
	jr	1$
