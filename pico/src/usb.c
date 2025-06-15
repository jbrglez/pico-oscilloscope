#ifndef USB_C
#define USB_C

#include "hardware.h"

#include "usb.h"
#include "usb_ep.h"
#include "RW_pico_usb.c"

#include "uart.c"
#include "debug_str.c"
#include "signal_buffers.c"

#include "adc.c"
#include "i2c.c"


#define USB_INTS_EP_STALL_NAK   (1<<19)
#define USB_INTS_SETUP_REQ      (1<<16)
#define USB_INTS_BUS_RESET      (1<<12)
#define USB_INTS_BUFF_STATUS    (1<<4)



#ifdef DBG_LOG
static debug_str_t dbg_usb;
#endif

volatile usb_setup_packet *setup_packet = (volatile usb_setup_packet *)(usb_dpram->setup_packet);

typedef struct {
    volatile u8 should_set_address;
    volatile u8 state;
    volatile u8 dev_address;
    volatile u8 configuration;
} usb_state_t;

usb_state_t usb_state = {
    .should_set_address = 0,
    .dev_address = 0,
    .state = USB_STATE_DEFAULT,
    .configuration = 0,
};


internal endp_t *get_ep_by_addr(u32 addr) {
    switch (addr) {
        case 0x80:
            return &endp0_in;
        case 0x00:
            return &endp0_out;
        case 0x01:
            return &endp1_out;
        case 0x82:
            return &endp2_in;
        case 0x83:
            return &endp3_in;
        case 0x84:
            return &endp4_in;
        default:
            return 0;
    }
}


internal void ep_stall(endp_t *ep) {
    if (ep->id == 0) {
        hw_set_bits(&usb_hw->ep_stall_arm, (ep->dir == 1) ? (1<<0) : (1<<1));
    }
    *ep->buf_ctrl |= (1<<11);
}


internal void ep_clear_stall(endp_t *ep) {
    // if (ep->id != 0) {
    ep->next_data_pid = 0;
    usb_dpram->ep_buf_ctrl[ep->id].in &= ~(1<<11);
    // }
}


internal void ep_transfer(endp_t *ep, u32 len) {
    u32 val = (ep->dir << 15) | (ep->next_data_pid << 13) | len;
    *ep->buf_ctrl = val;
    asm volatile("nop");
    asm volatile("nop");
    asm volatile("nop");
    *ep->buf_ctrl = val | (1<<10);
}


internal void init_usb(void) {
    hw_set_bits(&resets_hw->reset, RESET_USBCTRL);
    hw_clear_bits(&resets_hw->reset, RESET_USBCTRL);
    while (!(resets_hw->reset_done & RESET_USBCTRL));

    i32 *p = (i32 *)(usb_dpram);
    for (i32 i = 0; i < sizeof(usb_device_dpram_t) / 4; i++) {
        p[i] = 0;
    }
    // NOTE: Enable interrupt only on core 0. (core 1 should not be running)
    if (sio_hw->cpuid == 0) {
        nvic_hw->icpr = USBCTRL_IRQ;
        nvic_hw->iser = USBCTRL_IRQ;
    }

    usb_hw->muxing = (1<<3) | (1<<0);
    usb_hw->pwr = (1<<3) | (1<<2);      // Force VBUS detect so the device thinks it is plugged into a host
    usb_hw->main_ctrl = (1<<0);
    // usb_hw->sie_ctrl = (1<<29);         // Enable an interrupt per EP0 transaction
    // usb_hw->inte = (1<<19) | (1<<16) | (1<<12) | (1<<4);
    usb_hw->sie_ctrl = (1<<31) | (1<<29);
    usb_hw->inte = USB_INTS_EP_STALL_NAK | USB_INTS_SETUP_REQ |
                   USB_INTS_BUS_RESET | USB_INTS_BUFF_STATUS;

    usb_dpram->ep_ctrl[0].out = (1<<31) | (1<<29) | (endp1_out.descriptor->bmAttributes << 26) | DPRAM_OFFSET(endp1_out.data_buffer);
    usb_dpram->ep_ctrl[1].in  = (1<<31) | (1<<29) | ( endp2_in.descriptor->bmAttributes << 26) | DPRAM_OFFSET(endp2_in.data_buffer);
    usb_dpram->ep_ctrl[2].in  = (1<<31) | (1<<29) | ( endp3_in.descriptor->bmAttributes << 26) | DPRAM_OFFSET(endp3_in.data_buffer);
    usb_dpram->ep_ctrl[3].in  = (1<<31) | (1<<29) | ( endp4_in.descriptor->bmAttributes << 26) | DPRAM_OFFSET(endp4_in.data_buffer);

    hw_set_bits(&usb_hw->sie_ctrl, (1<<16));

    usb_hw->main_ctrl = 1;
}


