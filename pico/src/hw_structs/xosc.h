/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_XOSC_H
#define _HARDWARE_STRUCTS_XOSC_H


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
