/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_SSI_H
#define _HARDWARE_STRUCTS_SSI_H


typedef struct {
    io_rw_32 ctrlr0;
    io_rw_32 ctrlr1;
    io_rw_32 ssienr;
    io_rw_32 mwcr;
    io_rw_32 ser;
    io_rw_32 baudr;
    io_rw_32 txftlr;
    io_rw_32 rxftlr;
    io_ro_32 txflr;
    io_ro_32 rxflr;
    io_ro_32 sr;
    io_rw_32 imr;
    io_ro_32 isr;
    io_ro_32 risr;
    io_ro_32 txoicr;
    io_ro_32 rxoicr;
    io_ro_32 rxuicr;
    io_ro_32 msticr;
    io_ro_32 icr;
    io_rw_32 dmacr;
    io_rw_32 dmatdlr;
    io_rw_32 dmardlr;
    io_ro_32 idr;
    io_ro_32 ssi_version_id;
    io_rw_32 dr0;
    uint32_t _pad0[35];
    io_rw_32 rx_sample_dly;
    io_rw_32 spi_ctrlr0;
    io_rw_32 txd_drive_edge;
} ssi_hw_t;

#define ssi_hw ((ssi_hw_t *)XIP_SSI_BASE)
// static_assert(sizeof (ssi_hw_t) == 0x00fc, "");

#endif // _HARDWARE_STRUCTS_SSI_H
