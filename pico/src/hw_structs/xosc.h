/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_XOSC_H
#define _HARDWARE_STRUCTS_XOSC_H


#define XOSC_STATUS_STABLE (1u<<31u)

#define XOSC_CTRL_ENABLE (0xFAB<<12u)
#define XOSC_CTRL_DISABLE (0xD1E<<12u)
#define XOSC_CTRL_FREQ_RANGE_1MHZ_15MHZ (0xAA0<<0u)

#define XOSC_STARTUP_DELAY_LSB 0u

typedef struct {
    io_rw_32 ctrl;
    io_rw_32 status;
    io_rw_32 dormant;
    io_rw_32 startup;
    uint32_t _pad0[3];
    io_rw_32 count;
} xosc_hw_t;

#define xosc_hw ((xosc_hw_t *)XOSC_BASE)
// static_assert(sizeof (xosc_hw_t) == 0x0020, "");

#endif // _HARDWARE_STRUCTS_XOSC_H
