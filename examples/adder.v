;
; a program that gets characters from std input
; converts them to numbers, adds them and 'prints' the result
;

ldx S, 2048 ; have a big and far enough stack

ldx     A, 5
ldx     B, S
sub     B, 500 ;a buffer to store things in
push    B ;buff


ldx     B, MSG1
call    PRINTS
ldx     B, [S] ;buff
call    GETS
ldx     B, [S]
call    ATOI
push    C ;return value

ldx     B, MSG2
call    PRINTS
add     S, 2
ldx     B, [S] ;buff
add     B, 50
sub     S, 2
call    GETS
add     S, 2
ldx     B, [S]
add     B, 50
sub     S, 2
call    ATOI
;return value is in C

pop A
add A, C ; result
push A
ldx B, MSG3 
call PRINTS
pop A
push A
call PRINT_INTEGER
pop A
hlt

;to debug add this anywhere while running in -v mode, it doesnt modify the state of whats surrounding it
    push A
    in
    pop A
;

;prints the null termianted string pointed to by B
PRINTS:
    PPRINTS:
    ldx     A, [B]
    and     A, 0x00FF
    cmp     A, 0
    jz      PRINTS_END
    out
    add     B, 1
    jmp     PPRINTS
PRINTS_END:
    ret

;stores the string entered by the user to B
GETS:
    in
    cmp A, 0x0A
    jz  GETS_END
    stx [B], A
    add B, 1
    jmp GETS
GETS_END:
    stx [B], A ;newline
    add B, 1
    xor A, A
    stx [B], A
    ret

;does a lot of stuff :)
IS_DIGIT:
    ldx A, [B]
    and A, 0x00FF
    cmp A, 48 ;ascii 0
    jb  NOT_DIGIT
    cmp A, 57
    ja  NOT_DIGIT
    sub A, 48 ;convert to digit
    add B, 1
    ret
    NOT_DIGIT:
    ldx A, 100 ;sentinel value to return if not digit
    ret


;mul procedure, takes operands in A and B
;returns in C
MUL: 
    xor C, C
    cmp A, 0
    jz MUL_RET
MUL_LOOP:
    add C, B
    sub A, 1
    jnz MUL_LOOP
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



;converts the string at B to an integer in A
;why is it so long? idk, 
;maybe this is the reason cpus usually have more than 3 registers :)
ATOI:
    push B ;begin ptr
    call IS_DIGIT
    cmp A, 100 ; (100 is a special value) 
    jne PRE_ATOI_ITER
    xor A, A ;else it's an empty string / not a number
    pop B
    ret 
PRE_ATOI_ITER:
    sub B, 1
ATOI_ITER:
    call IS_DIGIT
    cmp A, 100
    jne ATOI_ITER

    sub B, 1 ;ignore last byte
    ldx C, 1 ; base variable
    push C
    ldx C, 0 ;accumulator
ATOI_REV:
    call IS_DIGIT ;preserves C, adds 1 to B, A = digit
    push B ;ptr
    push C ;accumulator
    add S, 4
    ldx B, [S] ;base var
    sub S, 4
    call MUL
    ldx A, C ; A = result
    pop C ;accum
    add C, A ;C += result
    push C ;accumlator
    ldx A, 10
    call MUL
    add S, 4
    stx [S], C ;update our base to be base = base * 10
    sub S, 4
    pop C ;accumulator
    pop B ;ptr
    sub B, 2 ;IS_DIGIT itself adds one, we subtract two to effectively subtract one
    ;whats at the stack-4 atm is the begin ptr
    add S, 2
    ldx A, [S]
    sub S, 2
    cmp B, A
    jb ATOI_END ;if less then we reached the beginning
    jmp ATOI_REV
ATOI_END:
    add S, 4 ;clean up the stack
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
    pop C ; C = quotient
    pop B ; mem ptr
    push C ; quotient
    ldx C, [B]
    and C, 0xFF00 ; keep only the previous byte
    add C, 48 ; ascii '0'
    add C, A ; remainder digit
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
    call PRINTS
    hlt
    add S, 1
    ret





    
MSG2: db 0xA
MSG1:
      db "enter a number:", 0xA, 0, 
MSG3: db "n1 + n2 = ",0xA, 0

