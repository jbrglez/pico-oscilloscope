/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_PLL_H
#define _HARDWARE_STRUCTS_PLL_H


typedef struct {
    io_rw_32 cs;
    io_rw_32 pwr;
    io_rw_32 fbdiv_int;
    io_rw_32 prim;
} pll_hw_t;

#define pll_sys_hw ((pll_hw_t *)PLL_SYS_BASE)
#define pll_usb_hw ((pll_hw_t *)PLL_USB_BASE)
// static_assert(sizeof (pll_hw_t) == 0x0010, "");

#endif // _HARDWARE_STRUCTS_PLL_H
