#ifndef ADC_C
#define ADC_C

#include "hardware.h"
#include "config.h"
#include "uart.c"

#include "debug_str.c"

#include "usb.h"
#include "usb_ep.h"


internal void init_adc(void) {
    hw_set_bits(&resets_hw->reset, RESET_ADC);
    hw_clear_bits(&resets_hw->reset, RESET_ADC);
    while (!(resets_hw->reset_done & RESET_ADC));

    adc_hw->cs = 1;

    while (!(adc_hw->cs & (1<<8)));

    io_bank0_hw->io[26].ctrl = GPIO_FUNC_NULL;
    io_bank0_hw->io[27].ctrl = GPIO_FUNC_NULL;
    pads_bank0_hw->io[26] = (1<<6);
    pads_bank0_hw->io[27] = (1<<6);

    adc_hw->cs = (0<<12) | (1<<0);
    adc_hw->fcs = (1<<24) | (1<<3) | (1<<1) | (1<<0);
}


internal u16 adc_get_sample() {
    while(!(adc_hw->cs & (1<<8)));
    hw_set_bits(&adc_hw->cs, (1<<2));
    while(!(adc_hw->cs & (1<<8)));
    return adc_hw->result;
}


typedef struct {
    volatile u16 size;
    volatile u16 idx_last;
    volatile i32 num_writes;
    volatile i32 num_reads;
#if defined(DBG_LOG) && defined(DBG_USB_EP4_XFER)
    #define DBG_NUM_EP4_REQS (1<<4)
    #define DBG_NUM_EP4_REQS_MASK ((1<<4) - 1)
    volatile u64 xfer_req_time[DBG_NUM_EP4_REQS];
    volatile u16 xfer_req_size[DBG_NUM_EP4_REQS];
    volatile u32 xfer_req_offset[DBG_NUM_EP4_REQS];
    volatile u8  xfer_req_time_idx;
    volatile u64 isr_time[DBG_NUM_EP4_REQS];
    volatile i32 isr_reads[DBG_NUM_EP4_REQS];
    volatile u8  isr_time_idx;
    volatile u64 t_capture_start;
#endif
} running_adc_dma_t;

static running_adc_dma_t ep4_ADMA = { .size = (1<<11) };


typedef struct {
    u32 start_offset_into_usb_dpram;
    u16 size;
} new_sample_batch_t;


internal new_sample_batch_t adc_get_new_sample_batch(void) {
    new_sample_batch_t new_samples;

    new_samples.start_offset_into_usb_dpram =  ep4_ADMA.idx_last | 0x800;

#if defined(DBG_LOG) && defined(DBG_USB_EP4_XFER)
    ep4_ADMA.xfer_req_time_idx++;
    ep4_ADMA.xfer_req_time_idx &= DBG_NUM_EP4_REQS_MASK;
    ep4_ADMA.xfer_req_time[ep4_ADMA.xfer_req_time_idx] = time_us();
#endif

    #define MASK_2048_BYTES_64_BYTE_ALIGNED 0x7C0
    u16 idx_current = dma_hw->ch[DMA_ADC_CH].write_addr & MASK_2048_BYTES_64_BYTE_ALIGNED;


    // NOTE: Buffer ready for transfer can/should start only on 64-byte aligned addresses.
    #define MAX_BATCH_SIZE 960

    i32 idx_lst = ep4_ADMA.idx_last;

    // if ((ep4_ADMA.num_writes <= ep4_ADMA.num_reads) && (idx_current >= ep4_ADMA.idx_last)) {
    if (idx_current >= ep4_ADMA.idx_last) {
        new_samples.size = MIN(idx_current - ep4_ADMA.idx_last, MAX_BATCH_SIZE);
        ep4_ADMA.idx_last += new_samples.size;
    }
    else {
        new_samples.size = ep4_ADMA.size - ep4_ADMA.idx_last;
        if (new_samples.size > MAX_BATCH_SIZE) {
            new_samples.size = MAX_BATCH_SIZE;
            ep4_ADMA.idx_last += MAX_BATCH_SIZE;
        }
        else {
            ep4_ADMA.num_reads++;
            ep4_ADMA.idx_last = 0;
        }
    }

#if defined(DBG_LOG) && defined(DBG_USB_EP4_XFER)
    ep4_ADMA.xfer_req_size[ep4_ADMA.xfer_req_time_idx] = new_samples.size;
    ep4_ADMA.xfer_req_offset[ep4_ADMA.xfer_req_time_idx] = new_samples.start_offset_into_usb_dpram;
#endif

    return new_samples;
}


