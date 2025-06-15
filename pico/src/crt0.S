// # RESOURCES:
//   * https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_crt0/crt0.S


.syntax unified
.cpu cortex-m0plus
.thumb


.section .vectors, "ax"

.align 2

// --------------------------------------------------------
//                      NVIC isr-s
// --------------------------------------------------------

#define __StackTop 0x20042000

.global __vectors, __VECTOR_TABLE
__VECTOR_TABLE:
__vectors:
.word __StackTop
.word _reset_handler
.word isr_nmi
.word isr_hardfault
.word isr_invalid // Reserved
.word isr_invalid // Reserved
.word isr_invalid // Reserved
.word isr_invalid // Reserved
.word isr_invalid // Reserved
.word isr_invalid // Reserved
.word isr_invalid // Reserved
.word isr_svcall
.word isr_invalid // Reserved
.word isr_invalid // Reserved
.word isr_pendsv
.word isr_systick

.word __unhandled_user_irq
.word __unhandled_user_irq
.word __unhandled_user_irq
.word __unhandled_user_irq
.word __unhandled_user_irq
.word isr_usbctrl
.word __unhandled_user_irq
.word __unhandled_user_irq
.word __unhandled_user_irq
.word __unhandled_user_irq
.word __unhandled_user_irq
.word isr_dma_0
.word isr_dma_1
.word __unhandled_user_irq
.word __unhandled_user_irq
.word __unhandled_user_irq
.word __unhandled_user_irq
.word __unhandled_user_irq
.word __unhandled_user_irq
.word __unhandled_user_irq
.word __unhandled_user_irq
.word __unhandled_user_irq
.word __unhandled_user_irq
.word __unhandled_user_irq
.word __unhandled_user_irq
.word __unhandled_user_irq
.word __unhandled_user_irq
.word __unhandled_user_irq
.word __unhandled_user_irq
.word __unhandled_user_irq
.word __unhandled_user_irq
.word __unhandled_user_irq

.macro decl_isr_bkpt name
.weak \name
.type \name,%function
.thumb_func
\name:
    bkpt #0
.endm

decl_isr_bkpt isr_invalid
decl_isr_bkpt isr_nmi
decl_isr_bkpt isr_hardfault
decl_isr_bkpt isr_svcall
decl_isr_bkpt isr_pendsv
decl_isr_bkpt isr_systick


.global __unhandled_user_irq
.thumb_func
__unhandled_user_irq:
    mrs  r0, ipsr
    subs r0, #16
.global unhandled_user_irq_num_in_r0
unhandled_user_irq_num_in_r0:
    bkpt #0


// --------------------------------------------------------
//                  Entry point and reset
// --------------------------------------------------------

#define SIO_CPUID 0xd0000000
#define M0PLUS_VTOR 0xe000ed08
#define BOOTROM_VTABLE_OFFSET 0x00

.section .reset, "ax"

// ELF entry point:
.type _entry_point,%function
.thumb_func
.global _entry_point
_entry_point:

    // Debugger tried to run code after loading.
    // Go back through bootrom + boot2 to properly initialise flash.
    ldr r0, =BOOTROM_VTABLE_OFFSET

_enter_vtable_in_r0:
    ldr r1, =(M0PLUS_VTOR)
    str r0, [r1]
    ldmia r0!, {r1, r2}
    msr msp, r1
    bx r2


.type _reset_handler,%function
.thumb_func
_reset_handler:
    // Only core 0 should be running here. We check again to be sure.
    ldr r0, =(SIO_CPUID)
    ldr r0, [r0]
    cmp r0, #0
    beq platform_entry
hold_non_core0_in_bootrom:
    ldr r0, =BOOTROM_VTABLE_OFFSET
    b _enter_vtable_in_r0

platform_entry:
    ldr r1, =runtime_init
    blx r1
    ldr r1, =main
    blx r1
    ldr r1, =exit
    blx r1
    // exit should not return.  If it does, hang the core.
1:
    bkpt #0
    b 1b


.section .data
.global __pico_dbg_str_start__
__pico_dbg_str_start__:
.word __pico_dbg_str_section_start__
