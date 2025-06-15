/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_IO_QSPI_H
#define _HARDWARE_STRUCTS_IO_QSPI_H


/**
 * \brief QSPI pin function selectors on RP2040 (used as typedef \ref gpio_function1_t) 
 */
typedef enum gpio_function1_rp2040 {
    GPIO_FUNC1_XIP = 0,
    GPIO_FUNC1_SIO = 5,
    GPIO_FUNC1_NULL = 0x1f,
} gpio_function1_t;

typedef struct {
    io_ro_32 status;
    io_rw_32 ctrl;
} io_qspi_status_ctrl_hw_t;

typedef struct {
    io_rw_32 inte;
    io_rw_32 intf;
    io_ro_32 ints;
} io_qspi_irq_ctrl_hw_t;

typedef struct {
    io_qspi_status_ctrl_hw_t io[6];
    io_rw_32 intr;
    union {
        struct {
            io_qspi_irq_ctrl_hw_t proc0_irq_ctrl;
            io_qspi_irq_ctrl_hw_t proc1_irq_ctrl;
            io_qspi_irq_ctrl_hw_t dormant_wake_irq_ctrl;
        };
        io_qspi_irq_ctrl_hw_t irq_ctrl[3];
    };
} io_qspi_hw_t;

#define io_qspi_hw ((io_qspi_hw_t *)IO_QSPI_BASE)
// static_assert(sizeof (io_qspi_hw_t) == 0x0058, "");

#define ioqspi_hw io_qspi_hw

#endif // _HARDWARE_STRUCTS_IO_QSPI_H
