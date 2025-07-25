/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_PLL_H
#define _HARDWARE_STRUCTS_PLL_H


#define PLL_CS_LOCKED (1u << 31u)
#define PLL_CS_REFDIV_LSB 0u

#define PLL_PWR_VCOPD (1u << 5u)
#define PLL_PWR_POSTDIVPD (1u << 3u)
#define PLL_PWR_DSMPD (1u << 2u)
#define PLL_PWR_PD (1u << 0u)

#define PLL_PRIM_POSTDIV1_LSB 16u
#define PLL_PRIM_POSTDIV2_LSB 12u

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
