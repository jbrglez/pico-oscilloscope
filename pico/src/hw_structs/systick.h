/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_SYSTICK_H
#define _HARDWARE_STRUCTS_SYSTICK_H


typedef struct {
    io_rw_32 csr;
    io_rw_32 rvr;
    io_rw_32 cvr;
    io_ro_32 calib;
} systick_hw_t;

#define systick_hw ((systick_hw_t *)(PPB_BASE + M0PLUS_SYST_CSR_OFFSET))
// static_assert(sizeof (systick_hw_t) == 0x0010, "");

#endif // _HARDWARE_STRUCTS_SYSTICK_H
