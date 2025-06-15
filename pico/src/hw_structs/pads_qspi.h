/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_PADS_QSPI_H
#define _HARDWARE_STRUCTS_PADS_QSPI_H


typedef struct {
    io_rw_32 voltage_select;
    io_rw_32 io[6];
} pads_qspi_hw_t;

#define pads_qspi_hw ((pads_qspi_hw_t *)PADS_QSPI_BASE)
// static_assert(sizeof (pads_qspi_hw_t) == 0x001c, "");

#endif // _HARDWARE_STRUCTS_PADS_QSPI_H
