; divide 50 by 5
xor A, A
xor C, C
xor B, B
add A, 50
add B, 5
; 5 instructions above, so this instruction address is (5) * 4 = 20
sub A, B
;jmp if A < 5
add C, 1
cmp A, B
jl 40
jmp 20
; this instruction address is (10) * 4 = 40
; the result will hopefully be in C :), the remainder in A
; you can make this run faster by giving a higher value for cycles, -c 30 for example
hlt