internal void endp4_next_transfer(void) {
    new_sample_batch_t new_samples_batch = adc_get_new_sample_batch();

    usb_dpram->ep_ctrl[3].in  = (1<<31) | (1<<29) 
        | ( ep4_in.bmAttributes << 26) 
        | new_samples_batch.start_offset_into_usb_dpram;

    ep_transfer(&endp4_in, new_samples_batch.size);
    endp4_in.next_data_pid ^= 1;
}


internal void copy_buf(u8 *src, u8 *dest, i32 len) {
    for (i32 i = 0; i < len; i++) {
        dest[i] = src[i];
    }
}


internal void usb_handle_get_descriptor(void) {
    u8 DType  = setup_packet->wValue >> 8;
    u8 DIndex = setup_packet->wValue & 0xFF;

    u32 requested_len = 0x3FF & setup_packet->wLength;

    switch (DType) {
        case DESCRIPTOR_DEVICE:
            copy_buf((u8 *)&device_descriptor, &usb_dpram->ep0_buf_a[0], sizeof(device_descriptor));
            ep_transfer(&endp0_in, MIN(sizeof(device_descriptor), requested_len));
            break;

        case DESCRIPTOR_CONFIGURATION: {
            u8 *buf = (u8 *)&usb_dpram->ep0_buf_a[0];

            copy_buf((u8 *)&config_descriptor, buf, sizeof(usb_configuration_descriptor));
            buf += sizeof(usb_configuration_descriptor);
            copy_buf((u8 *)&interface_descriptor, buf, sizeof(usb_interface_descriptor));
            buf += sizeof(usb_interface_descriptor);
            copy_buf((u8 *)&ep1_out, buf, sizeof(interface_descriptor));
            buf += sizeof(usb_endpoint_descriptor);
            copy_buf((u8 *)&ep2_in, buf, sizeof(interface_descriptor));
            buf += sizeof(usb_endpoint_descriptor);
            copy_buf((u8 *)&ep3_in, buf, sizeof(interface_descriptor));
            buf += sizeof(usb_endpoint_descriptor);
            copy_buf((u8 *)&ep4_in, buf, sizeof(interface_descriptor));
            buf += sizeof(usb_endpoint_descriptor);

            u32 len = (u32)buf - (u32)&usb_dpram->ep0_buf_a[0];
            ep_transfer(&endp0_in, MIN(len, requested_len));

            break;
        }

        case DESCRIPTOR_STRING: {
            u32 len = 0;
            if (DIndex == 0) {
                u8 str_desc[4] = { sizeof(str_desc), DESCRIPTOR_STRING, 0x09, 0x04};
                copy_buf((u8 *)&str_desc, &usb_dpram->ep0_buf_a[0], 4);
                len = sizeof(str_desc);
            }
            else if (DIndex == 1) {
                u8 str_vendor[] = { 0, DESCRIPTOR_STRING, 'R', 0, 'P', 0, 'i', 0 };
                len = sizeof(str_vendor);
                str_vendor[0] = len;
                copy_buf((u8 *)&str_vendor, &usb_dpram->ep0_buf_a[0], len);
            }
            else if (DIndex == 2) {
                u8 str_product[] = { 0, DESCRIPTOR_STRING, 'P', 0, 'i', 0, 'c', 0, 'o', 0, ' ', 0, 'O', 0, 's', 0, 'c', 0, 'i', 0, 'l', 0, 'o', 0, 's', 0, 'c', 0, 'o', 0, 'p', 0, 'e', 0};
                len = sizeof(str_product);
                str_product[0] = len;
                copy_buf((u8 *)&str_product, &usb_dpram->ep0_buf_a[0], len);
            }
            else {
                ep_stall(&endp0_in);
                return;
            }
            ep_transfer(&endp0_in, MIN(len, 0x3FF & setup_packet->wLength));

            break;
        }

        case DESCRIPTOR_INTERFACE_POWER:
        case DESCRIPTOR_DEVICE_QUALIFIER:
        case DESCRIPTOR_OTHER_SPEED_CONFIGURATION:
        default:
            ep_stall(&endp0_in);
            break;
    }
}


