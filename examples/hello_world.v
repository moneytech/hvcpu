ldx S, 1000 ; have a big enough stack


ldx     A, 5
ldx     B, MSG1
call    PRINTS
hlt

;prints the null termianted string pointed to by B
PRINTS:
    ldx     A, [B]
    and     A, 0x00FF
    jz      PRINTS_END
    out
    add     B, 1
    jmp     PRINTS
PRINTS_END:
    ret

    
MSG1:
    db "hello world", 0xA, 0, 

