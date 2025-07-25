/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_UART_H
#define _HARDWARE_STRUCTS_UART_H

#define UART_FLAG_REG_TX_FIFO_FULL (1u << 5u)
#define UART_FLAG_REG_RX_FIFO_EMPTY (1u << 4u)

#define UART_LINE_CTRL_REG_WORD_LEN_8BIT (3u << 5u)
#define UART_LINE_CTRL_REG_WORD_LEN_7BIT (2u << 5u)
#define UART_LINE_CTRL_REG_WORD_LEN_6BIT (1u << 5u)
#define UART_LINE_CTRL_REG_ENABLE_FIFOS (1u << 4u)

#define UART_CTRL_REG_RECEIVE_ENABLE (1u << 9u)
#define UART_CTRL_REG_TRANSMIT_ENABLE (1u << 8u)
#define UART_CTRL_REG_UART_ENABLE (1u << 0u)

typedef struct {
    io_rw_32 dr;
    io_rw_32 rsr;
    uint32_t _pad0[4];
    io_ro_32 fr;
    uint32_t _pad1;
    io_rw_32 ilpr;
    io_rw_32 ibrd;
    io_rw_32 fbrd;
    io_rw_32 lcr_h;
    io_rw_32 cr;
    io_rw_32 ifls;
    io_rw_32 imsc;
    io_ro_32 ris;
    io_ro_32 mis;
    io_rw_32 icr;
    io_rw_32 dmacr;
} uart_hw_t;

#define uart0_hw ((uart_hw_t *)UART0_BASE)
#define uart1_hw ((uart_hw_t *)UART1_BASE)
// static_assert(sizeof (uart_hw_t) == 0x004c, "");

#endif // _HARDWARE_STRUCTS_UART_H