internal void usb_handle_setup_packet_standard(void) {
    u8 direction_IN = setup_packet->bmRequestType & (1<<7);
    u8 req_recipient = setup_packet->bmRequestType & 0b11;
    u8 request = setup_packet->bRequest;

    endp0_in.next_data_pid = 1;

    if (direction_IN) {
        switch (request) {
            case GET_STATUS: {
                u16 intf_endpt = setup_packet->wIndex;

                if (usb_state.state == USB_STATE_ADDRESSED) {
                    if ((req_recipient == USB_REQUEST_RECIPIENT_DEVICE) && (intf_endpt == 0)) {
                        endp0_in.data_buffer[0] = 0;
                        endp0_in.data_buffer[1] = 0;
                        ep_transfer(&endp0_in, 2);
                    }
                    else {
                        ep_stall(&endp0_in);
                        return;
                    }
                }
                else if (usb_state.state == USB_STATE_CONFIGURED) {
                    switch (req_recipient) {
                        case USB_REQUEST_RECIPIENT_DEVICE:
                        case USB_REQUEST_RECIPIENT_INTERFACE:
                            if (intf_endpt != 0) {
                                ep_stall(&endp0_in);
                                return;
                            }
                            endp0_in.data_buffer[0] = 0;
                            endp0_in.data_buffer[1] = 0;
                            ep_transfer(&endp0_in, 2);
                        break;

                        case USB_REQUEST_RECIPIENT_ENDPOINT: {
                            endp0_in.data_buffer[1] = 0;
                            endp_t *ep = get_ep_by_addr(intf_endpt);
                            if (ep != 0) {
                                endp0_in.data_buffer[0] = (*ep->buf_ctrl >> 11) & 1;
                                ep_transfer(&endp0_in, 2);
                                return;
                            }
                            else {
                                ep_stall(&endp0_in);
                                return;
                            }
                            break;

                        default:
                            ep_stall(&endp0_in);
                            return;
                            break;
                        }
                    }
                }
                else {
                    ep_stall(&endp0_in);
                    return;
                }

                break;
            }

            case GET_DESCRIPTOR:
                usb_handle_get_descriptor();
                break;

            case GET_CONFIGURATION:
                *(u8 *)&usb_dpram->ep0_buf_a[0] = usb_state.configuration;
                ep_transfer(&endp0_in, 1);
                break;

            case GET_INTERFACE: {
                u16 interface = setup_packet->wIndex;
                if ((usb_state.state == USB_STATE_CONFIGURED) && (interface == 0)) {
                    *(u8 *)&usb_dpram->ep0_buf_a[0] = 0;
                    ep_transfer(&endp0_in, 1);
                }
                else {
                    ep_stall(&endp0_in);
                }
                break;
            }

            // case SYNC_FRAME:
            //     break;

            default:
                uart_puts("\nEP0_IN stall SET\n\n\n");
                ep_stall(&endp0_in);
                break;
        }
    }
    else {
        u8 DIndex = setup_packet->wValue & 0xFF;
        switch (request) {
            case CLEAR_FEATURE: {
                u16 feature = setup_packet->wValue;
                u16 intf_endpt = setup_packet->wIndex;
                if (req_recipient == USB_REQUEST_RECIPIENT_DEVICE) {
                }
                else if (req_recipient == USB_REQUEST_RECIPIENT_ENDPOINT) {
                    endp_t *ep = get_ep_by_addr(intf_endpt);
                    if ((feature == ENDPOINT_HALT) && (ep != 0) && (usb_state.state == USB_STATE_CONFIGURED)) {
                        ep_clear_stall(ep);
                    }
                    else {
                        ep_stall(&endp0_out);
                    }
                }
                else {
                    ep_stall(&endp0_out);
                }
                break;
            }

            case SET_FEATURE: {
                u16 feature = setup_packet->wValue;
                u16 intf_endpt = setup_packet->wIndex;
                endp_t *ep = get_ep_by_addr(intf_endpt);
                if ((req_recipient == USB_REQUEST_RECIPIENT_ENDPOINT) &&
                    (feature == ENDPOINT_HALT) &&
                    (ep != 0) && (ep->id > 0) &&
                    (usb_state.state == USB_STATE_CONFIGURED)) {
                        ep_stall(ep);
                }
                else {
                    ep_stall(&endp0_out);
                }
                break;
            }

            case SET_ADDRESS:
                usb_state.dev_address = (setup_packet->wValue & 0xff);
                usb_state.should_set_address = 1;
                ep_transfer(&endp0_in, 0);
                break;

            case SET_CONFIGURATION:
                if (usb_state.state == USB_STATE_ADDRESSED || usb_state.state == USB_STATE_CONFIGURED) {
                    if (DIndex == 0) {
                        usb_state = (usb_state_t){
                            .should_set_address = 0,
                            .dev_address = 0,
                            .state = USB_STATE_DEFAULT,
                            .configuration = 0,
                        };
                    }
                    else if (DIndex == 1) {
                        usb_state.state = USB_STATE_CONFIGURED;
                        usb_state.configuration = 1;
                    }
                    else  {
                        ep_stall(&endp0_out);
                        DBG_str(&dbg_usb, "\n\n ERROR: USB SET_CONFIGURATION recieved invalid configuration number.\n\n");
                    }
                    ep_transfer(&endp0_in, 0);
                }
                else {
                    ep_stall(&endp0_out);
                }
                break;

            case SET_DESCRIPTOR:
            case SET_INTERFACE:
            default:
                ep_stall(&endp0_out);
                break;
        }
    }
}


