#ifndef DEBUG_STR_C
#define DEBUG_STR_C

#include "config.h"

#if defined(DBG_LOG) && defined(CONFIG_USE_UART)

#include "my_types.h"
#include "uart.c"


typedef struct {
    u32 capacity;
    u32 mask;
    u32 write_pos;
    u32 read_pos;
    char *str;
} debug_str_t;


internal void init_debug_str(debug_str_t *dbg_struct, u32 n_pow2) {
    dbg_struct->capacity = 1 << n_pow2;
    dbg_struct->mask = (1 << n_pow2) - 1;
    dbg_struct->write_pos = 0;
    dbg_struct->read_pos = 0;

    extern const u32 __pico_dbg_str_start__;
    static u32 addr = 0;
    dbg_struct->str = (char *)(__pico_dbg_str_start__ + addr);
    addr += dbg_struct->capacity;
}


internal void DBG_ch(debug_str_t *dbg_str, char c) {
    dbg_str->str[dbg_str->write_pos++ & dbg_str->mask] = c;
}


internal void DBG_str(debug_str_t *dbg_str, char *s) {
    while (*s != 0) {
        dbg_str->str[dbg_str->write_pos++ & dbg_str->mask] = *s++;
    }
}


internal void DBG_hex(debug_str_t *dbg_str, u32 d) {
    u32 n;
    i32 c;
    DBG_ch(dbg_str, '0');
    DBG_ch(dbg_str, 'x');
    for(c = 28; c >= 0; c -= 4) {
        n = (d>>c) & 0xF;
        n += n>9 ? 0x37 : 0x30;
        DBG_ch(dbg_str, n);
    }
}


internal void DBG_hex_64(debug_str_t *dbg_str, u64 d) {
    u64 n;
    i32 c;
    DBG_ch(dbg_str, '0');
    DBG_ch(dbg_str, 'x');
    for(c = 60; c >= 0; c -= 4) {
        n = (d>>c) & 0xF;
        n += n>9 ? 0x37 : 0x30;
        DBG_ch(dbg_str, n);
    }
}


internal void DBG_hex_8(debug_str_t *dbg_str, u32 d) {
    u32 n;
    i32 c;
    DBG_ch(dbg_str, '0');
    DBG_ch(dbg_str, 'x');
    for(c = 4; c >= 0; c -= 4) {
        n = (d>>c) & 0xF;
        n += n>9 ? 0x37 : 0x30;
        DBG_ch(dbg_str, n);
    }
}


internal void DBG_hex_16(debug_str_t *dbg_str, u32 d) {
    u32 n;
    i32 c;
    DBG_ch(dbg_str, '0');
    DBG_ch(dbg_str, 'x');
    for(c = 12; c >= 0; c -= 4) {
        n = (d>>c) & 0xF;
        n += n>9 ? 0x37 : 0x30;
        DBG_ch(dbg_str, n);
    }
}


internal void DBG_raw_hex_bytes(debug_str_t *dbg_str, volatile char *s, i32 len) {
    u32 n;
    i32 c;
    DBG_str(dbg_str, "HEX: ");
    for (i32 i = 0; i < len; i++) {
        u32 d = s[i];
        for(c = 4; c >= 0; c -= 4) {
            n = (d>>c) & 0xF;
            n += n>9 ? 0x37 : 0x30;
            DBG_ch(dbg_str, n);
        }
        DBG_ch(dbg_str, ' ');
    }
}


internal void DBG_print(debug_str_t *dbg_str) {
    while (dbg_str->read_pos < dbg_str->write_pos) {
        char c = dbg_str->str[dbg_str->read_pos++ & dbg_str->mask];
        if (c == '\n')
            uart_send('\r');
        uart_send(c);
    }
}


static debug_str_t dbg0;



#else

#undef DBG_LOG

#define DBG_str(dbg_str, s)
#define DBG_hex(dbg_str, d)
#define DBG_hex_8(dbg_str, d)
#define DBG_hex_16(dbg_str, d)

#define DBG_print(dbg_str)


#endif

#endif
