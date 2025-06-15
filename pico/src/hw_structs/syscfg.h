/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_SYSCFG_H
#define _HARDWARE_STRUCTS_SYSCFG_H


typedef struct {
    io_rw_32 proc0_nmi_mask;
    io_rw_32 proc1_nmi_mask;
    io_rw_32 proc_config;
    io_rw_32 proc_in_sync_bypass;
    io_rw_32 proc_in_sync_bypass_hi;
    io_rw_32 dbgforce;
    io_rw_32 mempowerdown;
} syscfg_hw_t;

#define syscfg_hw ((syscfg_hw_t *)SYSCFG_BASE)
// static_assert(sizeof (syscfg_hw_t) == 0x001c, "");

#endif // _HARDWARE_STRUCTS_SYSCFG_H
