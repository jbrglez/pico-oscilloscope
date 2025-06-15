/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_I2C_H
#define _HARDWARE_STRUCTS_I2C_H


typedef struct {
    io_rw_32 con;
    io_rw_32 tar;
    io_rw_32 sar;
    uint32_t _pad0;
    io_rw_32 data_cmd;
    io_rw_32 ss_scl_hcnt;
    io_rw_32 ss_scl_lcnt;
    io_rw_32 fs_scl_hcnt;
    io_rw_32 fs_scl_lcnt;
    uint32_t _pad1[2];
    io_ro_32 intr_stat;
    io_rw_32 intr_mask;
    io_ro_32 raw_intr_stat;
    io_rw_32 rx_tl;
    io_rw_32 tx_tl;
    io_ro_32 clr_intr;
    io_ro_32 clr_rx_under;
    io_ro_32 clr_rx_over;
    io_ro_32 clr_tx_over;
    io_ro_32 clr_rd_req;
    io_ro_32 clr_tx_abrt;
    io_ro_32 clr_rx_done;
    io_ro_32 clr_activity;
    io_ro_32 clr_stop_det;
    io_ro_32 clr_start_det;
    io_ro_32 clr_gen_call;
    io_rw_32 enable;
    io_ro_32 status;
    io_ro_32 txflr;
    io_ro_32 rxflr;
    io_rw_32 sda_hold;
    io_ro_32 tx_abrt_source;
    io_rw_32 slv_data_nack_only;
    io_rw_32 dma_cr;
    io_rw_32 dma_tdlr;
    io_rw_32 dma_rdlr;
    io_rw_32 sda_setup;
    io_rw_32 ack_general_call;
    io_ro_32 enable_status;
    io_rw_32 fs_spklen;
    uint32_t _pad2;
    io_ro_32 clr_restart_det;
    uint32_t _pad3[18];
    io_ro_32 comp_param_1;
    io_ro_32 comp_version;
    io_ro_32 comp_type;
} i2c_hw_t;

#define i2c0_hw ((i2c_hw_t *)I2C0_BASE)
#define i2c1_hw ((i2c_hw_t *)I2C1_BASE)
// static_assert(sizeof (i2c_hw_t) == 0x0100, "");

#endif // _HARDWARE_STRUCTS_I2C_H
