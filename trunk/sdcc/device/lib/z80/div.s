        ;; Originally from GBDK by Pascal Felber.

        .area   _CODE

        ;; Unsigned
__divuchar_rrx_s::
        ld      hl,#2+1
        add     hl,sp

        ld      e,(hl)
        dec     hl
        ld      l,(hl)

        ;; Fall through
__divuchar_rrx_hds::
        ld      c,l
        call    __divu8

        ld      l,c
        ld      h,b

        ret

__divuint_rrx_s::
        ld      hl,#2+3
        add     hl,sp

        ld      d,(hl)
        dec     hl
        ld      e,(hl)
        dec     hl
        ld      a,(hl)
        dec     hl
        ld      l,(hl)
        ld      h,a

        ;; Fall through
__divuint_rrx_hds::
        ld      b,h
        ld      c,l
        call    __divu16

        ld      l,c
        ld      h,b

        ret

__divsuchar_rrx_s::
        ld      hl,#2+1
        add     hl,sp

        ld      e,(hl)
        dec     hl
        ld      c,(hl)
        ld      b, #0

        call    signexte

        ld      l,c
        ld      h,b

        ret

__modsuchar_rrx_s::
        ld      hl,#2+1
        add     hl,sp

        ld      e,(hl)
        dec     hl
        ld      c,(hl)
        ld      b, #0

        call    signexte

        ld      l,e
        ld      h,d

        ret

__divuschar_rrx_s::
        ld      hl,#2+1
        add     hl,sp

        ld      e,(hl)
        ld      d, #0
        dec     hl
        ld      c,(hl)

        ld      a,c             ; Sign extend
        rlca
        sbc     a
        ld      b,a

        call    __div16

        ld      l,c
        ld      h,b

        ret

__div8::
.mod8::
        ld      a,c             ; Sign extend
        rlca
        sbc     a
        ld      b,a
signexte:
        ld      a,e             ; Sign extend
        rlca
        sbc     a
        ld      d,a

        ; Fall through to __div16

        ;; 16-bit division
        ;;
        ;; Entry conditions
        ;;   BC = dividend
        ;;   DE = divisor
        ;;
        ;; Exit conditions
        ;;   BC = quotient
        ;;   DE = remainder
        ;;   If divisor is non-zero, carry=0
        ;;   If divisor is 0, carry=1 and both quotient and remainder are 0
        ;;
        ;; Register used: AF,BC,DE,HL
__div16::
.mod16::
        ;; Determine sign of quotient by xor-ing high bytes of dividend
        ;;  and divisor. Quotient is positive if signs are the same, negative
        ;;  if signs are different
        ;; Remainder has same sign as dividend
        ld      a,b             ; Get high byte of dividend
        push    af              ; Save as sign of remainder
        xor     d               ; Xor with high byte of divisor
        push    af              ; Save sign of quotient

        ;; Take absolute value of divisor
        bit     7,d
        jr      Z,.chkde        ; Jump if divisor is positive
        sub     a               ; Substract divisor from 0
        sub     e
        ld      e,a
        sbc     a               ; Propagate borrow (A=0xFF if borrow)
        sub     d
        ld      d,a
        ;; Take absolute value of dividend
.chkde:
        bit     7,b
        jr      Z,.dodiv        ; Jump if dividend is positive
        sub     a               ; Substract dividend from 0
        sub     c
        ld      c,a
        sbc     a               ; Propagate borrow (A=0xFF if borrow)
        sub     b
        ld      b,a
        ;; Divide absolute values
.dodiv:
        call    __divu16
        jr      C,.exit         ; Exit if divide by zero
        ;; Negate quotient if it is negative
        pop     af              ; recover sign of quotient
        and     #0x80
        jr      Z,.dorem        ; Jump if quotient is positive
        sub     a               ; Substract quotient from 0
        sub     c
        ld      c,a
        sbc     a               ; Propagate borrow (A=0xFF if borrow)
        sub     b
        ld      b,a
.dorem:
        ;; Negate remainder if it is negative
        pop     af              ; recover sign of remainder
        and     #0x80
        ret     Z               ; Return if remainder is positive
        sub     a               ; Substract remainder from 0
        sub     e
        ld      e,a
        sbc     a               ; Propagate remainder (A=0xFF if borrow)
        sub     d
        ld      d,a
        ret
.exit:
        pop     af
        pop     af
        ret

__divu8::
.modu8::
        ld      b,#0x00
        ld      d,b
        ; Fall through to divu16

__divu16::
.modu16::
        ;; Check for division by zero
        ld      a,e
        or      d
        jr      NZ,.divide      ; Branch if divisor is non-zero
        ld      bc,#0x00        ; Divide by zero error
        ld      d,b
        ld      e,c
        scf                     ; Set carry, invalid result
        ret
.divide:
        ld      hl,#0
;       ld      l,c             ; L = low byte of dividend/quotient
;       ld      h,b             ; H = high byte of dividend/quotient
;       ld      bc,#0x00        ; BC = remainder
        or      a               ; Clear carry to start
        ex      af,af'
        ld      a,#16           ; 16 bits in dividend
.dvloop:
        ;; Shift next bit of quotient into bit 0 of dividend
        ;; Shift next MSB of dividend into LSB of remainder
        ;; BC holds both dividend and quotient. While we shift a bit from
        ;;  MSB of dividend, we shift next bit of quotient in from carry
        ;; HL holds remainder
        ;; Do a 32-bit left shift, shifting carry to L, L to H,
        ;;  H to C, C to B
        ex      af,af'
        rl      c               ; Carry (next bit of quotient) to bit 0
        rl      b               ; Clears carry since BC was 0
        adc     hl,hl

        ;; If remainder is >= divisor, next bit of quotient is 1. This
        ;;  bit goes to carry
        push    hl              ; Save current remainder
        sbc     hl,de
;       ld      a,c             ; Substract divisor from remainder
;       sbc     e
;       ld      c,a
;       ld      a,b
;       sbc     d
;       ld      b,a
        ccf                     ; Complement borrow so 1 indicates a
                                ;  successful substraction (this is the
                                ;  next bit of quotient)
        jr      C,.drop         ; Jump if remainder is >= dividend
        pop     hl              ; Otherwise, restore remainder
        jr      .nodrop
.drop:
        inc     sp
        inc     sp
.nodrop:
        ex      af,af'
        dec     a               ; DEC does not affect carry flag
        jp      NZ,.dvloop
        ex      af,af'
        ;; Shift last carry bit into quotient
        ld      d,h             ; DE = remainder
        ld      e,l
        rl      c               ; Carry to L
;       ld      c,l             ; C = low byte of quotient
        rl      b
;       ld      b,h             ; B = high byte of quotient
        or      a               ; Clear carry, valid result
        ret

