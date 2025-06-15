/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_MPU_H
#define _HARDWARE_STRUCTS_MPU_H


typedef struct {
    io_ro_32 type;
    io_rw_32 ctrl;
    io_rw_32 rnr;
    io_rw_32 rbar;
    io_rw_32 rasr;
} mpu_hw_t;

#define mpu_hw ((mpu_hw_t *)(PPB_BASE + M0PLUS_MPU_TYPE_OFFSET))
// static_assert(sizeof (mpu_hw_t) == 0x0014, "");

#endif // _HARDWARE_STRUCTS_MPU_H
