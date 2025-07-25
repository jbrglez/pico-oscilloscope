/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_ADC_H
#define _HARDWARE_STRUCTS_ADC_H


#define ADC_CS_RROBIN_LSB 16u
#define ADC_CS_AINSEL_LSB 12u
#define ADC_CS_ERR_STICKY (1u << 10u)
#define ADC_CS_ERR (1u << 9u)
#define ADC_CS_READY (1u << 8u)
#define ADC_CS_START_MANY (1u << 3u)
#define ADC_CS_START_ONCE (1u << 2u)
#define ADC_CS_EN (1u << 0u)

#define ADC_FCS_TRESH_LSB 24u
#define ADC_FCS_EMPTY (1u << 8u)
#define ADC_FCS_DREQ_EN (1u << 3u)
#define ADC_FCS_SHIFT (1u << 1u)
#define ADC_FCS_EN (1u << 0u)

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
