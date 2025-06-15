/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_ROSC_H
#define _HARDWARE_STRUCTS_ROSC_H


typedef struct {
    io_rw_32 ctrl;
    io_rw_32 freqa;
    io_rw_32 freqb;
    io_rw_32 dormant;
    io_rw_32 div;
    io_rw_32 phase;
    io_rw_32 status;
    io_ro_32 randombit;
    io_rw_32 count;
} rosc_hw_t;

#define rosc_hw ((rosc_hw_t *)ROSC_BASE)
// static_assert(sizeof (rosc_hw_t) == 0x0024, "");

#endif // _HARDWARE_STRUCTS_ROSC_H