internal void adc_stop_dma_transfers_running(void) {
    hw_clear_bits(&adc_hw->cs, (1<<3));
}


internal void adc_stop_capturing(void) {
    hw_clear_bits(&adc_hw->cs, (1<<10) | (1<<3) | (1<<2));
    hw_clear_bits(&dma_hw->inte1, 1 << DMA_ADC_CH);
    dma_hw->abort = 1 << DMA_ADC_CH;
    while(dma_hw->abort);
    while(dma_hw->ch[DMA_ADC_CH].al1_ctrl & DMA_CH_BUSY_BIT);
    dma_hw->intr = 1 << DMA_ADC_CH;
    hw_set_bits(&dma_hw->inte1, 1 << DMA_ADC_CH);
}


internal void adc_start_capture_ep4(u16 num) {
    ep4_ADMA.idx_last = 0;
    ep4_ADMA.num_writes = 0;
    ep4_ADMA.num_reads = 0;

    hw_clear_bits(&adc_hw->cs, (1<<3) | (1<<2));
    while(!(adc_hw->cs & (1<<8)));
    while(!(adc_hw->fcs & (1<<8))) {
        // Clear out the ADC FIFO if not empty.
        u32 _unused = adc_hw->fifo;
    }

#if defined(DBG_LOG) && defined(DBG_USB_EP4_XFER)
    ep4_ADMA.t_capture_start = time_us();
#endif

    if (num == 0b01) {
        hw_write_masked(&adc_hw->cs, (0<<16) | (0<<12), (0b11111<<16) | (0b111<<12));
    }
    else if (num == 0b10) {
        hw_write_masked(&adc_hw->cs, (0<<16) | (1<<12), (0b11111<<16) | (0b111<<12));
    }
    else if (num == 0b11) {
        hw_write_masked(&adc_hw->cs, (0b11<<16) | (0<<12), (0b11111<<16) | (0b111<<12));
    }
    else {
        return;
    }

    dma_hw->ch[DMA_ADC_CH].transfer_count = (1<<11);
    dma_hw->ch[DMA_ADC_CH].al2_write_addr_trig = (u32)endp4_in.data_buffer;
    hw_set_bits(&adc_hw->cs, (1<<3));
}


