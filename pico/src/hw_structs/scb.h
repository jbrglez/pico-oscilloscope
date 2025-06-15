/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_SCB_H
#define _HARDWARE_STRUCTS_SCB_H


typedef struct {
    io_ro_32 cpuid;
    io_rw_32 icsr;
    io_rw_32 vtor;
    io_rw_32 aircr;
    io_rw_32 scr;
} armv6m_scb_hw_t;

#define scb_hw ((armv6m_scb_hw_t *)(PPB_BASE + M0PLUS_CPUID_OFFSET))
// static_assert(sizeof (armv6m_scb_hw_t) == 0x0014, "");

#endif // _HARDWARE_STRUCTS_SCB_H
