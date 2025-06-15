/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_CLOCKS_H
#define _HARDWARE_STRUCTS_CLOCKS_H


/** \brief Clock numbers on RP2040 (used as typedef \ref clock_num_t) 
 *  \ingroup hardware_clocks
 */
typedef enum clock_num_rp2040 {
    clk_gpout0 = 0,
    clk_gpout1 = 1,
    clk_gpout2 = 2,
    clk_gpout3 = 3,
    clk_ref = 4,
    clk_sys = 5,
    clk_peri = 6,
    clk_usb = 7,
    clk_adc = 8,
    clk_rtc = 9,
    CLK_COUNT
} clock_num_t;

/** \brief  Clock destination numbers on RP2040 (used as typedef \ref clock_dest_num_t)
 *  \ingroup hardware_clocks
 */
typedef enum clock_dest_num_rp2040 {
    CLK_DEST_SYS_CLOCKS = 0,
    CLK_DEST_ADC_ADC = 1,
    CLK_DEST_SYS_ADC = 2,
    CLK_DEST_SYS_BUSCTRL = 3,
    CLK_DEST_SYS_BUSFABRIC = 4,
    CLK_DEST_SYS_DMA = 5,
    CLK_DEST_SYS_I2C0 = 6,
    CLK_DEST_SYS_I2C1 = 7,
    CLK_DEST_SYS_IO = 8,
    CLK_DEST_SYS_JTAG = 9,
    CLK_DEST_SYS_VREG_AND_CHIP_RESET = 10,
    CLK_DEST_SYS_PADS = 11,
    CLK_DEST_SYS_PIO0 = 12,
    CLK_DEST_SYS_PIO1 = 13,
    CLK_DEST_SYS_PLL_SYS = 14,
    CLK_DEST_SYS_PLL_USB = 15,
    CLK_DEST_SYS_PSM = 16,
    CLK_DEST_SYS_PWM = 17,
    CLK_DEST_SYS_RESETS = 18,
    CLK_DEST_SYS_ROM = 19,
    CLK_DEST_SYS_ROSC = 20,
    CLK_DEST_RTC_RTC = 21,
    CLK_DEST_SYS_RTC = 22,
    CLK_DEST_SYS_SIO = 23,
    CLK_DEST_PERI_SPI0 = 24,
    CLK_DEST_SYS_SPI0 = 25,
    CLK_DEST_PERI_SPI1 = 26,
    CLK_DEST_SYS_SPI1 = 27,
    CLK_DEST_SYS_SRAM0 = 28,
    CLK_DEST_SYS_SRAM1 = 29,
    CLK_DEST_SYS_SRAM2 = 30,
    CLK_DEST_SYS_SRAM3 = 31,
    CLK_DEST_SYS_SRAM4 = 32,
    CLK_DEST_SYS_SRAM5 = 33,
    CLK_DEST_SYS_SYSCFG = 34,
    CLK_DEST_SYS_SYSINFO = 35,
    CLK_DEST_SYS_TBMAN = 36,
    CLK_DEST_SYS_TIMER = 37,
    CLK_DEST_PERI_UART0 = 38,
    CLK_DEST_SYS_UART0 = 39,
    CLK_DEST_PERI_UART1 = 40,
    CLK_DEST_SYS_UART1 = 41,
    CLK_DEST_SYS_USBCTRL = 42,
    CLK_DEST_USB_USBCTRL = 43,
    CLK_DEST_SYS_WATCHDOG = 44,
    CLK_DEST_SYS_XIP = 45,
    CLK_DEST_SYS_XOSC = 46,
    NUM_CLOCK_DESTINATIONS
} clock_dest_num_t;

typedef struct {
    io_rw_32 ctrl;
    io_rw_32 div;
    io_ro_32 selected;
} clock_hw_t;

typedef struct {
    io_rw_32 ctrl;
    io_ro_32 status;
} clock_resus_hw_t;

typedef struct {
    io_rw_32 ref_khz;
    io_rw_32 min_khz;
    io_rw_32 max_khz;
    io_rw_32 delay;
    io_rw_32 interval;
    io_rw_32 src;
    io_ro_32 status;
    io_ro_32 result;
} fc_hw_t;

typedef struct {
    clock_hw_t clk[10];
    clock_resus_hw_t resus;
    fc_hw_t fc0;
    union {
        struct {
            io_rw_32 wake_en0; 

            io_rw_32 wake_en1; 
        };
        io_rw_32 wake_en[2];
    };
    union {
        struct {
            io_rw_32 sleep_en0; 

            io_rw_32 sleep_en1; 
        };
        io_rw_32 sleep_en[2];
    };
    union {
        struct {
            io_ro_32 enabled0; 

            io_ro_32 enabled1; 
        };
        io_ro_32 enabled[2];
    };
    io_ro_32 intr;
    io_rw_32 inte;
    io_rw_32 intf;
    io_ro_32 ints;
} clocks_hw_t;

#define clocks_hw ((clocks_hw_t *)CLOCKS_BASE)
// static_assert(sizeof (clocks_hw_t) == 0x00c8, "");

#endif // _HARDWARE_STRUCTS_CLOCKS_H
