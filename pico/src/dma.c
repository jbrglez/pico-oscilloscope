#ifndef DMA_C
#define DMA_C

#include "hardware.h"
#include "config.h"
#include "adc.c"
#include "pio.c"


void isr_dma_1(void) {
    u32 remaining_ints = dma_hw->ints1;

    for (u32 i = 0; remaining_ints && (i < 12); i++) {
        u32 bit = 1 << i;
        if (remaining_ints & bit) {
            remaining_ints ^= bit;
            hw_set_bits(&dma_hw->ints1, bit);

            if (i == DMA_ADC_CH) {
                adc_dma_isr();
            }
            else if (i == DMA_SIG_COPY_CH) {
                ep_transfer(&endp0_out, 0x40);
                if (sig_buffers_PIO.status == SIG_BUF_NEW_DATA_IN_TRANSFER) {
                    sig_buffers_PIO.status = SIG_BUF_NORMAL;
                }
                else if (sig_buffers_PIO.status == SIG_BUF_SWITCH_BUFFERS) {
                    sig_gen_data_t *tmp = sig_buffers_PIO.active_buf;
                    sig_buffers_PIO.active_buf = sig_buffers_PIO.inactive_buf;
                    sig_buffers_PIO.inactive_buf = tmp;
                    sig_buffers_PIO.status = SIG_BUF_NORMAL;
                }
            }
        }
    }
}


internal void dma_configure_transfers(void) {
    hw_set_bits(&resets_hw->reset, RESET_DMA);
    hw_clear_bits(&resets_hw->reset, RESET_DMA);
    while (!(resets_hw->reset_done & RESET_DMA));

    if (sio_hw->cpuid == 0) {
        nvic_hw->icpr = DMA_IRQ_0;
        nvic_hw->iser = DMA_IRQ_0;
        nvic_hw->icpr = DMA_IRQ_1;
        nvic_hw->iser = DMA_IRQ_1;
    }

    dma_hw->inte1 = (1 << DMA_ADC_CH) | (1 << DMA_SIG_COPY_CH);
    dma_hw->ch[DMA_SIG_COPY_CH].al1_ctrl = (0x3f << 15) | (DMA_SIG_COPY_CH << 11) | (1<<5) | (1<<4) | (2<<2) | (1<<0);

    dma_hw->ch[DMA_ADC_CH].read_addr  = (u32)(&adc_hw->fifo);
    dma_hw->ch[DMA_ADC_CH].write_addr = ((u32)usb_dpram + (1<<11));
    dma_hw->ch[DMA_ADC_CH].transfer_count = (1<<11);
    dma_hw->ch[DMA_ADC_CH].al1_ctrl = (36<<15) | (DMA_ADC_CH << 11) | (1<<10) | (11<<6) | (1<<5) | (0<<2) | (1<<0);

    // adc_hw->div = (47999<<8);       // Set Sampling Rate / Clock Divider
    // adc_hw->fcs = (1<<24) | (1<<3) | (1<<1) | (1<<0);

    dma_hw->inte0 = (1 << DMA_PIO_CH_1) | (1 << DMA_PIO_CH_2);
    dma_hw->ch[DMA_PIO_CH_1].write_addr = (u32)(&pio0_hw->txf[0]);
    dma_hw->ch[DMA_PIO_CH_1].al1_ctrl = (0<<21) | (0<<15) | (DMA_PIO_CH_2 << 11) | (0<<5) | (1<<4) | (0<<2) | (1<<1) | (1<<0);
    dma_hw->ch[DMA_PIO_CH_2].write_addr = (u32)(&pio0_hw->txf[0]);
    dma_hw->ch[DMA_PIO_CH_2].al1_ctrl = (0<<21) | (0<<15) | (DMA_PIO_CH_1 << 11) | (0<<5) | (1<<4) | (0<<2) | (1<<1) | (1<<0);
}


internal void activate_pio_dma(void) {
    dma_hw->ch[DMA_PIO_CH_1].transfer_count = (u32)sig_buffers_PIO.active_buf->length;
    dma_hw->ch[DMA_PIO_CH_2].transfer_count = (u32)sig_buffers_PIO.active_buf->length;
    dma_hw->ch[DMA_PIO_CH_2].read_addr = (u32)(sig_buffers_PIO.active_buf->data);
    dma_hw->ch[DMA_PIO_CH_1].al3_read_addr_trig = (u32)(sig_buffers_PIO.active_buf->data);
}


void isr_dma_0(void) {
    u32 remaining_ints = dma_hw->ints0;

    for (u32 i = 0; remaining_ints && (i < 12); i++) {
        u32 bit = 1 << i;
        if (remaining_ints & bit) {
            remaining_ints ^= bit;
            hw_set_bits(&dma_hw->ints0, bit);

            if (i == DMA_PIO_CH_1) {
                pio_dma_configure_ch(DMA_PIO_CH_1);
            }
            else if (i == DMA_PIO_CH_2) {
                pio_dma_configure_ch(DMA_PIO_CH_2);
            }
        }
    }
}


#endif
