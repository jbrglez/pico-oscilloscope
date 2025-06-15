#ifndef CLOCKS_C
#define CLOCKS_C

#include "hardware.h"
#include "my_math.c"


internal void xosc_init(void) {
    xosc_hw->ctrl = 0xAA0;
    xosc_hw->startup = 47; //  wait ~1ms on startup
    hw_set_bits(&xosc_hw->ctrl, 0xFAB000); // enable
    while(!(xosc_hw->status & (1<<31)));
}


internal void pll_usb_init(void) {
    // NOTE:  48 MHz
    hw_set_bits(&resets_hw->reset, RESET_PLL_USB);
    hw_clear_bits(&resets_hw->reset, RESET_PLL_USB);
    while(!(resets_hw->reset_done & RESET_PLL_USB));

    pll_usb_hw->cs =  1;
    pll_usb_hw->fbdiv_int = 120;
    hw_clear_bits(&pll_usb_hw->pwr, 0x21);
    while (!(pll_usb_hw->cs & (1<<31)));
    pll_usb_hw->prim = (6 << 16) | (5 << 12);
    hw_clear_bits(&pll_usb_hw->pwr, 0x8);
}


internal void pll_sys_init(void) {
    // NOTE:  126 MHz
    hw_set_bits(&resets_hw->reset, RESET_PLL_SYS);
    hw_clear_bits(&resets_hw->reset, RESET_PLL_SYS);
    while(!(resets_hw->reset_done & RESET_PLL_SYS));

    pll_sys_hw->cs =  1;
    pll_sys_hw->fbdiv_int = 126;
    hw_clear_bits(&pll_sys_hw->pwr, 0x21);
    while (!(pll_sys_hw->cs & (1<<31)));
    pll_sys_hw->prim = (6 << 16) | (2 << 12);
    hw_clear_bits(&pll_sys_hw->pwr, 0x8);
}


internal void clocks_init() {
    clocks_hw->resus.ctrl = 0;
    xosc_init();

    hw_clear_bits(&clocks_hw->clk[clk_peri].ctrl, (1<<11));
    hw_clear_bits(&clocks_hw->clk[clk_usb].ctrl,  (1<<11));
    hw_clear_bits(&clocks_hw->clk[clk_adc].ctrl,  (1<<11));

    hw_write_masked(&clocks_hw->clk[clk_ref].ctrl, 2, 0b11);
    while (!(clocks_hw->clk[clk_ref].selected & (1<<2)));
    hw_clear_bits(&clocks_hw->clk[clk_sys].ctrl, 1);
    while (!(clocks_hw->clk[clk_sys].selected & (1<<0)));

    pll_sys_init();
    pll_usb_init();

    hw_clear_bits(&clocks_hw->clk[clk_sys].ctrl, (0xF<<5));
    hw_set_bits(&clocks_hw->clk[clk_sys].ctrl, 1);
    while (!(clocks_hw->clk[clk_sys].selected & (1<<1)));

    clocks_hw->clk[clk_peri].ctrl = (1<<11) | (2<<5);
    clocks_hw->clk[clk_usb].ctrl  = (1<<11) | (0<<5);
    clocks_hw->clk[clk_adc].ctrl  = (1<<11) | (0<<5);

    // Enable tick generation every 1us for timer (clk_ref: 12MHz)
    watchdog_hw->tick = (1<<9) | 12;
}


// NOTE: Predictable/Safe when only one core can call at a time.
internal u64 time_us(void) {
    u32 low = timer_hw->timelr;
    u32 high = timer_hw->timehr;
    return ((u64)high << 32) | low;
}


internal void busy_wait_us(u64 wait_us) {
    u64 start = time_us();
    u64 end = start + wait_us;
    while (time_us() < end);
}


internal u64 time_s(void) {
    return divide_u64(time_us(), 1000000, 0);
}


internal u64 time_safe_us(void) {
    u32 hi_prev = timer_hw->timerawh;
    u32 low = timer_hw->timerawl;
    u32 high = timer_hw->timerawh;
    if (high != hi_prev) {
        low = timer_hw->timerawl;
    }
    return ((u64)high << 32) | low;
}


internal void busy_wait_safe_us(u64 wait_us) {
    u64 start = time_safe_us();
    u64 end = start + wait_us;
    while (time_safe_us() < end);
}


#endif
