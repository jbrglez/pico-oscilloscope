#ifndef SIGNAL_BUFFERS_C
#define SIGNAL_BUFFERS_C

#include "my_types.h"
#include "config.h"
#include "hardware.h"


typedef enum {
    SIG_BUF_NORMAL = 0,
    SIG_BUF_NEW_DATA_IN_TRANSFER = 1 << 0,
    SIG_BUF_NEW_DATA_TRANSFER_COMPLETED = 1 << 1,
    SIG_BUF_SWITCH_BUFFERS = 1 << 3,
    SIG_BUF_RESET = 1 << 7,
} signal_buffer_state;


typedef struct {
    u8  *data;
    u32 length;
    u32 capacity;
} sig_gen_data_t;


typedef struct {
    sig_gen_data_t *active_buf;
    sig_gen_data_t *inactive_buf;
    signal_buffer_state status;
    u16 buf_copy_addr_offset;
    u16 buf_copy_len;
} signal_buffer_t;


// u32 signal_data_1[4096] __attribute__ ((aligned(4096)));
// u32 signal_data_2[4096] __attribute__ ((aligned(4096)));
// u8 signal_data_1[4*4096] __attribute__ ((aligned(4096)));
// u8 signal_data_2[4*4096] __attribute__ ((aligned(4096)));
u8 signal_data_1[4*4096];
u8 signal_data_2[4*4096];

sig_gen_data_t sig_piousb_1 = {
    .data = (u8 *)&signal_data_1,
    .capacity = sizeof(signal_data_1)/sizeof(signal_data_1[0]),
};

sig_gen_data_t sig_piousb_2 = {
    .data = (u8 *)&signal_data_2,
    .capacity = sizeof(signal_data_2)/sizeof(signal_data_2[0]),
};

signal_buffer_t sig_buffers_PIO = {
    .active_buf = &sig_piousb_1,
    .inactive_buf = &sig_piousb_2,
    .status = SIG_BUF_NORMAL,
    // .status = SIG_BUF_NEW_DATA_TRANSFER_COMPLETED,
};


internal void pio_dma_copy_new_signal_samples(u32 src, u32 offset, u32 len) {
    while(dma_hw->ch[DMA_SIG_COPY_CH].al1_ctrl & DMA_CH_BUSY_BIT);
    dma_hw->ch[DMA_SIG_COPY_CH].transfer_count = (u32)len;
    dma_hw->ch[DMA_SIG_COPY_CH].read_addr = (u32)(src);
    dma_hw->ch[DMA_SIG_COPY_CH].al2_write_addr_trig = (u32)(sig_buffers_PIO.inactive_buf->data) + offset;
}


#endif
