// # RESOURCES:
//   * https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/boot_stage2/boot2_generic_03h.S
//     * (boot2_helpers/exit_from_boot2.S):
//        https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/boot_stage2/asminclude/boot2_helpers/exit_from_boot2.S
//     * (pico_default_asm_setup):
//        https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/pico_platform/include/pico/asm_helper.S
//   * https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_crt0/crt0.S
//   * https://github.com/mlugg/microbian/blob/main/pi-pico/boot_stage2.s


.syntax unified
.cpu cortex-m0plus
.thumb

.section .boot2, "ax"
// .section .text


#define M0PLUS_VTOR 0xe000ed08

.global _stage2_boot
.type _stage2_boot,%function
.thumb_func
_stage2_boot:
    push {lr}

    // NOTE:
    // We enable the flash, copy everything to SRAM and then disable the flash.
    // Since we don't use flash/XIP, 16KB of SRAM at 0x15000000,
    // that would other wise be used as XIP cache, is now free for use.
    bl enable_xip
    bl copy_flash_to_ram
    bl disable_xip

check_return:
    pop {r0}
    cmp r0, #0
    beq vector_into_crt0
    bx r0
vector_into_crt0:
    ldr r0, =__vectors
    ldr r1, =(M0PLUS_VTOR)
    str r0, [r1]
    ldmia r0, {r0, r1}
    msr msp, r0
    bx r1



#define XIP_SSI_BASE 0x18000000
#define SPI_CTRLR0_XIP (3 << 24) |  (2 << 8) | (6 << 2) | (0 << 0)
#define CTRLR0_XIP (0 << 21) | (31 << 16)  | (3  << 8)

enable_xip:
    push {lr}

    ldr r3, =XIP_SSI_BASE

    movs r1, #0
    str r1, [r3, #0x8]

    movs r1, #4
    str r1, [r3, #0x14]

    ldr r1, =(CTRLR0_XIP)
    str r1, [r3, #0]

    ldr r1, =(SPI_CTRLR0_XIP)
    ldr r0, =(XIP_SSI_BASE + 0xf4)
    str r1, [r0]

    movs r1, #0x0
    str r1, [r3, #4]

    movs r1, #1
    str r1, [r3, #8]

    pop {pc}



disable_xip:
    push {lr}
    ldr r0, =XIP_SSI_BASE
    movs r1, #0
    str r1, [r0, #0x08]
    pop {pc}



copy_flash_to_ram:
    push {lr}
    adr r4, data_cpy_table
1:
    ldmia r4!, {r0-r2}
    cmp r0, #0
    beq 2f
    bl data_cpy16
    b 1b
2:
    ldr r0, =__pico_bss_start__
    ldr r1, =__pico_bss_end__
    push {r4-r5}
    movs r2, #0
    movs r3, #0
    movs r4, #0
    movs r5, #0
    b bss_fill_test
bss_fill_loop:
    stm r0!, {r2, r3, r4, r5}
bss_fill_test:
    cmp r0, r1
    bne bss_fill_loop
    pop {r4-r5}
    pop {pc}



data_cpy16:
    push {lr}
    push {r4-r6}
    b 1f
data_cpy16_loop:
    ldmia r0!, {r3, r4, r5, r6}
    stmia r1!, {r3, r4, r5, r6}
1:
    cmp r1, r2
    blo data_cpy16_loop
    pop {r4-r6}
    pop {pc}



.align 2
data_cpy_table:

.word __pico_nvic_source__
.word __pico_nvic_start__
.word __pico_nvic_end__

.word __pico_text_source__
.word __pico_text_start__
.word __pico_text_end__

.word __pico_rodata_source__
.word __pico_rodata_start__
.word __pico_rodata_end__

.word __pico_data_source__
.word __pico_data_start__
.word __pico_data_end__

// .word __scratch_x_source__
// .word __scratch_x_start__
// .word __scratch_x_end__
//
// .word __scratch_y_source__
// .word __scratch_y_start__
// .word __scratch_y_end__

.word 0 // null terminator

