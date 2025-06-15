#ifndef USB_EP_H
#define USB_EP_H

#include "hardware.h"
#include "usb.h"


static const usb_endpoint_descriptor ep0_out = {
        .bLength          = sizeof(usb_endpoint_descriptor),
        .bDescriptorType  = DESCRIPTOR_ENDPOINT,
        .bEndpointAddress = 0x00,
        .bmAttributes     = USB_TRANSFER_CONTROL,
        .wMaxPacketSize   = 64,
        .bInterval        = 0
};

static const usb_endpoint_descriptor ep0_in = {
        .bLength          = sizeof(usb_endpoint_descriptor),
        .bDescriptorType  = DESCRIPTOR_ENDPOINT,
        .bEndpointAddress = 0x80,
        .bmAttributes     = USB_TRANSFER_CONTROL,
        .wMaxPacketSize   = 64,
        .bInterval        = 0
};

static const usb_device_descriptor device_descriptor = {
        .bLength         = sizeof(usb_device_descriptor),
        .bDescriptorType = DESCRIPTOR_DEVICE,
        .bcdUSB          = 0x0110, // USB 1.1 device
        .bDeviceClass    = 0,      // Specified in interface descriptor
        .bDeviceSubClass = 0,      // No subclass
        .bDeviceProtocol = 0,      // No protocol
        .bMaxPacketSize0 = 64,     // Max packet size for ep0
        .idVendor        = 0x0000,
        .idProduct       = 0x0001,
        .bcdDevice       = 0,      // No device revision number
        .iManufacturer   = 1,      // Manufacturer string index
        .iProduct        = 2,      // Product string index
        .iSerialNumber   = 0,      // No serial number
        .bNumConfigurations = 1
};

static const usb_interface_descriptor interface_descriptor = {
        .bLength            = sizeof(usb_interface_descriptor),
        .bDescriptorType    = DESCRIPTOR_INTERFACE,
        .bInterfaceNumber   = 0,
        .bAlternateSetting  = 0,
        .bNumEndpoints      = 4,
        .bInterfaceClass    = 0xff, // Vendor specific endpoint
        .bInterfaceSubClass = 0,
        .bInterfaceProtocol = 0,
        .iInterface         = 0
};


static const usb_endpoint_descriptor ep1_out = {
        .bLength          = sizeof(usb_endpoint_descriptor),
        .bDescriptorType  = DESCRIPTOR_ENDPOINT,
        .bEndpointAddress = 0x01,
        // .bmAttributes     = USB_TRANSFER_INTERRUPT,
        .bmAttributes     = USB_TRANSFER_BULK,
        .wMaxPacketSize   = 64,
        .bInterval        = 0
};

static const usb_endpoint_descriptor ep2_in = {
        .bLength          = sizeof(usb_endpoint_descriptor),
        .bDescriptorType  = DESCRIPTOR_ENDPOINT,
        .bEndpointAddress = 0x82,
        // .bmAttributes     = USB_TRANSFER_ISO,
        .bmAttributes     = USB_TRANSFER_BULK,
        .wMaxPacketSize   = 512,
        .bInterval        = 0
};

static const usb_endpoint_descriptor ep3_in = {
        .bLength          = sizeof(usb_endpoint_descriptor),
        .bDescriptorType  = DESCRIPTOR_ENDPOINT,
        .bEndpointAddress = 0x83,
        .bmAttributes     = USB_TRANSFER_BULK,
        .wMaxPacketSize   = 64,
        .bInterval        = 0
};

static const usb_endpoint_descriptor ep4_in = {
        .bLength          = sizeof(usb_endpoint_descriptor),
        .bDescriptorType  = DESCRIPTOR_ENDPOINT,
        .bEndpointAddress = 0x84,
        .bmAttributes     = (1<<2) | USB_TRANSFER_ISO,
        // .wMaxPacketSize   = (1<<11) | 960,
        .wMaxPacketSize   = 960,
        .bInterval        = 1
};

static const usb_configuration_descriptor config_descriptor = {
        .bLength         = sizeof(usb_configuration_descriptor),
        .bDescriptorType = DESCRIPTOR_CONFIGURATION,
        .wTotalLength    = (sizeof(config_descriptor) +
                            sizeof(interface_descriptor) +
                            sizeof(ep1_out) +
                            sizeof(ep2_in) +
                            sizeof(ep3_in) +
                            sizeof(ep4_in)),
        .bNumInterfaces  = 1,
        .bConfigurationValue = 1,
        .iConfiguration = 0,      // No string
        // .bmAttributes = 0xc0,  // attributes: self powered, no remote wakeup
        .bmAttributes = 0x80,     // attributes: bus powered, no remote wakeup
        .bMaxPower = 500 / 2,     // 500ma
};


static endp_t endp0_in  = {
        .id = 0, .dir = 1, .next_data_pid = 0,
        .descriptor = &ep0_in,
        .buf_ctrl = &(usb_dpram->ep_buf_ctrl[0].in),
        .data_buffer = &(usb_dpram->ep0_buf_a[0])
};

static endp_t endp0_out = {
        .id = 0, .dir = 0, .next_data_pid = 0,
        .descriptor = &ep0_out,
        .buf_ctrl = &(usb_dpram->ep_buf_ctrl[0].out),
        .data_buffer = &(usb_dpram->ep0_buf_a[0])
};

static endp_t endp1_out = {
        .id = 1, .dir = 0, .next_data_pid = 0,
        .descriptor = &ep1_out,
        .buf_ctrl = &(usb_dpram->ep_buf_ctrl[1].out),
        .data_buffer = &(usb_dpram->epx_data[0x0])
};

static endp_t endp2_in  = {
        .id = 2, .dir = 1, .next_data_pid = 0,
        .descriptor = &ep2_in,
        .buf_ctrl = &(usb_dpram->ep_buf_ctrl[2].in),
        .data_buffer = (volatile u8 *)((uintptr_t)usb_dpram + 0x200)
};

static endp_t endp3_in  = {
        .id = 3, .dir = 1, .next_data_pid = 0,
        .descriptor = &ep3_in,
        .buf_ctrl = &(usb_dpram->ep_buf_ctrl[3].in),
        .data_buffer = (volatile u8 *)((uintptr_t)usb_dpram + 0x400)
};

static endp_t endp4_in  = {
        .id = 4, .dir = 1, .next_data_pid = 0,
        .descriptor = &ep4_in,
        .buf_ctrl = &(usb_dpram->ep_buf_ctrl[4].in),
        .data_buffer = (volatile u8 *)((uintptr_t)usb_dpram + 0x800)
};


#endif
