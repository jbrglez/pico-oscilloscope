/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_SIO_H
#define _HARDWARE_STRUCTS_SIO_H


typedef struct {
    io_ro_32 cpuid;
    io_ro_32 gpio_in;
    io_ro_32 gpio_hi_in;
    uint32_t _pad0;
    io_rw_32 gpio_out;
    io_wo_32 gpio_out_set;
    io_wo_32 gpio_out_clr;
    io_wo_32 gpio_out_xor;
    io_rw_32 gpio_oe;
    io_wo_32 gpio_oe_set;
    io_wo_32 gpio_oe_clr;
    io_wo_32 gpio_oe_xor;
    io_rw_32 gpio_hi_out;
    io_wo_32 gpio_hi_set;
    io_wo_32 gpio_hi_clr;
    io_wo_32 gpio_hi_xor;
    io_rw_32 gpio_hi_oe;
    io_wo_32 gpio_hi_oe_set;
    io_wo_32 gpio_hi_oe_clr;
    io_wo_32 gpio_hi_oe_xor;
    io_rw_32 fifo_st;
    io_wo_32 fifo_wr;
    io_ro_32 fifo_rd;
    io_ro_32 spinlock_st;
    io_rw_32 div_udividend;
    io_rw_32 div_udivisor;
    io_rw_32 div_sdividend;
    io_rw_32 div_sdivisor;
    io_rw_32 div_quotient;
    io_rw_32 div_remainder;
    io_ro_32 div_csr;
    uint32_t _pad1;
    interp_hw_t interp[2];
    io_rw_32 spinlock[32];
} sio_hw_t;

#define sio_hw ((sio_hw_t *)SIO_BASE)
// static_assert(sizeof (sio_hw_t) == 0x0180, "");

#endif // _HARDWARE_STRUCTS_SIO_H
