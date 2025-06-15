#ifndef _HARDWARE_H
#define _HARDWARE_H


#include "my_types.h"


#define ROM_BASE                        0x00000000

#define XIP_BASE                        0x10000000
#define XIP_MAIN_BASE                   0x10000000
#define XIP_NOALLOC_BASE                0x11000000
#define XIP_NOCACHE_BASE                0x12000000
#define XIP_NOCACHE_NOALLOC_BASE        0x13000000
#define XIP_CTRL_BASE                   0x14000000
#define XIP_SRAM_BASE                   0x15000000
#define XIP_SRAM_END                    0x15004000
#define XIP_SSI_BASE                    0x18000000

#define SRAM_BASE                       0x20000000
#define SRAM_STRIPED_BASE               0x20000000
#define SRAM_STRIPED_END                0x20040000
#define SRAM4_BASE                      0x20040000
#define SRAM5_BASE                      0x20041000
#define SRAM_END                        0x20042000

#define SRAM0_BASE                      0x21000000
#define SRAM1_BASE                      0x21010000
#define SRAM2_BASE                      0x21020000
#define SRAM3_BASE                      0x21030000

#define SYSINFO_BASE                    0x40000000
#define SYSCFG_BASE                     0x40004000
#define CLOCKS_BASE                     0x40008000
#define RESETS_BASE                     0x4000c000
#define PSM_BASE                        0x40010000
#define IO_BANK0_BASE                   0x40014000
#define IO_QSPI_BASE                    0x40018000
#define PADS_BANK0_BASE                 0x4001c000
#define PADS_QSPI_BASE                  0x40020000
#define XOSC_BASE                       0x40024000
#define PLL_SYS_BASE                    0x40028000
#define PLL_USB_BASE                    0x4002c000
#define BUSCTRL_BASE                    0x40030000
#define UART0_BASE                      0x40034000
#define UART1_BASE                      0x40038000
#define SPI0_BASE                       0x4003c000
#define SPI1_BASE                       0x40040000
#define I2C0_BASE                       0x40044000
#define I2C1_BASE                       0x40048000
#define ADC_BASE                        0x4004c000
#define PWM_BASE                        0x40050000
#define TIMER_BASE                      0x40054000
#define WATCHDOG_BASE                   0x40058000
#define RTC_BASE                        0x4005c000
#define ROSC_BASE                       0x40060000
#define VREG_AND_CHIP_RESET_BASE        0x40064000
#define TBMAN_BASE                      0x4006c000

#define DMA_BASE                        0x50000000
#define USBCTRL_DPRAM_BASE              0x50100000
#define USBCTRL_BASE                    0x50100000
#define USBCTRL_REGS_BASE               0x50110000
#define PIO0_BASE                       0x50200000
#define PIO1_BASE                       0x50300000
#define XIP_AUX_BASE                    0x50400000

#define SIO_BASE                        0xd0000000
#define PPB_BASE                        0xe0000000


#define REG_ALIAS_RW     0x0000
#define REG_ALIAS_XOR    0x1000
#define REG_ALIAS_SET    0x2000
#define REG_ALIAS_CLR    0x3000


#define DPRAM_OFFSET(addr) ((u32)(addr) - USBCTRL_DPRAM_BASE)



#define __force_inline inline 


__force_inline internal void hw_set_bits(io_rw_32 *addr, uint32_t mask) {
    *(io_rw_32 *) ((void *)(REG_ALIAS_SET + (uintptr_t)((volatile void *) addr))) = mask;
}

__force_inline internal void hw_clear_bits(io_rw_32 *addr, uint32_t mask) {
    *(io_rw_32 *) ((void *)(REG_ALIAS_CLR + (uintptr_t)((volatile void *) addr))) = mask;
}

__force_inline internal void hw_xor_bits(io_rw_32 *addr, uint32_t mask) {
    *(io_rw_32 *) ((void *)(REG_ALIAS_XOR + (uintptr_t)((volatile void *) addr))) = mask;
}

__force_inline internal void hw_write_masked(io_rw_32 *addr, uint32_t values, uint32_t write_mask) {
    hw_xor_bits(addr, (*addr ^ values) & write_mask);
}


typedef enum irq_num_rp2040 {
    TIMER_IRQ_0     = (1 << 0),
    TIMER_IRQ_1     = (1 << 1),
    TIMER_IRQ_2     = (1 << 2),
    TIMER_IRQ_3     = (1 << 3),
    PWM_IRQ_WRAP    = (1 << 4),
    USBCTRL_IRQ     = (1 << 5),
    XIP_IRQ         = (1 << 6),
    PIO0_IRQ_0      = (1 << 7),
    PIO0_IRQ_1      = (1 << 8),
    PIO1_IRQ_0      = (1 << 9),
    PIO1_IRQ_1      = (1 << 10),
    DMA_IRQ_0       = (1 << 11),
    DMA_IRQ_1       = (1 << 12),
    IO_IRQ_BANK0    = (1 << 13),
    IO_IRQ_QSPI     = (1 << 14),
    SIO_IRQ_PROC0   = (1 << 15),
    SIO_IRQ_PROC1   = (1 << 16),
    CLOCKS_IRQ      = (1 << 17),
    SPI0_IRQ        = (1 << 18),
    SPI1_IRQ        = (1 << 19),
    UART0_IRQ       = (1 << 20),
    UART1_IRQ       = (1 << 21),
    ADC_IRQ_FIFO    = (1 << 22),
    I2C0_IRQ        = (1 << 23),
    I2C1_IRQ        = (1 << 24),
    RTC_IRQ         = (1 << 25),
    IRQ_COUNT
} irq_num_t;


#include "hw_structs/adc.h"
#include "hw_structs/busctrl.h"
#include "hw_structs/clocks.h"
#include "hw_structs/dma.h"
#include "hw_structs/dma_debug.h"
#include "hw_structs/i2c.h"
#include "hw_structs/interp.h"
#include "hw_structs/io_bank0.h"
#include "hw_structs/io_qspi.h"
#include "hw_structs/m0plus.h"
#include "hw_structs/mpu.h"
#include "hw_structs/nvic.h"
#include "hw_structs/pads_bank0.h"
#include "hw_structs/pads_qspi.h"
#include "hw_structs/pio.h"
#include "hw_structs/pll.h"
#include "hw_structs/psm.h"
#include "hw_structs/pwm.h"
#include "hw_structs/resets.h"
#include "hw_structs/rosc.h"
#include "hw_structs/rtc.h"
#include "hw_structs/scb.h"
#include "hw_structs/sio.h"
#include "hw_structs/spi.h"
#include "hw_structs/ssi.h"
#include "hw_structs/syscfg.h"
#include "hw_structs/sysinfo.h"
#include "hw_structs/systick.h"
#include "hw_structs/tbman.h"
#include "hw_structs/timer.h"
#include "hw_structs/uart.h"
#include "hw_structs/usb.h"
#include "hw_structs/usb_dpram.h"
#include "hw_structs/vreg_and_chip_reset.h"
#include "hw_structs/watchdog.h"
#include "hw_structs/xip.h"
#include "hw_structs/xip_ctrl.h"
#include "hw_structs/xosc.h"


#endif
