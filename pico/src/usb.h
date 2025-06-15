#ifndef USB_H
#define USB_H

#include "my_types.h"

typedef volatile u8 v_u8;
typedef volatile u16 v_u16;


typedef enum {
    USB_STATE_DEFAULT,
    USB_STATE_ADDRESSED,
    USB_STATE_CONFIGURED,
} USB_state;


typedef enum {
    GET_STATUS = 0,
    CLEAR_FEATURE = 1,
    SET_FEATURE = 3,
    SET_ADDRESS = 5,
    GET_DESCRIPTOR = 6,
    SET_DESCRIPTOR = 7,
    GET_CONFIGURATION = 8,
    SET_CONFIGURATION = 9,
    GET_INTERFACE = 10,
    SET_INTERFACE = 11,
    SYNC_FRAME = 12,
} USB_Request_Type;


typedef enum {
    DESCRIPTOR_DEVICE = 1,
    DESCRIPTOR_CONFIGURATION = 2,
    DESCRIPTOR_STRING = 3,
    DESCRIPTOR_INTERFACE = 4,
    DESCRIPTOR_ENDPOINT = 5,
    DESCRIPTOR_DEVICE_QUALIFIER = 6,
    DESCRIPTOR_OTHER_SPEED_CONFIGURATION = 7,
    DESCRIPTOR_INTERFACE_POWER = 8,
} USB_Descriptor_Type;


typedef enum {
    ENDPOINT_HALT = 0,
    DEVICE_REMOTE_WAKEUP = 1,
    TEST_MODE = 2,
} USB_Feature_Selector;


typedef enum {
    USB_TRANSFER_CONTROL = 0,
    USB_TRANSFER_ISO = 1,
    USB_TRANSFER_BULK = 2,
    USB_TRANSFER_INTERRUPT = 3,
} USB_Transfer_Type;


typedef enum {
    USB_REQUEST_RECIPIENT_DEVICE = 0,
    USB_REQUEST_RECIPIENT_INTERFACE = 1,
    USB_REQUEST_RECIPIENT_ENDPOINT = 2,
} USB_Request_Recipient;


typedef struct __attribute__((packed)) {
    u8  bmRequestType;
    u8  bRequest;
    u16 wValue;
    u16 wIndex;
    u16 wLength;
} usb_setup_packet;


typedef struct __attribute__((packed)) {
    u8 bLength;
    u8 bDescriptorType;
} usb_descriptor;


typedef struct __attribute__((packed)) {
    u8  bLength;
    u8  bDescriptorType;
    u16 bcdUSB;
    u8  bDeviceClass;
    u8  bDeviceSubClass;
    u8  bDeviceProtocol;
    u8  bMaxPacketSize0;
    u16 idVendor;
    u16 idProduct;
    u16 bcdDevice;
    u8  iManufacturer;
    u8  iProduct;
    u8  iSerialNumber;
    u8  bNumConfigurations;
} usb_device_descriptor;


typedef struct __attribute__((packed)) {
    u8  bLength;
    u8  bDescriptorType;
    u16 wTotalLength;
    u8  bNumInterfaces;
    u8  bConfigurationValue;
    u8  iConfiguration;
    u8  bmAttributes;
    u8  bMaxPower;
} usb_configuration_descriptor;


typedef struct __attribute__((packed)) {
    u8 bLength;
    u8 bDescriptorType;
    u8 bInterfaceNumber;
    u8 bAlternateSetting;
    u8 bNumEndpoints;
    u8 bInterfaceClass;
    u8 bInterfaceSubClass;
    u8 bInterfaceProtocol;
    u8 iInterface;
} usb_interface_descriptor;


typedef struct __attribute__((packed)) {
    u8  bLength;
    u8  bDescriptorType;
    u8  bEndpointAddress;
    u8  bmAttributes;
    u16 wMaxPacketSize;
    u8  bInterval;
} usb_endpoint_descriptor;


typedef struct __attribute__((packed)) {
    u8  bLength;
    u8  bDescriptorType;
    u8  bEndpointAddress;
    u8  bmAttributes;
    u16 wMaxPacketSize;
    u8  bInterval;
    u8  bRefresh;
    u8  bSyncAddr;
} usb_endpoint_descriptor_long;


// -----------------------------------------------

typedef struct {
    volatile u8  *data_buffer;
    volatile u32 *buf_ctrl;
    const usb_endpoint_descriptor *descriptor;
    union {
        struct { u8 id : 7, dir : 1; };
        u8 addr;
    };
    u8 next_data_pid;
} endp_t;


internal void ep_transfer(endp_t *ep, u32 len);


#endif
