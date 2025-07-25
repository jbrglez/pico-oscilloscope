/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_PADS_BANK0_H
#define _HARDWARE_STRUCTS_PADS_BANK0_H

#define PADS_BANK0_IO_OUTPUT_DISABLE (1u << 7u)
#define PADS_BANK0_IO_INPUT_ENABLE (1u << 6u)
#define PADS_BANK0_IO_DRIVE_2MA  (0u << 4u)
#define PADS_BANK0_IO_DRIVE_4MA  (1u << 4u)
#define PADS_BANK0_IO_DRIVE_8MA  (2u << 4u)
#define PADS_BANK0_IO_DRIVE_12MA (3u << 4u)
#define PADS_BANK0_IO_PULL_UP_EN (1u << 3u)
#define PADS_BANK0_IO_PULL_DOWN_EN (1u << 2u)
#define PADS_BANK0_IO_SCHMITT (1u << 1u)
#define PADS_BANK0_IO_SLEWFAST (1u << 0u)

typedef struct {
    io_rw_32 voltage_select;
    io_rw_32 io[30];
} pads_bank0_hw_t;

#define pads_bank0_hw ((pads_bank0_hw_t *)PADS_BANK0_BASE)
// static_assert(sizeof (pads_bank0_hw_t) == 0x007c, "");

#define padsbank0_hw pads_bank0_hw

#endif // _HARDWARE_STRUCTS_PADS_BANK0_H