internal void usb_handle_setup_packet_class(void) {
    u8 direction_IN = (setup_packet->bmRequestType >> 7) & 1;
    if (direction_IN) {
        ep_stall(&endp0_in);
    }
    else {
        ep_stall(&endp0_out);
    }
}


typedef enum {
    PICO_GET_OSCI_STATUS = 1,
    PICO_SET_OSCI_STATUS = 2,
    PICO_START_RUNNING_CAPTURE = 3,
    PICO_TEST_ADC = 4,
    PICO_NEW_SIGNAL = 5,
    PICO_GET_TEMPERATURE = 6,
} pico_request;


static volatile u8 global_var = 0;


internal void usb_handle_setup_packet_vendor(void) {
    u8 direction_IN = (setup_packet->bmRequestType >> 7) & 1;
    u8 req_recipient = setup_packet->bmRequestType & 0b11;
    u8 request = setup_packet->bRequest;

    u32 len = 0;
    if (direction_IN) {
        switch (request) {
            case PICO_GET_OSCI_STATUS:
                len = 32;
                for (i32 i = 0; i < len; i++) {
                    usb_dpram->ep0_buf_a[i] = global_var+i;
                }
                ep_transfer(&endp0_in, len);
                // MIN(len, 0x3FF & setup_packet->wLength)
                break;
            case PICO_TEST_ADC:
                *(u16 *)endp0_in.data_buffer = adc_get_sample();
                ep_transfer(&endp0_in, 2);
                break;
            case PICO_GET_TEMPERATURE:
                *(i8 *)endp0_in.data_buffer = i2c_sensor_temp;
                ep_transfer(&endp0_in, 1);
                break;
            default:
                ep_stall(&endp0_in);
                break;
        }
    }
    else {
        switch (request) {
            case PICO_SET_OSCI_STATUS:
                global_var = setup_packet->wValue;
                ep_transfer(&endp0_in, 0);
                break;

            case  PICO_START_RUNNING_CAPTURE:
                ep_transfer(&endp4_in, 0);
                adc_stop_capturing();
                if ((setup_packet->wValue == 0b01) ||
                    (setup_packet->wValue == 0b10) ||
                    (setup_packet->wValue == 0b11)
                ) {
                    adc_start_capture_ep4(setup_packet->wValue);
                }
                ep_transfer(&endp0_in, 0);
                break;

            case PICO_NEW_SIGNAL:
                if (setup_packet->wValue & 2) {
                    sig_buffers_PIO.inactive_buf->length = setup_packet->wIndex;
                    sig_buffers_PIO.buf_copy_addr_offset = 0;
                    // sig_buffers_PIO.buf_copy_len = (setup_packet->wLength + 3) / 4;
                    sig_buffers_PIO.buf_copy_len = (setup_packet->wLength + 3) >> 2;
                }
                else {
                    sig_buffers_PIO.buf_copy_addr_offset = setup_packet->wIndex;
                    // sig_buffers_PIO.buf_copy_len = (setup_packet->wLength + 3) / 4;
                    sig_buffers_PIO.buf_copy_len = (setup_packet->wLength + 3) >> 2;
                }

                sig_buffers_PIO.status = (setup_packet->wValue & 1) ? SIG_BUF_SWITCH_BUFFERS : SIG_BUF_NEW_DATA_IN_TRANSFER;

                ep_transfer(&endp0_in, 0);

                break;

            default:
                ep_stall(&endp0_out);
                break;
        }
    }
}


