/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_BUSCTRL_H
#define _HARDWARE_STRUCTS_BUSCTRL_H


/** \brief Bus fabric performance counters on RP2040 (used as typedef \ref bus_ctrl_perf_counter_t)
 *  \ingroup hardware_busctrl
 */
typedef enum bus_ctrl_perf_counter_rp2040 {
    arbiter_rom_perf_event_access = 19,
    arbiter_rom_perf_event_access_contested = 18,
    arbiter_xip_main_perf_event_access = 17,
    arbiter_xip_main_perf_event_access_contested = 16,
    arbiter_sram0_perf_event_access = 15,
    arbiter_sram0_perf_event_access_contested = 14,
    arbiter_sram1_perf_event_access = 13,
    arbiter_sram1_perf_event_access_contested = 12,
    arbiter_sram2_perf_event_access = 11,
    arbiter_sram2_perf_event_access_contested = 10,
    arbiter_sram3_perf_event_access = 9,
    arbiter_sram3_perf_event_access_contested = 8,
    arbiter_sram4_perf_event_access = 7,
    arbiter_sram4_perf_event_access_contested = 6,
    arbiter_sram5_perf_event_access = 5,
    arbiter_sram5_perf_event_access_contested = 4,
    arbiter_fastperi_perf_event_access = 3,
    arbiter_fastperi_perf_event_access_contested = 2,
    arbiter_apb_perf_event_access = 1,
    arbiter_apb_perf_event_access_contested = 0
} bus_ctrl_perf_counter_t;

typedef struct {
    io_rw_32 value;
    io_rw_32 sel;
} bus_ctrl_perf_hw_t;

typedef struct {
    io_rw_32 priority;
    io_ro_32 priority_ack;
    bus_ctrl_perf_hw_t counter[4];
} busctrl_hw_t;

#define busctrl_hw ((busctrl_hw_t *)BUSCTRL_BASE)
// static_assert(sizeof (busctrl_hw_t) == 0x0028, "");

#define bus_ctrl_hw busctrl_hw

#endif // _HARDWARE_STRUCTS_BUSCTRL_H
