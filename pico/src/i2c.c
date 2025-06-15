#ifndef I2C_C
#define I2C_C

#include "hardware.h"
#include "uart.c"


#define I2C_restart (1<<10)
#define I2C_stop (1<<9)
#define I2C_write (0<<8)
#define I2C_read  (1<<8)


internal void i2c_init(void) {
    hw_set_bits(&resets_hw->reset, RESET_I2C0);
    hw_clear_bits(&resets_hw->reset, RESET_I2C0);
    while(!(resets_hw->reset_done & RESET_I2C0));

    i2c0_hw->enable = 0;

    #define I2C_SPEED_FAST 2
    #define I2C_SPEED_STANDARD 1
    hw_write_masked(&i2c0_hw->con, I2C_SPEED_STANDARD << 1, 0x3 << 1);

    i2c0_hw->tx_tl = 0;
    i2c0_hw->rx_tl = 0;

    // i2c0_hw->dma_cr = (1 << 1) | (1 << 0);

    #define CYCLES_1us  126
    #define I2C_HCNT    (5 * CYCLES_1us)
    #define I2C_LCNT    (6 * CYCLES_1us)
    u32 hcnt = 5 * CYCLES_1us;
    u32 lcnt = 6 * CYCLES_1us;
    u32 hold = 1 * CYCLES_1us;
    u32 spklen = (lcnt < 16) ? 1 : lcnt / 16;

    i2c0_hw->ss_scl_hcnt = hcnt;
    i2c0_hw->ss_scl_lcnt = lcnt;
    i2c0_hw->fs_scl_hcnt = hcnt;
    i2c0_hw->fs_scl_lcnt = lcnt;
    i2c0_hw->fs_spklen   = spklen;
    hw_write_masked(&i2c0_hw->sda_hold, hold << 0, 0xFFFF << 0);

    io_bank0_hw->io[20].ctrl = GPIO_FUNC_I2C;
    io_bank0_hw->io[21].ctrl = GPIO_FUNC_I2C;
    pads_bank0_hw->io[20] = (1<<6) | (1<<4) | (1<<3) | (1<<1) | (0<<0);
    pads_bank0_hw->io[21] = (1<<6) | (1<<4) | (1<<3) | (1<<1) | (0<<0);

    i2c0_hw->enable = 1;
}


i8 i2c_sensor_temp = 0;

internal void i2c_config_temp_sensor(void){
    // Microchip TC74A0-3.3VAT Tiny Serial Digital Thermal Sensor
    // (https://ww1.microchip.com/downloads/en/DeviceDoc/21462D.pdf)
    i2c0_hw->enable = 0;
    i2c0_hw->tar = 0x48;
    i2c0_hw->enable = 1;

    // Configure it for temperature reading.

    // Set normal mode
    i2c0_hw->data_cmd = I2C_write | 1;
    i2c0_hw->data_cmd = I2C_write | 0 | I2C_stop;

    // Set read T
    i2c0_hw->data_cmd = I2C_write | 0 | I2C_stop;
}


internal u8 i2c_read_byte_blocking(void) {
    i2c0_hw->data_cmd = I2C_read  | I2C_stop;
    while(!(i2c0_hw->status & (1<<3)));
    return (u8)(i2c0_hw->data_cmd & 0xFF);
}


internal i32 i2c_try_read_byte_blocking(void) {
    i2c0_hw->data_cmd = I2C_read  | I2C_stop;

    u32 abort = 0;
    do {
        abort = i2c0_hw->clr_tx_abrt;
    } while (!abort && !(i2c0_hw->status & (1<<3)));

    if (abort)
        return -1;

    return (i32)(i2c0_hw->data_cmd & 0xFF);
}


internal i32 reserved_addr(u8 addr) {
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}


#ifdef CONFIG_USE_UART

internal void bus_scan() {
    uart_puts("\nI2C Bus Scan\n");
    uart_puts("     0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

    for (i32 addr = 0; addr < (1 << 7); ++addr) {
        if (addr % 16 == 0) {
            uart_hex_8(addr);
            uart_puts(" ");
        }

        // Skip over any reserved addresses.
        i32 ret;
        if (reserved_addr(addr)) {
            ret = -1;
        }
        else {
            i2c0_hw->enable = 0;
            i2c0_hw->tar = addr;
            i2c0_hw->enable = 1;
            ret = i2c_try_read_byte_blocking();
        }

        uart_puts(ret < 0 ? "." : "@");
        uart_puts(addr % 16 == 15 ? "\n" : "  ");
    }
    uart_puts("Bus scan done.\n\n");
}

#else

#define bus_scan()

#endif


#endif
