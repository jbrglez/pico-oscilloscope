#include "config.h"
#include "hardware.h"
#include "my_math.c"
#include "uart.c"
#include "debug_str.c"
#include "clocks.c"

#include "adc.c"
#include "usb.c"
#include "dma.c"
#include "pio.c"
#include "i2c.c"



internal void busy_wait_cycles(u32 num_cycles) {
    asm volatile (
        ".syntax unified\n"
        "1: subs %0, #3\n"
        "bcs 1b\n"
    : "+l" (num_cycles) : : "cc", "memory"
    );
}


internal void init_out_pin(i32 n) {
    hw_clear_bits(&sio_hw->gpio_oe, 1<<n);
    hw_clear_bits(&sio_hw->gpio_out, 1<<n);
    io_bank0_hw->io[n].ctrl = GPIO_FUNC_SIO;
    sio_hw->gpio_oe_set = 1<<n;
}


internal void pin_to_1(i32 n) {
        sio_hw->gpio_out_set = 1<<n;
}

internal void pin_to_0(i32 n) {
        sio_hw->gpio_out_clr = 1<<n;
}

internal void pin_switch(i32 n) {
        sio_hw->gpio_out_xor = 1<<n;
}



int runtime_init() {
#ifdef DBG_LOG
    init_debug_str(&dbg0, 13);
    init_debug_str(&dbg_usb, 12);
#endif
    return 0;
}



