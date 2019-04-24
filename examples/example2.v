; adds a bunch of numbers
; expected result: B == 48
;                  A will overflow somehow
xor A, A
xor B, B
add A, 4
add A, 4
add A, 4
add A, 4
add B, A
add B, A
add B, A
xor A, A
add A, 0x4000
add A, 0x4000
add A, 0x4000
add A, 0x4000
add A, 0x4000
hlt

