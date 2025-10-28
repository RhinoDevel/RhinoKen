; (Optional) constant definitions are first, by definition:

reg_a =   ; This is the register A's address in memory.
          ; This is another comment.
    0     ; This is OK.

reg_b ; This is also OK.
   = 1
   
reg_x 
   = 
 2 ; Even this is alright.

reg_p = 3

* = reg_x ; (Optional) location counter assignm. (must be a value or constant).

.bytes

    2    ; Register X.
    
    main ; Register P.

main:

    add   a #0011     ; Constant or immediate: A             = A + 0011
    sub   b 0022      ; Memory:                B             = B - mem[0022]
    load  x (0033)    ; Indirect or deferred:  X             = mem[mem[0033]]
    store a 0044, x   ; Indexed:               mem[0004 + X] = A
    add   b (0055), x ; Indirect indexed:      B             = B + mem[mem[0055] + X]
    
loop_a:

    or    #$a7        ; Constant or immediate: A             = A | $a7
    and   123         ; Memory:                A             = A & mem[123]
    lneg  (0033)      ; Indirect or deferred:  A             = 0 - mem[mem[0033]]
    ; noop  ... ; Allowed with all "addressing modes" to get assembled as 2 bytes.

    jpd   ne a 0022   ; A != 0 ? 
    jpi   eq b 0033   ; B == 0 ?
    jmd   lt x 0022   ; X  < 0 ?
    jmi        0033   ; Unc.
    jpd   ge a loop_a ; A >= 0 ?
    jpi   gt b 0033   ; B  > 0 ?

    set   0 7 0033
    set   1 6 0033
    skp   0 5 reg_b
    skp   1 4 0033

    sftl  1 a
    sftr  2 b
    rotl  3 a
    rotr  4 b

    halt
    noop ; The 1-byte instruction version.