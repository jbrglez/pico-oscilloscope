/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_DMA_H
#define _HARDWARE_STRUCTS_DMA_H


typedef struct {
    io_rw_32 read_addr;
    io_rw_32 write_addr;
    io_rw_32 transfer_count;
    io_rw_32 ctrl_trig;
    io_rw_32 al1_ctrl;
    io_rw_32 al1_read_addr;
    io_rw_32 al1_write_addr;
    io_rw_32 al1_transfer_count_trig;
    io_rw_32 al2_ctrl;
    io_rw_32 al2_transfer_count;
    io_rw_32 al2_read_addr;
    io_rw_32 al2_write_addr_trig;
    io_rw_32 al3_ctrl;
    io_rw_32 al3_write_addr;
    io_rw_32 al3_transfer_count;
    io_rw_32 al3_read_addr_trig;
} dma_channel_hw_t;

typedef struct {
    io_rw_32 intr;
    io_rw_32 inte;
    io_rw_32 intf;
    io_rw_32 ints;
} dma_irq_ctrl_hw_t;

typedef struct {
    dma_channel_hw_t ch[12];
    uint32_t _pad0[64];
    union {
        struct {
            io_rw_32 intr;

            io_rw_32 inte0;

            io_rw_32 intf0;

            io_rw_32 ints0;

            uint32_t __pad0;

            io_rw_32 inte1;

            io_rw_32 intf1;

            io_rw_32 ints1;
        };
        dma_irq_ctrl_hw_t irq_ctrl[2];
    };
    io_rw_32 timer[4];
    io_wo_32 multi_channel_trigger;
    io_rw_32 sniff_ctrl;
    io_rw_32 sniff_data;
    uint32_t _pad1;
    io_ro_32 fifo_levels;
    io_wo_32 abort;
} dma_hw_t;

#define dma_hw ((dma_hw_t *)DMA_BASE)
// static_assert(sizeof (dma_hw_t) == 0x0448, "");

#define DMA_CH_BUSY_BIT (1<<24)

#endif // _HARDWARE_STRUCTS_DMA_H
