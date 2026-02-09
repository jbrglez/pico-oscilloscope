#ifndef UART_C
#define UART_C

#include "config.h"
#include "hardware.h"
#include "my_math.c"

#include <stdarg.h>

//#define UART_MAX_NUM_DMA_MSG_BLOCKS 256
#define UART_MAX_NUM_DMA_MSG_BLOCKS 1024

#define U_TYPE_str 0
#define U_TYPE_u32 1

#define MSG_STATUS_DONE 0
#define MSG_STATUS_READY 1

typedef struct uart_msg_block_t {
    // struct uart_msg_block_t *next_msg;
    union {
        char *msg;
        u32 hex;
    };
    u32 len;
    u16 type;
    u16 msg_status;
} uart_msg_block_t;

char msg_hex_buf[32];

volatile b32 uart_dma_active = 0;

volatile u32 uart_msg_idx_next_write = 0;
volatile u32 uart_msg_idx_next_read  = 0;
uart_msg_block_t uart_msgs[UART_MAX_NUM_DMA_MSG_BLOCKS] = {0};

#ifdef CONFIG_USE_UART


typedef struct uart_msg_rbuf_t {
    char *start;
    char *end;
    u32 buf_size_pow2;
    u32 buf_size;
    u32 buf_mask;
    volatile char *putp;
    volatile char *getp;
    volatile b32 active;
} uart_msg_rbuf_t;

uart_msg_rbuf_t murbuf_G = {
    .buf_size_pow2 = 14,
    .buf_size = 0x4000,
    .buf_mask = 0x3FFF,
};

inline void put_char(char c)
{
    *murbuf_G.putp = c;
    char *next = (char *)murbuf_G.putp + 1;
    murbuf_G.putp = (next < murbuf_G.end) ? next : murbuf_G.start;
}


internal void putsu(const char *msg)
{
    while (*msg != 0)
        put_char(*msg++);

    if (!murbuf_G.active)
        hw_set_bits(&dma_hw->intf1, 1<<DMA_UART1_CH);

    // if (!(dma_hw->ch[DMA_UART1_CH].ctrl_trig & DMA_CH_CTRL_BUSY))
    //     hw_set_bits(&dma_hw->intf1, 1<<DMA_UART1_CH);
}


internal void putsu_hex(u32 d)
{
    put_char('0');
    put_char('x');
    char n;
    for(i32 c = 28; c >= 0; c -= 4) {
        n = (d>>c) & 0xF;
        n += (n > 9) ? 0x37 : 0x30;
        put_char(n);
    }

    if (!murbuf_G.active)
        hw_set_bits(&dma_hw->intf1, 1<<DMA_UART1_CH);

    // if (!(dma_hw->ch[DMA_UART1_CH].ctrl_trig & DMA_CH_CTRL_BUSY))
    //     hw_set_bits(&dma_hw->intf1, 1<<DMA_UART1_CH);
}


internal void uprintf(const char *msgf, ...)
{
    va_list args;
    va_start(args, msgf);

    char c = *msgf;
    while (c != 0) {
        if (c != '%') {
            put_char(c);
        }
        else {
            c = *msgf++;
            switch (c) {
                case 's': {
                    const char *msg = va_arg(args, const char *);
                    while (*msg != 0)
                        put_char(*msg++);
                } break;

                case 'd': {
                    i32 d = va_arg(args, i32);
                    if (d < 0) {
                        put_char('-');
                        d = -d;
                    }

                    u32 u = (u32)d;
                    u32 rem[10];

                    u32 num_digits = 0;
                    for (u32 i = 0; i < 10; i++) {
                        u = divide_u32(u, 10, &rem[i]);
                        num_digits++;
                        if (u == 0)
                            break;
                    }

                    for (u32 i = 1; i < num_digits+1; i++) {
                        put_char((char)rem[num_digits - i] + '0');
                    }
                } break;

                case 'u': {
                    u32 u = va_arg(args, u32);
                    u32 rem[10];

                    u32 num_digits = 0;
                    for (u32 i = 0; i < 10; i++) {
                        u = divide_u32(u, 10, &rem[i]);
                        num_digits++;
                        if (u == 0)
                            break;
                    }

                    for (u32 i = 1; i < num_digits+1; i++) {
                        put_char((char)rem[num_digits - i] + '0');
                    }
                } break;

                case 'x': {
                    u32 d = va_arg(args, u32);
                    put_char('0');
                    put_char('x');
                    char n;
                    for(i32 c = 28; c >= 0; c -= 4) {
                        n = (d>>c) & 0xF;
                        n += (n > 9) ? 0x37 : 0x30;
                        put_char(n);
                    }
                } break;

                default: {
                }
            }
        }
        c = *msgf++;
    }

    va_end(args);

    if (!murbuf_G.active)
        hw_set_bits(&dma_hw->intf1, 1<<DMA_UART1_CH);

    // if (!(dma_hw->ch[DMA_UART1_CH].ctrl_trig & DMA_CH_CTRL_BUSY))
    //     hw_set_bits(&dma_hw->intf1, 1<<DMA_UART1_CH);
}


