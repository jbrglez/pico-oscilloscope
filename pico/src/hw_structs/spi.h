/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_SPI_H
#define _HARDWARE_STRUCTS_SPI_H


typedef struct {
    io_rw_32 cr0;
    io_rw_32 cr1;
    io_rw_32 dr;
    io_ro_32 sr;
    io_rw_32 cpsr;
    io_rw_32 imsc;
    io_ro_32 ris;
    io_ro_32 mis;
    io_rw_32 icr;
    io_rw_32 dmacr;
} spi_hw_t;

#define spi0_hw ((spi_hw_t *)SPI0_BASE)
#define spi1_hw ((spi_hw_t *)SPI1_BASE)
// static_assert(sizeof (spi_hw_t) == 0x0028, "");

#endif // _HARDWARE_STRUCTS_SPI_H
