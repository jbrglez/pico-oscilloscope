/**
 * Copyright (c) 2024 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _HARDWARE_STRUCTS_USB_DPRAM_H
#define _HARDWARE_STRUCTS_USB_DPRAM_H


#define USB_NUM_ENDPOINTS 16

#ifndef USB_MAX_ENDPOINTS
#define USB_MAX_ENDPOINTS USB_NUM_ENDPOINTS
#endif

#define USB_HOST_INTERRUPT_ENDPOINTS (USB_NUM_ENDPOINTS - 1)

#define USB_BUF_CTRL_FULL      0x00008000u
#define USB_BUF_CTRL_LAST      0x00004000u
#define USB_BUF_CTRL_DATA0_PID 0x00000000u
#define USB_BUF_CTRL_DATA1_PID 0x00002000u
#define USB_BUF_CTRL_SEL       0x00001000u
#define USB_BUF_CTRL_STALL     0x00000800u
#define USB_BUF_CTRL_AVAIL     0x00000400u
#define USB_BUF_CTRL_LEN_MASK  0x000003FFu
#define USB_BUF_CTRL_LEN_LSB   0

#define EP_CTRL_ENABLE_BITS (1u << 31u)
#define EP_CTRL_DOUBLE_BUFFERED_BITS (1u << 30)
#define EP_CTRL_INTERRUPT_PER_BUFFER (1u << 29)
#define EP_CTRL_INTERRUPT_PER_DOUBLE_BUFFER (1u << 28)
#define EP_CTRL_INTERRUPT_ON_NAK (1u << 16)
#define EP_CTRL_INTERRUPT_ON_STALL (1u << 17)
#define EP_CTRL_BUFFER_TYPE_LSB 26u
#define EP_CTRL_HOST_INTERRUPT_INTERVAL_LSB 16u

#define USB_DPRAM_SIZE 4096u

#ifndef USB_DPRAM_MAX
#define USB_DPRAM_MAX USB_DPRAM_SIZE
#endif

#define USB_MAX_ISO_PACKET_SIZE 1023
#define USB_MAX_PACKET_SIZE 64

typedef struct {
    volatile u8 setup_packet[8];

    // Starts at ep1
    struct usb_device_dpram_ep_ctrl {
        io_rw_32 in;
        io_rw_32 out;
    } ep_ctrl[USB_NUM_ENDPOINTS - 1];

    // Starts at ep0
    struct usb_device_dpram_ep_buf_ctrl {
        io_rw_32 in;
        io_rw_32 out;
    } ep_buf_ctrl[USB_NUM_ENDPOINTS];

    // EP0 buffers are fixed. Assumes single buffered mode for EP0
    u8 ep0_buf_a[0x40];
    u8 ep0_buf_b[0x40];

    // Rest of DPRAM can be carved up as needed
    u8 epx_data[USB_DPRAM_MAX - 0x180];
} usb_device_dpram_t;

// static_assert(sizeof(usb_device_dpram_t) == USB_DPRAM_MAX, "");
// static_assert(offsetof(usb_device_dpram_t, epx_data) == 0x180, "");

typedef struct {
    volatile u8 setup_packet[8];

    // Interrupt endpoint control 1 -> 15
    struct usb_host_dpram_ep_ctrl {
        io_rw_32 ctrl;
        io_rw_32 spare;
    } int_ep_ctrl[USB_HOST_INTERRUPT_ENDPOINTS];

    io_rw_32 epx_buf_ctrl;
    io_rw_32 _spare0;

    // Interrupt endpoint buffer control
    struct usb_host_dpram_ep_buf_ctrl {
        io_rw_32 ctrl;
        io_rw_32 spare;
    } int_ep_buffer_ctrl[USB_HOST_INTERRUPT_ENDPOINTS];

    io_rw_32 epx_ctrl;

    u8 _spare1[124];

    // Should start at 0x180
    u8 epx_data[USB_DPRAM_MAX - 0x180];
} usb_host_dpram_t;

// static_assert(sizeof(usb_host_dpram_t) == USB_DPRAM_MAX, "");
// static_assert(offsetof(usb_host_dpram_t, epx_data) == 0x180, "");

#define usb_dpram ((usb_device_dpram_t *)USBCTRL_DPRAM_BASE)
#define usbh_dpram ((usb_host_dpram_t *)USBCTRL_DPRAM_BASE)

// static_assert( USB_HOST_INTERRUPT_ENDPOINTS == 15, "");

#endif // _HARDWARE_STRUCTS_USB_DPRAM_H
