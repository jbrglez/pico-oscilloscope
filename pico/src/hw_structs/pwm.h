/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_PWM_H
#define _HARDWARE_STRUCTS_PWM_H


typedef struct {
    io_rw_32 csr;
    io_rw_32 div;
    io_rw_32 ctr;
    io_rw_32 cc;
    io_rw_32 top;
} pwm_slice_hw_t;

typedef struct {
    io_rw_32 inte;
    io_rw_32 intf;
    io_ro_32 ints;
} pwm_irq_ctrl_hw_t;

typedef struct {
    pwm_slice_hw_t slice[8];
    io_rw_32 en;
    io_rw_32 intr;
    union {
        struct {
            io_rw_32 inte;

            io_rw_32 intf;

            io_rw_32 ints;
        };
        pwm_irq_ctrl_hw_t irq_ctrl[1];
    };
} pwm_hw_t;

#define pwm_hw ((pwm_hw_t *)PWM_BASE)
// static_assert(sizeof (pwm_hw_t) == 0x00b4, "");

#endif // _HARDWARE_STRUCTS_PWM_H
