/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_DMA_DEBUG_H
#define _HARDWARE_STRUCTS_DMA_DEBUG_H


typedef struct {
    io_rw_32 dbg_ctdreq;
    io_ro_32 dbg_tcr;
    uint32_t _pad0[14];
} dma_debug_channel_hw_t;

typedef struct {
    dma_debug_channel_hw_t ch[12];
} dma_debug_hw_t;

#define DMA_CH0_DBG_CTDREQ_OFFSET 0x800
#define dma_debug_hw ((dma_debug_hw_t *)(DMA_BASE + DMA_CH0_DBG_CTDREQ_OFFSET))

#endif // _HARDWARE_STRUCTS_DMA_DEBUG_H
