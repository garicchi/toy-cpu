;; A program for calculating sum from 1 to value of r3
ldh r0, 0x00
ldl r0, 0x00
ldh r1, 0x00
ldl r1, 0x01  ; A first number of sum
ldh r2, 0x00
ldl r2, 0x00
ldh r3, 0x00
ldl r3, 0x03  ; A last number of sum

;; loop until r2 and r3 set same value
loop:
add r2, r1
add r0, r2
st r0, 0x64  ; store result to 0x64 address
cmp r2, r3
je else
jmp loop
else:
hlt