internal void usb_handle_stall_nak() {
    uint32_t remaining_buffers = usb_hw->ep_nak_stall_status;

    uart_puts("\n USB STALL NAK rec\n");
    uart_puts("nak_stall_status: ");
    uart_hex(remaining_buffers);
    uart_puts(" \tep_stall_arm: ");
    uart_hex(usb_hw->ep_stall_arm);
    uart_puts(" \tsie_status: ");
    uart_hex(usb_hw->sie_status);

    uart_puts("\n");

    uart_puts("ep_buf_ctrl_IN ");
    uart_hex(usb_dpram->ep_buf_ctrl[0].in);
    uart_puts(" \tep_buf_ctrl_OUT ");
    uart_hex(usb_dpram->ep_buf_ctrl[0].out);

    uart_puts("\n\n\n\n");

    for (u32 i = 0; remaining_buffers && i < USB_NUM_ENDPOINTS * 2; i++) {
        u32 bit = 1 << i;
        if (remaining_buffers & bit) {
            // CHECK: Why does EP0 OUT bit get cleared by its own, but EP0 IN bit does not?
            hw_clear_bits(&usb_hw->ep_nak_stall_status, bit);

            // if (i == 0) {
            //     ep_clear_stall(&endp0_in);
            // }
            // ep_clear_stall(&endp0_in);

            remaining_buffers ^= bit;
        }
    }
}


