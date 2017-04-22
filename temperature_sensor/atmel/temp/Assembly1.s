
.section .text
.global delay_usec

delay_usec: sbiw r24, 1
            breq isOne
            jmp overOne
isOne:      nop
            ret
overOne:    call addCycles
            jmp delay_usec
addCycles:  ret