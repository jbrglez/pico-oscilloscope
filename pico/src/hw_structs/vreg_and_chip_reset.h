/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_VREG_AND_CHIP_RESET_H
#define _HARDWARE_STRUCTS_VREG_AND_CHIP_RESET_H


typedef struct {
    io_rw_32 vreg;
    io_rw_32 bod;
    io_rw_32 chip_reset;
} vreg_and_chip_reset_hw_t;

#define vreg_and_chip_reset_hw ((vreg_and_chip_reset_hw_t *)VREG_AND_CHIP_RESET_BASE)
// static_assert(sizeof (vreg_and_chip_reset_hw_t) == 0x000c, "");

#endif // _HARDWARE_STRUCTS_VREG_AND_CHIP_RESET_H
