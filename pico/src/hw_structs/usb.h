/**
 * Copyright (c) 2024 Raspberry Pi Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _HARDWARE_STRUCTS_USB_H
#define _HARDWARE_STRUCTS_USB_H



#define USB_MAIN_CTRL_MODE_HOST   (1u << 1u)
#define USB_MAIN_CTRL_MODE_DEVICE (0u << 1u)
#define USB_MAIN_CTRL_MODE_CONTROLLER_EN (1u << 0u)

#define USB_SIE_CTRL_EP0_INT_STALL (1u << 31u)
#define USB_SIE_CTRL_EP0_INT_1BUF  (1u << 29u)
#define USB_SIE_CTRL_PULLUP_EN     (1u << 16u)

#define USB_EP_STALL_ARM_EP0_IN  (1u << 0u)
#define USB_EP_STALL_ARM_EP0_OUT (1u << 1u)

#define USB_MUXING_SOFTCON (1u << 3u)
#define USB_MUXING_TO_PHY  (1u << 0u)

#define USB_PWR_VBUS_DETECT_OVERRIDE_EN (1u << 3u)
#define USB_PWR_VBUS_DETECT (1u << 2u)


typedef struct {
    io_rw_32 dev_addr_ctrl;
    io_rw_32 int_ep_addr_ctrl[15];
    io_rw_32 main_ctrl;
    io_wo_32 sof_wr;
    io_ro_32 sof_rd;
    io_rw_32 sie_ctrl;
    io_rw_32 sie_status;
    io_rw_32 int_ep_ctrl;
    io_rw_32 buf_status;
    io_ro_32 buf_cpu_should_handle;
    io_rw_32 abort;
    io_rw_32 abort_done;
    io_rw_32 ep_stall_arm;
    io_rw_32 nak_poll;
    io_rw_32 ep_nak_stall_status;
    io_rw_32 muxing;
    io_rw_32 pwr;
    io_rw_32 phy_direct;
    io_rw_32 phy_direct_override;
    io_rw_32 phy_trim;
    uint32_t _pad0;
    io_ro_32 intr;
    io_rw_32 inte;
    io_rw_32 intf;
    io_ro_32 ints;
} usb_hw_t;

#define usb_hw ((usb_hw_t *)USBCTRL_REGS_BASE)
// static_assert(sizeof (usb_hw_t) == 0x009c, "");

#endif // _HARDWARE_STRUCTS_USB_H
