/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_DMA_H
#define _HARDWARE_STRUCTS_DMA_H


#define DMA_CH_BUSY_BIT (1u<<24u)

#define DMA_CH_CTRL_BUSY (1u<<24u)
#define DMA_CH_CTRL_IRQ_QUIET (1u<<21u)
#define DMA_CH_CTRL_TREQ_SEL_LSB 15u
#define DMA_CH_CTRL_CHAIN_TO_LSB 11u
#define DMA_CH_CTRL_RING_SEL_WRITE (1u<<10u)
#define DMA_CH_CTRL_RING_SEL_READ  (0u<<10u)
#define DMA_CH_CTRL_RING_SIZE_LSB 6u
#define DMA_CH_CTRL_INCR_WRITE (1u<<5u)
#define DMA_CH_CTRL_INCR_READ (1u<<4u)
#define DMA_CH_CTRL_DATA_SIZE_BYTE (0u<<2u)
#define DMA_CH_CTRL_DATA_SIZE_HALFWORD (1u<<2u)
#define DMA_CH_CTRL_DATA_SIZE_WORD (2u<<2u)
#define DMA_CH_CTRL_HIGH_PRIORITY (1u<<1u)
#define DMA_CH_CTRL_EN (1u<<0u)

typedef enum {
    DREQ_PIO0_TX0 = 0,
    DREQ_PIO0_TX1 = 1,
    DREQ_PIO0_TX2 = 2,
    DREQ_PIO0_TX3 = 3,
    DREQ_PIO0_RX0 = 4,
    DREQ_PIO0_RX1 = 5,
    DREQ_PIO0_RX2 = 6,
    DREQ_PIO0_RX3 = 7,

    DREQ_PIO1_TX0 = 8,
    DREQ_PIO1_TX1 = 9,
    DREQ_PIO1_TX2 = 10,
    DREQ_PIO1_TX3 = 11,
    DREQ_PIO1_RX0 = 12,
    DREQ_PIO1_RX1 = 13,
    DREQ_PIO1_RX2 = 14,
    DREQ_PIO1_RX3 = 15,

    DREQ_SPI0_TX = 16,
    DREQ_SPI0_RX = 17,
    DREQ_SPI1_TX = 18,
    DREQ_SPI1_RX = 19,

    DREQ_UART0_TX = 20,
    DREQ_UART0_RX = 21,
    DREQ_UART1_TX = 22,
    DREQ_UART1_RX = 23,

    DREQ_PWM_WRAP0 = 24,
    DREQ_PWM_WRAP1 = 25,
    DREQ_PWM_WRAP2 = 26,
    DREQ_PWM_WRAP3 = 27,
    DREQ_PWM_WRAP4 = 28,
    DREQ_PWM_WRAP5 = 29,
    DREQ_PWM_WRAP6 = 30,
    DREQ_PWM_WRAP7 = 31,

    DREQ_I2C0_TX = 32,
    DREQ_I2C0_RX = 33,
    DREQ_I2C1_TX = 34,
    DREQ_I2C1_RX = 35,

    DREQ_ADC = 36,

    DREQ_XIP_STREAM = 37,
    DREQ_XIP_SSITX = 38,
    DREQ_XIP_SSIRX = 39,

    DREQ_TIMER0 = 0x3B,
    DREQ_TIMER1 = 0x3C,
    DREQ_TIMER2 = 0x3D,
    DREQ_TIMER3 = 0x3E,

    DREQ_PERMANENT = 0x3F,
} DMA_DREQ_src;


typedef struct {
    io_rw_32 read_addr;
    io_rw_32 write_addr;
    io_rw_32 transfer_count;
    io_rw_32 ctrl_trig;
    io_rw_32 al1_ctrl;
    io_rw_32 al1_read_addr;
    io_rw_32 al1_write_addr;
    io_rw_32 al1_transfer_count_trig;
    io_rw_32 al2_ctrl;
    io_rw_32 al2_transfer_count;
    io_rw_32 al2_read_addr;
    io_rw_32 al2_write_addr_trig;
    io_rw_32 al3_ctrl;
    io_rw_32 al3_write_addr;
    io_rw_32 al3_transfer_count;
    io_rw_32 al3_read_addr_trig;
} dma_channel_hw_t;

typedef struct {
    io_rw_32 intr;
    io_rw_32 inte;
    io_rw_32 intf;
    io_rw_32 ints;
} dma_irq_ctrl_hw_t;

typedef struct {
    dma_channel_hw_t ch[12];
    uint32_t _pad0[64];
    union {
        struct {
            io_rw_32 intr;

            io_rw_32 inte0;

            io_rw_32 intf0;

            io_rw_32 ints0;

            uint32_t __pad0;

            io_rw_32 inte1;

            io_rw_32 intf1;

            io_rw_32 ints1;
        };
        dma_irq_ctrl_hw_t irq_ctrl[2];
    };
    io_rw_32 timer[4];
    io_wo_32 multi_channel_trigger;
    io_rw_32 sniff_ctrl;
    io_rw_32 sniff_data;
    uint32_t _pad1;
    io_ro_32 fifo_levels;
    io_wo_32 abort;
} dma_hw_t;

#define dma_hw ((dma_hw_t *)DMA_BASE)
// static_assert(sizeof (dma_hw_t) == 0x0448, "");

#endif // _HARDWARE_STRUCTS_DMA_H
