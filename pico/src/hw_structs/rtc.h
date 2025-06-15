/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_RTC_H
#define _HARDWARE_STRUCTS_RTC_H


typedef struct {
    io_rw_32 clkdiv_m1;
    io_rw_32 setup_0;
    io_rw_32 setup_1;
    io_rw_32 ctrl;
    io_rw_32 irq_setup_0;
    io_rw_32 irq_setup_1;
    io_ro_32 rtc_1;
    io_ro_32 rtc_0;
    io_ro_32 intr;
    io_rw_32 inte;
    io_rw_32 intf;
    io_ro_32 ints;
} rtc_hw_t;

#define rtc_hw ((rtc_hw_t *)RTC_BASE)
// static_assert(sizeof (rtc_hw_t) == 0x0030, "");

#endif // _HARDWARE_STRUCTS_RTC_H
