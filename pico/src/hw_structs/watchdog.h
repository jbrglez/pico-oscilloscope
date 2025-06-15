/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_WATCHDOG_H
#define _HARDWARE_STRUCTS_WATCHDOG_H


typedef struct {
    io_rw_32 ctrl;
    io_wo_32 load;
    io_ro_32 reason;
    io_rw_32 scratch[8];
    io_rw_32 tick;
} watchdog_hw_t;

#define watchdog_hw ((watchdog_hw_t *)WATCHDOG_BASE)
// static_assert(sizeof (watchdog_hw_t) == 0x0030, "");

#endif // _HARDWARE_STRUCTS_WATCHDOG_H