internal void usb_handle_buff_status() {
    uint32_t remaining_buffers = usb_hw->buf_status;

    for (u32 i = 0; remaining_buffers && i < USB_NUM_ENDPOINTS * 2; i++) {
        u32 bit = 1 << i;
        if (remaining_buffers & bit) {
            hw_clear_bits(&usb_hw->buf_status, bit);

            if (i == 0) {
                if (usb_state.should_set_address) {
                    usb_hw->dev_addr_ctrl = usb_state.dev_address;
                    usb_state.should_set_address = 0;
                    usb_state.state = USB_STATE_ADDRESSED;
                } else {
                    endp0_out.next_data_pid = 1;
                    ep_transfer(&endp0_out, 0x40);
                }
            }

            else if (i == 1) {
                if (sig_buffers_PIO.status != SIG_BUF_NORMAL) {
                    pio_dma_copy_new_signal_samples((u32)endp0_out.data_buffer,
                                     sig_buffers_PIO.buf_copy_addr_offset,
                                     sig_buffers_PIO.buf_copy_len);
                }
                else {
                    ep_transfer(&endp0_out, 0x40);
                }
            }

            else if (i == 3) {   // EP1 OUT
                pico_rw_t *rec = (pico_rw_t *)endp1_out.data_buffer;
                u32 *send_data = (u32 *)endp3_in.data_buffer;

                if (rec->action == PICO_RW_WRITE_ONE) {
                    *(u32 *)(rec->write_addr) = rec->write_value;
                }
                else if (rec->action == PICO_RW_WRITE_MULTIPLE) {
                    for (i32 i = 0; i < rec->num_writes; i++) {
                        ((u32 *)(rec->write_addr_start))[i] = rec->write_values[i];
                    }
                }
                else if (rec->action == PICO_RW_READ_ONE) {
                    *send_data = *(u32 *)(rec->read_addr);
                    ep_transfer(&endp3_in, 4);
                }
                else if (rec->action == PICO_RW_READ_MULTIPLE) {
                    for (i32 i = 0; i < rec->num_reads; i++) {
                        send_data[i] = ((u32 *)(rec->read_addr_start))[i];
                    }
                    ep_transfer(&endp3_in, 4*rec->num_reads);
                }

                ep_transfer(&endp1_out, 64);
                endp1_out.next_data_pid ^= 1;
            }

            else if (i == 4) {   // EP2 IN
                ep_transfer(&endp2_in, 64);
                endp2_in.next_data_pid ^= 1;
            }

            else if (i == 6) {   // EP3 IN
                ep_transfer(&endp3_in, 64);
                endp3_in.next_data_pid ^= 1;
            }

            else if (i == 8) {   // EP4 IN
                endp4_next_transfer();
            }

            else {
            }

            // remaining_buffers &= ~bit;
            remaining_buffers ^= bit;
        }
    }
}


internal void usb_bus_reset(void) {
    usb_state.dev_address = 0;
    usb_state.should_set_address = 0;
    usb_hw->dev_addr_ctrl = 0;
    usb_state.state = USB_STATE_DEFAULT;
    usb_state.configuration = 0;
}


#define USB_SIE_STATUS_SETUP_REC        0x20000
#define USB_SIE_STATUS_BUS_RESET        0x80000

void isr_usbctrl(void) {
    // USB interrupt handler
    // NOTE: Make sure there are no race conditions between the cores 0 and 1.
    // Possible when the same interrupt is enabled on both cores (NVIC).

    uint32_t status = usb_hw->ints;
    uint32_t handled = 0;

    // Setup packet received
    if (status & USB_INTS_SETUP_REQ) {
        handled |= USB_INTS_SETUP_REQ;
        hw_clear_bits(&usb_hw->sie_status, USB_SIE_STATUS_SETUP_REC);
        switch (setup_packet->bmRequestType & (0b11 << 5)) {
            case (0 << 5):
                usb_handle_setup_packet_standard();
                break;
            case (1 << 5):
                usb_handle_setup_packet_class();
                break;
            case (2 << 5):
                usb_handle_setup_packet_vendor();
                break;
            case (3 << 5):
            default: {
                u8 direction_IN = (setup_packet->bmRequestType >> 7) & 1;
                if (direction_IN) {
                    ep_stall(&endp0_in);
                }
                else {
                    ep_stall(&endp0_out);
                }
                DBG_str(&dbg_usb, "ERROR: [Setup packet : bmRequestType] Reserved Type.\n");
                break;
            }
        }
    }

    if (status & USB_INTS_EP_STALL_NAK) {
        handled |= USB_INTS_EP_STALL_NAK;
        usb_handle_stall_nak();
    }

    // Buffer status, one or more buffers have completed
    if (status & USB_INTS_BUFF_STATUS) {
        handled |= USB_INTS_BUFF_STATUS;
        usb_handle_buff_status();
    }

    if (status & USB_INTS_BUS_RESET) {
        handled |= USB_INTS_BUS_RESET;
        hw_clear_bits(&usb_hw->sie_status, USB_SIE_STATUS_BUS_RESET);
        usb_bus_reset();
    }

    if (status ^ handled) {
        // UNHANDLED IRQ
        DBG_str(&dbg_usb, "ERROR: Unhandled USB IRQ: \n");
        DBG_hex(&dbg_usb, status ^ handled);
        DBG_str(&dbg_usb, ".\n");
    }
}


#endif