int main() {
    hw_clear_bits(&resets_hw->reset, RESET_IO_BANK0);
    while (!(resets_hw->reset_done & RESET_IO_BANK0));

    // Force the power supply into PWM mode.
    init_out_pin(23);
    pin_to_1(23);
    // -------------------------------------

    init_out_pin(25);
    init_out_pin(LED_1_PIN);
    init_out_pin(LED_2_PIN);

    pin_to_1(25);

    clocks_init();

    uart_init();


    uart_puts("\r\e[2J"); // Clear screen
    uart_puts(" Starting \n");
    uart_puts("======================================\n");
    uart_puts("======================================\n");


    init_reset_PIO();

    // init_pio_in_pin(pio0_hw, 0, 2);
    for (i32 i = 0; i < PIO_NUM_OUT_PINS; i++) {
        init_pio_out_pin(pio0_hw, 0, PIO_FIRST_OUT_PIN + i);
    }

    #define PIO_SIG_SM_NUM 0
    load_pio_program(&sig_gen, pio0_hw, PIO_SIG_SM_NUM,
                     PIO_FIRST_OUT_PIN, PIO_NUM_OUT_PINS,
                     0, 0, 0, 2, 1, 0);
    init_signal_gen_program(pio0_hw, PIO_SIG_SM_NUM);


    init_adc();
    dma_configure_transfers();
    init_usb();

    activate_pio_dma();

    pin_to_1(LED_1_PIN);

    u32 i = 0;
    while (!(usb_state.state == USB_STATE_CONFIGURED));
    ep_transfer(&endp1_out, 64);
    ep_transfer(&endp2_in, 64);
    endp2_in.next_data_pid ^= 1;
    ep_transfer(&endp3_in, 64);
    endp3_in.next_data_pid ^= 1;
    ep_transfer(&endp4_in, 0);

    pin_to_1(LED_2_PIN);



#ifdef CONFIG_USE_I2C
    i2c_init();
    bus_scan();
    i2c_config_temp_sensor();
#endif

    uart_puts("\n<***************>\r\n\n");


#ifdef DBG_LOG
    uart_puts("DEBUG LOG:\n");
    uart_puts("----------------------------------------------------------------\n");
#endif

    while(1) {
        busy_wait_us(100 * 1000);

        pin_switch(LED_1_PIN);

#ifdef DBG_LOG
        if ((dbg0.write_pos != dbg0.read_pos) ||
            (dbg_usb.write_pos != dbg_usb.read_pos)
        ) {
            uart_puts("\r\e[J");
            DBG_print(&dbg_usb);
            DBG_print(&dbg0);
        }
#endif

        uart_puts("----------------------------------------------------------------\n\n");


        uart_puts("time since boot [s]: ");
        uart_hex((u32)time_s());
#ifdef CONFIG_USE_I2C
        i32 curr_temp = i2c_try_read_byte_blocking();
        i2c_sensor_temp = (curr_temp != -1) ? (curr_temp & 0xFF) : 0;
        uart_puts("\t\t temperature[C]: ");
        uart_hex_8(i2c_sensor_temp);
#endif
        uart_puts("\n");
        uart_puts("\t USB:\n");

        uart_puts("\t ep_nak_stall_status: ");
        uart_hex(usb_hw->ep_nak_stall_status);
        uart_puts(" \tep_stall_arm: ");
        uart_hex(usb_hw->ep_stall_arm);
        uart_puts(" \tsie_status: ");
        uart_hex(usb_hw->sie_status);
        uart_puts("\n");

        uart_puts("\t ep_buf_ctrl[0].in: ");
        uart_hex(usb_dpram->ep_buf_ctrl[0].in);
        uart_puts(" \tep_buf_ctrl[0].out: ");
        uart_hex(usb_dpram->ep_buf_ctrl[0].out);

        uart_puts("\n\t ep_buf_ctrl[1].out:  ");
        uart_hex(usb_dpram->ep_buf_ctrl[1].out);
        uart_puts("\t ep_buf_ctrl[4].in:  ");
        uart_hex(usb_dpram->ep_buf_ctrl[4].in);

        uart_puts("\n\t ep4_ADMA:");
        uart_puts("\tidx_last:  ");
        uart_hex(ep4_ADMA.idx_last);
        uart_puts("\twrites:  ");
        uart_hex(ep4_ADMA.num_writes);
        uart_puts("\treads:  ");
        uart_hex(ep4_ADMA.num_reads);


        uart_puts("\n");


        uart_puts("\n\t ADC:");
        uart_puts("\t CS:  ");
        uart_hex(adc_hw->cs);
        uart_puts("\t FCS:  ");
        uart_hex(adc_hw->fcs);
        uart_puts("\t INTS:  ");
        uart_hex(adc_hw->ints);


        uart_puts("\n\t I2C: ");
        uart_puts("\t STATUS:  ");
        uart_hex(i2c0_hw->status);
        uart_puts("\t CON:  ");
        uart_hex(i2c0_hw->con);
        uart_puts("\t tar:  ");
        uart_hex(i2c0_hw->tar);

        uart_puts("\n\t");
        uart_puts("\t txflr:  ");
        uart_hex(i2c0_hw->txflr);
        uart_puts("\t rxflr:  ");
        uart_hex(i2c0_hw->rxflr);
        uart_puts("\t RAW_INTR_STAT:  ");
        uart_hex(i2c0_hw->raw_intr_stat);
        uart_puts("\t TX_ABRT_SOURCE:  ");
        uart_hex(i2c0_hw->tx_abrt_source);


        uart_puts("\n");


        uart_puts("\n\t DMA[DMA_ADC_CH]:");
        uart_puts("\tread:  ");
        uart_hex(dma_hw->ch[DMA_ADC_CH].read_addr);
        uart_puts("\twrite:  ");
        uart_hex(dma_hw->ch[DMA_ADC_CH].write_addr);
        uart_puts("\tcount:  ");
        uart_hex(dma_hw->ch[DMA_ADC_CH].transfer_count);
        uart_puts("\tctrl:  ");
        uart_hex(dma_hw->ch[DMA_ADC_CH].ctrl_trig);
        uart_puts("\tdbg_tcr:  ");
        uart_hex(dma_debug_hw->ch[DMA_ADC_CH].dbg_tcr);


        uart_puts("\n\t DMA[DMA_PIO_CH_1]:");
        uart_puts("\tread:  ");
        uart_hex(dma_hw->ch[DMA_PIO_CH_1].read_addr);
        uart_puts("\twrite:  ");
        uart_hex(dma_hw->ch[DMA_PIO_CH_1].write_addr);
        uart_puts("\tcount:  ");
        uart_hex(dma_hw->ch[DMA_PIO_CH_1].transfer_count);
        uart_puts("\tctrl:  ");
        uart_hex(dma_hw->ch[DMA_PIO_CH_1].ctrl_trig);
        uart_puts("\tdbg_tcr:  ");
        uart_hex(dma_debug_hw->ch[DMA_PIO_CH_1].dbg_tcr);

        uart_puts("\n\t DMA[DMA_PIO_CH_2]:");
        uart_puts("\tread:  ");
        uart_hex(dma_hw->ch[DMA_PIO_CH_2].read_addr);
        uart_puts("\twrite:  ");
        uart_hex(dma_hw->ch[DMA_PIO_CH_2].write_addr);
        uart_puts("\tcount:  ");
        uart_hex(dma_hw->ch[DMA_PIO_CH_2].transfer_count);
        uart_puts("\tctrl:  ");
        uart_hex(dma_hw->ch[DMA_PIO_CH_2].ctrl_trig);
        uart_puts("\tdbg_tcr:  ");
        uart_hex(dma_debug_hw->ch[DMA_PIO_CH_2].dbg_tcr);


        uart_puts("\n");


        uart_puts("\n\t PIO:");
        uart_puts("\tFLEVELS:  ");
        uart_hex(pio0_hw->flevel);


        uart_puts("\n\t PIO_buffers:  ");
        uart_puts("\t status:  ");
        uart_hex(sig_buffers_PIO.status);

        uart_puts("\n\t\t active:  ");
        uart_puts("\t data: ");
        uart_hex((u32)sig_buffers_PIO.active_buf->data);
        uart_puts("\t length: ");
        uart_hex((u32)sig_buffers_PIO.active_buf->length);
        uart_puts("\n\t\t inactive:  ");
        uart_puts("\t data: ");
        uart_hex((u32)sig_buffers_PIO.inactive_buf->data);
        uart_puts("\t length: ");
        uart_hex((u32)sig_buffers_PIO.inactive_buf->length);

#if 0
        i32 num_new_lines = 0;

        uart_puts("\n");

        #define SHOW_BUF_LEN 32
        uart_puts("\n\t SIG_active:[0:]");
        for (i32 i = 0; i < SHOW_BUF_LEN; i++) {
            if ((i & 0xF) == 0) {
                uart_puts("\n\t");
                num_new_lines++;
            }
            uart_hex_8(sig_buffers_PIO.active_buf->data[i]);
            uart_puts(" ");
        }
        uart_puts("\n\t SIG_active:[:-1]");
        for (i32 i = SHOW_BUF_LEN; i > 0; i--) {
            if ((i & 0xF) == 0) {
                uart_puts("\n\t");
                num_new_lines++;
            }
            uart_hex_8(sig_buffers_PIO.active_buf->data[sig_buffers_PIO.active_buf->length - i]);
            uart_puts(" ");
        }


        uart_puts("\n");

        uart_puts("\n\t SIG_inactive:[0:]");
        for (i32 i = 0; i < SHOW_BUF_LEN; i++) {
            if ((i & 0xF) == 0) {
                uart_puts("\n\t");
                num_new_lines++;
            }
            uart_hex_8(sig_buffers_PIO.inactive_buf->data[i]);
            uart_puts(" ");
        }
        uart_puts("\n\t SIG_inactive:[:-1]");
        for (i32 i = SHOW_BUF_LEN; i > 0; i--) {
            if ((i & 0xF) == 0) {
                uart_puts("\n\t");
                num_new_lines++;
            }
            uart_hex_8(sig_buffers_PIO.active_buf->data[sig_buffers_PIO.active_buf->length - i]);
            uart_puts(" ");
        }

        for (i32 i = 0; i < num_new_lines + 6; i++) {
            uart_puts("\r\e[A");
        }
#endif

        uart_puts("\r\e[20A");
    }

    return 0;
}



int exit() {
	return -1;
}

