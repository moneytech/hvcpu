TEST_1:
TEST_2:
;set stack pointer
xor S, S
TEST_3:
add S, 400
;--RELATIVE JUMPS DEPRECATED DUE TO SAFETY CONCERNS, DONT USE THEM--
;--USE LABELS INSTEAD--

jmp ENTRY_POINT

;mul procedure, takes operands in A and B
MUL: 
    xor C, C
MUL_LOOP:
    add C, B
    sub A, 1
    jz MUL_RET
    jmp MUL_LOOP
MUL_RET:
    pop IP ;return

ENTRY_POINT:
    xor A, A
    xor B, B
    xor C, C
    add C, 64; 
    push C
    add A, 7 ;
    add B, 5
    ;call MUL
    jmp MUL
    hlt 