internal void uart1_init(void) {
    hw_set_bits(&resets_hw->reset, RESET_UART1);
    hw_clear_bits(&resets_hw->reset, RESET_UART1);
    while(!(resets_hw->reset_done & RESET_UART1));

    u32 baud = UART1_BAUD_RATE;
    u32 rem = 0;
    u32 ibrd = divide_u32(48000000, baud << 4, &rem);
    u32 fbrd = divide_u32((rem << 6) + (baud << 3), baud << 4, 0);

    uart1_hw->ibrd = ibrd;
    uart1_hw->fbrd = fbrd;

    #define UART_DMACR_TXDMAE (1u << 1u)
    uart1_hw->dmacr = UART_DMACR_TXDMAE;

    uart1_hw->lcr_h = UART_LINE_CTRL_REG_WORD_LEN_8BIT | UART_LINE_CTRL_REG_ENABLE_FIFOS;
    uart1_hw->cr = UART_CTRL_REG_RECEIVE_ENABLE | UART_CTRL_REG_TRANSMIT_ENABLE | UART_CTRL_REG_UART_ENABLE;

    io_bank0_hw->io[UART1_TX_PIN].ctrl = GPIO_FUNC_UART;
    io_bank0_hw->io[UART1_RX_PIN].ctrl = GPIO_FUNC_UART;

    // FIX: get/reserve the location for this buffer dynamically (some memory function).
    murbuf_G.start = (char *)0x20010000;

    murbuf_G.end  = murbuf_G.start + murbuf_G.buf_size;
    murbuf_G.putp = murbuf_G.start;
    murbuf_G.getp = murbuf_G.start;

    for (int i = 0; i < murbuf_G.buf_size; i++) {
        murbuf_G.start[i] = 0;
    }
}


internal void uart_init(void) {
    hw_set_bits(&resets_hw->reset, RESET_UART0);
    hw_clear_bits(&resets_hw->reset, RESET_UART0);
    while(!(resets_hw->reset_done & RESET_UART0));

    u32 baud = UART_BAUD_RATE;
    u32 rem = 0;
    u32 ibrd = divide_u32(48000000, baud << 4, &rem);
    u32 fbrd = divide_u32((rem << 6) + (baud << 3), baud << 4, 0);

    uart0_hw->ibrd = ibrd;
    uart0_hw->fbrd = fbrd;

    #define UART_DMACR_TXDMAE (1u << 1u)
    uart0_hw->dmacr = UART_DMACR_TXDMAE;

    uart0_hw->lcr_h = UART_LINE_CTRL_REG_WORD_LEN_8BIT | UART_LINE_CTRL_REG_ENABLE_FIFOS;
    uart0_hw->cr = UART_CTRL_REG_RECEIVE_ENABLE | UART_CTRL_REG_TRANSMIT_ENABLE | UART_CTRL_REG_UART_ENABLE;

    io_bank0_hw->io[UART_TX_PIN].ctrl = GPIO_FUNC_UART;
    io_bank0_hw->io[UART_RX_PIN].ctrl = GPIO_FUNC_UART;
}


internal char uart_get(void) {
    while(uart0_hw->fr & UART_FLAG_REG_RX_FIFO_EMPTY);
    return uart0_hw->dr;
}


internal i32 uart_get_non_blocking(void) {
    if (uart0_hw->fr & UART_FLAG_REG_RX_FIFO_EMPTY)
        return uart0_hw->dr;
    else 
        return -1;
}


internal void uart_send(char c) {
    while(uart0_hw->fr & UART_FLAG_REG_TX_FIFO_FULL);
    uart0_hw->dr = (i32)c;
}


