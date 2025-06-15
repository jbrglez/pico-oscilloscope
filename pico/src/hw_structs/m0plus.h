/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_M0PLUS_H
#define _HARDWARE_STRUCTS_M0PLUS_H


typedef struct {
    uint32_t _pad0[14340];
    io_rw_32 syst_csr;
    io_rw_32 syst_rvr;
    io_rw_32 syst_cvr;
    io_ro_32 syst_calib;
    uint32_t _pad1[56];
    io_rw_32 nvic_iser;
    uint32_t _pad2[31];
    io_rw_32 nvic_icer;
    uint32_t _pad3[31];
    io_rw_32 nvic_ispr;
    uint32_t _pad4[31];
    io_rw_32 nvic_icpr;
    uint32_t _pad5[95];
    io_rw_32 nvic_ipr[8];
    uint32_t _pad6[568];
    io_ro_32 cpuid;
    io_rw_32 icsr;
    io_rw_32 vtor;
    io_rw_32 aircr;
    io_rw_32 scr;
    io_ro_32 ccr;
    uint32_t _pad7;
    io_rw_32 shpr[2];
    io_rw_32 shcsr;
    uint32_t _pad8[26];
    io_ro_32 mpu_type;
    io_rw_32 mpu_ctrl;
    io_rw_32 mpu_rnr;
    io_rw_32 mpu_rbar;
    io_rw_32 mpu_rasr;
} m0plus_hw_t;

#define ppb_hw ((m0plus_hw_t *)PPB_BASE)
// static_assert(sizeof (m0plus_hw_t) == 0xeda4, "");

#endif // _HARDWARE_STRUCTS_M0PLUS_H
