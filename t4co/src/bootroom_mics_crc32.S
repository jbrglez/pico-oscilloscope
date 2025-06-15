// Snippet from: https://github.com/raspberrypi/pico-bootrom-rp2040/blob/master/bootrom/bootrom_misc.S

// This code is here just for reference and is not used by t4co programs.

// -------------------------------------

// Each byte digested MSB-first
// r0: start pointer
// r1: length in bytes
// r2: checksum seed value

.global crc32_small
.type crc32_small,%function
.thumb_func
crc32_small:
    push {r4, r5, lr}
    // r1 now end
    add r1, r0
    ldr  r5, =#0x4c11db7

    b byte_loop_test
byte_loop:
    ldrb r4, [r0]

    rev  r3, r2
    eor  r4, r3
    lsl  r4, #24

    mov r3, #8
bit_loop:
    lsl r4, #1
    bcc no_bit
    eor r4, r5
no_bit:
    sub r3, #1
    bne bit_loop

    lsl r2, #8
    eor r2, r4

    add r0, #1
byte_loop_test:
    cmp r0, r1
    blt byte_loop

    mov r0, r2
    pop {r4, r5, pc}
