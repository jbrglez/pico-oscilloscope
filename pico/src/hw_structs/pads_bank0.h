/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_PADS_BANK0_H
#define _HARDWARE_STRUCTS_PADS_BANK0_H


typedef struct {
    io_rw_32 voltage_select;
    io_rw_32 io[30];
} pads_bank0_hw_t;

#define pads_bank0_hw ((pads_bank0_hw_t *)PADS_BANK0_BASE)
// static_assert(sizeof (pads_bank0_hw_t) == 0x007c, "");

#define padsbank0_hw pads_bank0_hw

#endif // _HARDWARE_STRUCTS_PADS_BANK0_H
