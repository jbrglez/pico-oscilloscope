/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_TBMAN_H
#define _HARDWARE_STRUCTS_TBMAN_H


typedef struct {
    io_ro_32 platform;
} tbman_hw_t;

#define tbman_hw ((tbman_hw_t *)TBMAN_BASE)
// static_assert(sizeof (tbman_hw_t) == 0x0004, "");

#endif // _HARDWARE_STRUCTS_TBMAN_H