internal void adc_dma_isr(void) {
    asm volatile ("cpsid i");

    ep4_ADMA.num_writes++;

    i32 num_writes = ep4_ADMA.num_writes;
    i32 num_reads  = ep4_ADMA.num_reads;
    u16 idx_last   = ep4_ADMA.idx_last;

    asm volatile ("cpsie i");

#if defined(DBG_LOG) && defined(DBG_USB_EP4_XFER)
    ep4_ADMA.isr_time_idx++;
    ep4_ADMA.isr_time_idx &= DBG_NUM_EP4_REQS_MASK;
    ep4_ADMA.isr_time[ep4_ADMA.isr_time_idx] = time_us();
    ep4_ADMA.isr_reads[ep4_ADMA.isr_time_idx] = ep4_ADMA.num_writes;
#endif

    // if ((((num_writes - num_reads) < 2) && (idx_last > 0)) || (num_writes == num_reads)) {
    if ((((num_writes - num_reads) < 2) && (idx_last > 0)) || (num_writes == num_reads)) {
        dma_hw->ch[DMA_ADC_CH].transfer_count = (1<<11);
        dma_hw->ch[DMA_ADC_CH].al2_write_addr_trig = (u32)endp4_in.data_buffer;
    }
    else {
        hw_clear_bits(&adc_hw->cs, (1<<10) | (1<<3) | (1<<2));
        DBG_str(&dbg0, "Stopped ADC capturing.\n");

#if defined(DBG_LOG) && defined(DBG_USB_EP4_XFER)
        u64 t_now = time_us();

        DBG_str(&dbg0, "\tWould overwrite unread samples\n");
        DBG_str(&dbg0, "\tnum_writes ");
        DBG_hex(&dbg0, ep4_ADMA.num_writes);
        DBG_str(&dbg0, " ");
        DBG_hex(&dbg0, num_writes);
        DBG_str(&dbg0, "\tnum_reads ");
        DBG_hex(&dbg0, ep4_ADMA.num_reads);
        DBG_str(&dbg0, " ");
        DBG_hex(&dbg0, num_reads);
        DBG_str(&dbg0, "\tidx_last ");
        DBG_hex_16(&dbg0, ep4_ADMA.idx_last);
        DBG_str(&dbg0, " ");
        DBG_hex_16(&dbg0, idx_last);
        DBG_str(&dbg0, "\tsize ");
        DBG_hex_16(&dbg0, ep4_ADMA.size);
        DBG_str(&dbg0, "\n");


        // -----------------------------------------------------------
        //          adc_get_new_sample_batch calls
        // -----------------------------------------------------------
        u8 idx = ep4_ADMA.xfer_req_time_idx;
        u64 latest = ep4_ADMA.xfer_req_time[idx];

        idx++;
        idx &= DBG_NUM_EP4_REQS_MASK;
        u64 prev = ep4_ADMA.xfer_req_time[idx];

        DBG_str(&dbg0, "Time of the latest requests on EP4:\n");
        // DBG_str(&dbg0, "\ttime of req.[us]: \tsince[us]: \tdt[us]:\n");
        DBG_str(&dbg0, "\ttime of req.[us]: \tsince[us]: \tdt[us]: \tsize: \toffset:\n");
        for (u8 i = 0; i < DBG_NUM_EP4_REQS; i++) {
            u8 j = (idx + i) & DBG_NUM_EP4_REQS_MASK;
            u64 next = ep4_ADMA.xfer_req_time[j];

            DBG_str(&dbg0, "\t");
            DBG_hex_64(&dbg0, next);

            DBG_str(&dbg0, "\t");
            DBG_hex(&dbg0, (u32)(latest - next));

            DBG_str(&dbg0, "\t");
            DBG_hex(&dbg0, (u32)(next - prev));

            DBG_str(&dbg0, "\t");
            DBG_hex(&dbg0, ep4_ADMA.xfer_req_size[j]);
            DBG_str(&dbg0, "\t");
            DBG_hex(&dbg0, ep4_ADMA.xfer_req_offset[j]);

            DBG_str(&dbg0, "\n");

            prev = next;
        }
        // -----------------------------------------------------------


        // -----------------------------------------------------------
        //          adc_dam_isr calls  (this function)
        // -----------------------------------------------------------
        u8 idx_isr = ep4_ADMA.isr_time_idx;
        u64 latest_isr = ep4_ADMA.isr_time[idx_isr];

        idx_isr++;
        idx_isr &= DBG_NUM_EP4_REQS_MASK;
        u64 prev_isr = ep4_ADMA.isr_time[idx_isr];

        DBG_str(&dbg0, "Time of the latest adc_dam_isr requests:\n");
        DBG_str(&dbg0, "\ttime of req.[us]: \tsince[us]: \tdt[us]: \treads\n");
        for (u8 i = 0; i < DBG_NUM_EP4_REQS; i++) {
            u8 j = (idx_isr + i) & DBG_NUM_EP4_REQS_MASK;
            u64 next_isr = ep4_ADMA.isr_time[j];

            DBG_str(&dbg0, "\t");
            DBG_hex_64(&dbg0, next_isr);

            DBG_str(&dbg0, "\t");
            DBG_hex(&dbg0, (u32)(latest_isr - next_isr));

            DBG_str(&dbg0, "\t");
            DBG_hex(&dbg0, (u32)(next_isr - prev_isr));

            DBG_str(&dbg0, "\t");
            DBG_hex(&dbg0, ep4_ADMA.isr_reads[j]);

            DBG_str(&dbg0, "\n");

            prev_isr = next_isr;
        }
        // -----------------------------------------------------------


        DBG_str(&dbg0, "\tcapture start time [us]: ");
        DBG_hex_64(&dbg0, ep4_ADMA.t_capture_start);
        DBG_str(&dbg0, "\n");
        DBG_str(&dbg0, "\tcurrent time [us]: ");
        DBG_hex_64(&dbg0, t_now);
        DBG_str(&dbg0, "\n");
        DBG_str(&dbg0, "\ttime since the last req. [us]: ");
        DBG_hex_64(&dbg0, t_now - latest);
        DBG_str(&dbg0, "\n");
        DBG_str(&dbg0, "\ttime since the start of capturing. [us]: ");
        DBG_hex_64(&dbg0, t_now - ep4_ADMA.t_capture_start);
        DBG_str(&dbg0, "\n");
#endif
    }
}


#endif
