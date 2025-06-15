/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_INTERP_H
#define _HARDWARE_STRUCTS_INTERP_H


typedef struct {
    io_rw_32 accum[2];
    io_rw_32 base[3];
    io_ro_32 pop[3];
    io_ro_32 peek[3];
    io_rw_32 ctrl[2];
    io_rw_32 add_raw[2];
    io_wo_32 base01;
} interp_hw_t;

#define interp_hw_array ((interp_hw_t *)(SIO_BASE + SIO_INTERP0_ACCUM0_OFFSET))
// static_assert(sizeof (interp_hw_t) == 0x0040, "");
#define interp0_hw (&interp_hw_array[0])
#define interp1_hw (&interp_hw_array[1])

#endif // _HARDWARE_STRUCTS_INTERP_H
