/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_IO_BANK0_H
#define _HARDWARE_STRUCTS_IO_BANK0_H


/**
 * \brief GPIO pin function selectors on RP2040 (used as typedef \ref gpio_function_t) 
 * \ingroup hardware_gpio
 */
typedef enum gpio_function_rp2040 {
    GPIO_FUNC_XIP = 0,
    GPIO_FUNC_SPI = 1,
    GPIO_FUNC_UART = 2,
    GPIO_FUNC_I2C = 3,
    GPIO_FUNC_PWM = 4,
    GPIO_FUNC_SIO = 5,
    GPIO_FUNC_PIO0 = 6,
    GPIO_FUNC_PIO1 = 7,
    GPIO_FUNC_GPCK = 8,
    GPIO_FUNC_USB = 9,
    GPIO_FUNC_NULL = 0x1f,
} gpio_function_t;

typedef struct {
    io_ro_32 status;
    io_rw_32 ctrl;
} io_bank0_status_ctrl_hw_t;

typedef struct {
    io_rw_32 inte[4];
    io_rw_32 intf[4];
    io_ro_32 ints[4];
} io_bank0_irq_ctrl_hw_t;

typedef struct {
    io_bank0_status_ctrl_hw_t io[30];
    io_rw_32 intr[4];
    union {
        struct {
            io_bank0_irq_ctrl_hw_t proc0_irq_ctrl;
            io_bank0_irq_ctrl_hw_t proc1_irq_ctrl;
            io_bank0_irq_ctrl_hw_t dormant_wake_irq_ctrl;
        };
        io_bank0_irq_ctrl_hw_t irq_ctrl[3];
    };
} io_bank0_hw_t;

#define io_bank0_hw ((io_bank0_hw_t *)IO_BANK0_BASE)
// static_assert(sizeof (io_bank0_hw_t) == 0x0190, "");

#define iobank0_hw io_bank0_hw

#endif // _HARDWARE_STRUCTS_IO_BANK0_H
