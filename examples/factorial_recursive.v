
xor A, A
xor B, B
xor S, S
add S, 1000 ; have a big enough stack


add A, 5
call FACTORIAL
hlt



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
FACTORIAL:
    cmp  A, 1
    jle FACTORIAL_END
    push A
    sub  A, 1
    call FACTORIAL
    pop  B
    call MUL
    xor  A, A
    add  A, C
FACTORIAL_END:
    ret
    
