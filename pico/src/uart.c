#ifndef UART_C
#define UART_C

#include "config.h"
#include "hardware.h"
#include "my_math.c"

#ifdef CONFIG_USE_UART

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

    uart0_hw->lcr_h = (3<<5) | (1<<4);
    uart0_hw->cr = (1<<9) | (1<<8) | (1<<0);

    io_bank0_hw->io[UART_TX_PIN].ctrl = GPIO_FUNC_UART;
    io_bank0_hw->io[UART_RX_PIN].ctrl = GPIO_FUNC_UART;
}


internal char uart_get(void) {
    while(uart0_hw->fr & (1<<4));
    return uart0_hw->dr;
}


internal i32 uart_get_non_blocking(void) {
    if (uart0_hw->fr & (1<<4))
        return uart0_hw->dr;
    else 
        return -1;
}


internal void uart_send(char c) {
    while(uart0_hw->fr & (1<<5));
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

#else

#define uart_init()
#define uart_get()
#define uart_get_non_blocking()
#define uart_send(c)
#define uart_puts(s)
#define uart_hex(d)
#define uart_hex_64(d)
#define uart_hex_16(d)
#define uart_hex_8(d)
#define uart_raw_hex_byte_array(s,len)

#endif


#endif
