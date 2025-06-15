/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_ADC_H
#define _HARDWARE_STRUCTS_ADC_H


typedef struct {
    io_rw_32 cs;
    io_ro_32 result;
    io_rw_32 fcs;
    io_ro_32 fifo;
    io_rw_32 div;
    io_ro_32 intr;
    io_rw_32 inte;
    io_rw_32 intf;
    io_ro_32 ints;
} adc_hw_t;

#define adc_hw ((adc_hw_t *)ADC_BASE)
// static_assert(sizeof (adc_hw_t) == 0x0024, "");

#endif // _HARDWARE_STRUCTS_ADC_H
