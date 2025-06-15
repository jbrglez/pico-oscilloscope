/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_NVIC_H
#define _HARDWARE_STRUCTS_NVIC_H


typedef struct {
    io_rw_32 iser;
    uint32_t _pad0[31];
    io_rw_32 icer;
    uint32_t _pad1[31];
    io_rw_32 ispr;
    uint32_t _pad2[31];
    io_rw_32 icpr;
    uint32_t _pad3[95];
    io_rw_32 ipr[8];
} nvic_hw_t;

#define M0PLUS_NVIC_ISER_OFFSET 0xe100
#define nvic_hw ((nvic_hw_t *)(PPB_BASE + M0PLUS_NVIC_ISER_OFFSET))
// static_assert(sizeof (nvic_hw_t) == 0x0320, "");

#endif // _HARDWARE_STRUCTS_NVIC_H
