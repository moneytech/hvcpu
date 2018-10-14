xor A, A
xor B, B
ldx S, 1000

;does not currently work :)
;look at other examples


ldx A, 2524
call PRINT_INTEGER
hlt
ldx B, PROMPT_STR
call PRINT
in ; get a byte and store it in A
sub A, 48 ; adjust by ascii '0'
call FACTORIAL
ldx  A, C
call PRINT_INTEGER
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

;DIV procedure, input: A/B, remainder in B, quotient in A
DIV:
    xor C, C
    cmp B, 0
    jnz DIV_LOOP
    hlt ;division by 0
DIV_LOOP:
    cmp A, B
    jl  DIV_END
    add C, 1
    sub A, B
    jmp DIV_LOOP
DIV_END:
    ldx B, A
    ldx A, C
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
    ldx A, C
FACTORIAL_END:
    ret
    
;takes string pointer in B
PRINT: 
PRINT_LOOP:
    ldx A, [B]
    and A, 0xFF
    cmp A, 0
    jz PRINT_END
    out
    add B, 1
    jmp PRINT_LOOP

PRINT_END:
    ret

;PRINT_INTEGER procedure, takes integer in A
PRINT_INTEGER:
    ldx B, S
    sub B, 100
    ldx C, 0x000a
    stx [B], C
    call PRINT_SUBPROC
    ret
PRINT_SUBPROC:
    sub B, 1
    cmp A, 10
    jl  PRINT_SUBPROC_END
    push B ;mem ptr
    ldx B, 10
    call DIV
    push A ; quotient
    ldx A, B ;A = remainder
    pop C
    pop B ; mem ptr
    push C ; quotient
    ldx C, [B]
    and C, 0xFF00 ; keep only the previous byte
    add C, 48 ; ascii '0'
    add C, A ; remainder digit
    ldx C, 55 
    stx [B], C
    pop A ; quotient
    call PRINT_SUBPROC
    ret
PRINT_SUBPROC_END:
    ldx C, [B]
    and C, 0xFF00 ; keep only the previous byte (MSb little endian)
    add C, 48 ; ascii '0'
    add C, A ;digit
    stx [B], C
    call PRINT
    hlt
    add S, 1
    ret

    
    
    

PROMPT_STR:
    db "enter a number: ", 0x0a, 0x0
