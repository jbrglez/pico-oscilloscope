/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_XIP_H
#define _HARDWARE_STRUCTS_XIP_H


typedef struct {
    io_rw_32 ctrl;
    io_wo_32 flush;
    io_ro_32 stat;
    io_rw_32 ctr_hit;
    io_rw_32 ctr_acc;
    io_rw_32 stream_addr;
    io_rw_32 stream_ctr;
    io_ro_32 stream_fifo;
} xip_ctrl_hw_t;

#define xip_ctrl_hw ((xip_ctrl_hw_t *)XIP_CTRL_BASE)
// static_assert(sizeof (xip_ctrl_hw_t) == 0x0020, "");

#define XIP_STAT_FIFO_FULL  4
#define XIP_STAT_FIFO_EMPTY 2
#define XIP_STAT_FLUSH_RDY  1

#endif // _HARDWARE_STRUCTS_XIP_H
