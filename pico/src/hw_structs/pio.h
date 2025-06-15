/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_PIO_H
#define _HARDWARE_STRUCTS_PIO_H


typedef struct {
    io_rw_32 clkdiv;
    io_rw_32 execctrl;
    io_rw_32 shiftctrl;
    io_ro_32 addr;
    io_rw_32 instr;
    io_rw_32 pinctrl;
} pio_sm_hw_t;

typedef struct {
    io_rw_32 inte;
    io_rw_32 intf;
    io_ro_32 ints;
} pio_irq_ctrl_hw_t;

typedef struct {
    io_rw_32 ctrl;
    io_ro_32 fstat;
    io_rw_32 fdebug;
    io_ro_32 flevel;
    io_wo_32 txf[4];
    io_ro_32 rxf[4];
    io_rw_32 irq;
    io_wo_32 irq_force;
    io_rw_32 input_sync_bypass;
    io_ro_32 dbg_padout;
    io_ro_32 dbg_padoe;
    io_ro_32 dbg_cfginfo;
    io_wo_32 instr_mem[32];
    pio_sm_hw_t sm[4];
    io_ro_32 intr;
    union {
        struct {
            io_rw_32 inte0;
            io_rw_32 intf0;
            io_ro_32 ints0;
            io_rw_32 inte1;
            io_rw_32 intf1;
            io_ro_32 ints1;
        };
        pio_irq_ctrl_hw_t irq_ctrl[2];
    };
} pio_hw_t;

#define pio0_hw ((pio_hw_t *)PIO0_BASE)
#define pio1_hw ((pio_hw_t *)PIO1_BASE)
// static_assert(sizeof (pio_hw_t) == 0x0144, "");

#endif // _HARDWARE_STRUCTS_PIO_H
