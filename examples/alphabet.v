xor A, A
xor B, B
ldx S, 1000 ; have a big enough stack



add     A, 5
ldx     B, MSG1
call    PRINTS
hlt

;prints the null termianted string pointed to by B
PRINTS:
    ldx     A, [B]
    and     A, 0x00FF
    cmp     A, 0
    jz      PRINTS_END
    out
    add     B, 1
    jmp     PRINTS
PRINTS_END:
    ret

    


;this should probably be implemented by shiftand addition :), it takes forever to execute with larger numbers
;MUL procedure RETURNS IN C
MUL:
    xor C, C
MUL_LOOP:
    add C, B
    sub A, 1
    jz MUL_RET
    jmp MUL_LOOP
MUL_RET:
    ret


;(5 * (4 * (3 * (2 * 1))))
;this is an iterative version, the recursive version is left as an excercise for the reader
;and for the love of your actual cpu, don't try to calculate 6 factorial!
FACTORIAL:
    xor B, B
    add B, A
FACT_LOOP:
    sub B, 1
    cmp B, 0
    je  FACTORIAL_END
    call MUL
    xor A, A
    add A, C
    jmp FACT_LOOP
FACTORIAL_END:
    ret

    
MSG1:
    db "hello world",0xA,97, 0, 

