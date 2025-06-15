/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_RESETS_H
#define _HARDWARE_STRUCTS_RESETS_H


/** \brief Resettable component numbers on RP2040 (used as typedef \ref reset_num_t)
 *  \ingroup hardware_resets
 */
typedef enum reset_num_rp2040 {
    RESET_ADC = 1 << 0,
    RESET_BUSCTRL = 1 << 1,
    RESET_DMA = 1 << 2,
    RESET_I2C0 = 1 << 3,
    RESET_I2C1 = 1 << 4,
    RESET_IO_BANK0 = 1 << 5,
    RESET_IO_QSPI = 1 << 6,
    RESET_JTAG = 1 << 7,
    RESET_PADS_BANK0 = 1 << 8,
    RESET_PADS_QSPI = 1 << 9,
    RESET_PIO0 = 1 << 10,
    RESET_PIO1 = 1 << 11,
    RESET_PLL_SYS = 1 << 12,
    RESET_PLL_USB = 1 << 13,
    RESET_PWM = 1 << 14,
    RESET_RTC = 1 << 15,
    RESET_SPI0 = 1 << 16,
    RESET_SPI1 = 1 << 17,
    RESET_SYSCFG = 1 << 18,
    RESET_SYSINFO = 1 << 19,
    RESET_TBMAN = 1 << 20,
    RESET_TIMER = 1 << 21,
    RESET_UART0 = 1 << 22,
    RESET_UART1 = 1 << 23,
    RESET_USBCTRL = 1 << 24,
    RESET_COUNT
} reset_num_t;

typedef struct {
    io_rw_32 reset;
    io_rw_32 wdsel;
    io_ro_32 reset_done;
} resets_hw_t;

#define resets_hw ((resets_hw_t *)RESETS_BASE)
// static_assert(sizeof (resets_hw_t) == 0x000c, "");

#endif // _HARDWARE_STRUCTS_RESETS_H