internal void uart_puts(char *s) {
    while(*s) {
        if(*s=='\n')
            uart_send('\r');
        uart_send(*s++);
    }
}


internal void uart_hex(u32 d) {
    u32 n;
    uart_send('0');
    uart_send('x');
    for(i32 c = 28; c >= 0; c -= 4) {
        n = (d>>c) & 0xF;
        n += n>9 ? 0x37 : 0x30;
        uart_send(n);
    }
}


u32 __put_x_counter = 0;

#define PUT_X(var)                  \
    uart_puts("\t" #var " \t: ");   \
    uart_hex((u32)(var));           \
    uart_puts("\r\n");              \
    __put_x_counter++;


#define PUT_CLEAR()                 \
    while(__put_x_counter > 0) {    \
        __put_x_counter--;          \
        uart_puts("\e[1A");         \
    }


internal void uart_hex_64(u64 d) {
    u64 n;
    uart_send('0');
    uart_send('x');
    for(i32 c = 60; c >= 0; c -= 4) {
        n = (d>>c) & 0xF;
        n += n>9 ? 0x37 : 0x30;
        uart_send(n);
    }
}


internal void uart_hex_16(u32 d) {
    u32 n;
    uart_send('0');
    uart_send('x');
    for(i32 c = 12; c >= 0; c -= 4) {
        n = (d>>c) & 0xF;
        n += n>9 ? 0x37 : 0x30;
        uart_send(n);
    }
}


internal void uart_hex_8(u32 d) {
    u32 n;
    uart_send('0');
    uart_send('x');
    for(i32 c = 4; c >= 0; c -= 4) {
        n = (d>>c) & 0xF;
        n += n>9 ? 0x37 : 0x30;
        uart_send(n);
    }
}


internal void uart_raw_hex_byte_array(volatile unsigned char *s, i32 len) {
    u32 n;
    uart_puts("HEX: ");
    for (i32 i = 0; i < len; i++) {
        u32 d = s[i];
        for(i32 c = 4; c >= 0; c -= 4) {
            n = (d>>c) & 0xF;
            n += n>9 ? 0x37 : 0x30;
            uart_send(n);
        }
        uart_send(' ');
    }
}


internal void uart_puts_DMA_msg(char *msg, u32 len) {
    u32 idx = uart_msg_idx_next_write;

    u32 next_idx = idx + 1;
    uart_msg_idx_next_write = (next_idx == UART_MAX_NUM_DMA_MSG_BLOCKS) ? 0 : next_idx;

    uart_msgs[idx].msg = msg;
    uart_msgs[idx].len = len;
    uart_msgs[idx].type = U_TYPE_str;
    uart_msgs[idx].msg_status = MSG_STATUS_READY;

    if (!uart_dma_active) {
        // uart_dma_active = 1;
        hw_set_bits(&dma_hw->intf1, 1<<DMA_UART_CH);
    }
}

internal void uart_puts_DMA_hex(u32 val) {
    u32 idx = uart_msg_idx_next_write;

    u32 next_idx = idx + 1;
    uart_msg_idx_next_write = (next_idx == UART_MAX_NUM_DMA_MSG_BLOCKS) ? 0 : next_idx;

    uart_msgs[idx].hex = val;
    uart_msgs[idx].type = U_TYPE_u32;
    uart_msgs[idx].msg_status = MSG_STATUS_READY;

    if (!uart_dma_active) {
        // uart_dma_active = 1;
        hw_set_bits(&dma_hw->intf1, 1<<DMA_UART_CH);
    }
}


internal void uart_puts_Dmsg(char *msg) {
    u32 len = 0;
    while (msg[len] != 0) {
        len++;
    }
    uart_puts_DMA_msg(msg, len+1);
}


#else

#define putsu(msg)
#define putsu_hex(d)
#define uprintf(msgf, ...)
#define uart1_init()

#define uart_init()
#define uart_get()
#define uart_get_non_blocking()
#define uart_send(c)
#define uart_puts(s)
#define uart_hex(d)
#define uart_hex_64(d)
#define uart_hex_16(d)
#define uart_hex_8(d)
#define uart_raw_hex_byte_array(s, len)

#define uart_puts_DMA_hex(val)
#define uart_puts_Dmsg(msg)
#define uart_puts_DMA_msg(msg, len)

#endif


#endif
