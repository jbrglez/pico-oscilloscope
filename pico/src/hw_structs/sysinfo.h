/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_SYSINFO_H
#define _HARDWARE_STRUCTS_SYSINFO_H


typedef struct {
    io_ro_32 chip_id;
    io_ro_32 platform;
    uint32_t _pad0[2];
    io_ro_32 gitref_rp2040;
} sysinfo_hw_t;

#define sysinfo_hw ((sysinfo_hw_t *)SYSINFO_BASE)
// static_assert(sizeof (sysinfo_hw_t) == 0x0014, "");

#endif // _HARDWARE_STRUCTS_SYSINFO_H
