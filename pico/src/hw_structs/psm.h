/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_PSM_H
#define _HARDWARE_STRUCTS_PSM_H


typedef struct {
    io_rw_32 frce_on;
    io_rw_32 frce_off;
    io_rw_32 wdsel;
    io_ro_32 done;
} psm_hw_t;

#define psm_hw ((psm_hw_t *)PSM_BASE)
// static_assert(sizeof (psm_hw_t) == 0x0010, "");

#endif // _HARDWARE_STRUCTS_PSM_H
