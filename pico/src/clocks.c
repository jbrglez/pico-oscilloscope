#ifndef CLOCKS_C
#define CLOCKS_C

#include "hardware.h"
#include "my_math.c"


internal void xosc_init(void) {
    xosc_hw->ctrl = XOSC_CTRL_FREQ_RANGE_1MHZ_15MHZ;
    xosc_hw->startup = 47 << XOSC_STARTUP_DELAY_LSB; //  wait ~1ms on startup
    hw_set_bits(&xosc_hw->ctrl, XOSC_CTRL_ENABLE);
    while(!(xosc_hw->status & XOSC_STATUS_STABLE));
}


internal void pll_usb_init(void) {
    hw_set_bits(&resets_hw->reset, RESET_PLL_USB);
    hw_clear_bits(&resets_hw->reset, RESET_PLL_USB);
    while(!(resets_hw->reset_done & RESET_PLL_USB));

    // NOTE: Settings for operation at 48 MHz
    pll_usb_hw->cs =  1 << PLL_CS_REFDIV_LSB;
    pll_usb_hw->fbdiv_int = 120;
    hw_clear_bits(&pll_usb_hw->pwr, PLL_PWR_VCOPD | PLL_PWR_PD);
    while (!(pll_usb_hw->cs & PLL_CS_LOCKED));
    pll_usb_hw->prim = (6 << PLL_PRIM_POSTDIV1_LSB) | (5 << PLL_PRIM_POSTDIV2_LSB);
    hw_clear_bits(&pll_usb_hw->pwr, PLL_PWR_POSTDIVPD);
}


internal void pll_sys_init(void) {
    hw_set_bits(&resets_hw->reset, RESET_PLL_SYS);
    hw_clear_bits(&resets_hw->reset, RESET_PLL_SYS);
    while(!(resets_hw->reset_done & RESET_PLL_SYS));

    // NOTE: Settings for operation at 126 MHz
    pll_sys_hw->cs =  1 << PLL_CS_REFDIV_LSB;
    pll_sys_hw->fbdiv_int = 126;
    hw_clear_bits(&pll_sys_hw->pwr, PLL_PWR_VCOPD | PLL_PWR_PD);
    while (!(pll_sys_hw->cs & PLL_CS_LOCKED));
    pll_sys_hw->prim = (6 << PLL_PRIM_POSTDIV1_LSB) | (2 << PLL_PRIM_POSTDIV2_LSB);
    hw_clear_bits(&pll_sys_hw->pwr, PLL_PWR_POSTDIVPD);
}


internal void clocks_init() {
    clocks_hw->resus.ctrl = 0;
    xosc_init();

    hw_clear_bits(&clocks_hw->clk[clk_peri].ctrl, CLOCKS_CLK_ENABLE);
    hw_clear_bits(&clocks_hw->clk[clk_usb].ctrl,  CLOCKS_CLK_ENABLE);
    hw_clear_bits(&clocks_hw->clk[clk_adc].ctrl,  CLOCKS_CLK_ENABLE);

    hw_write_masked(&clocks_hw->clk[clk_ref].ctrl, CLOCKS_CLK_REF_CTRL_SRC_XOSC, CLOCKS_CLK_REF_CTRL_SRC_BITS);
    while (!(clocks_hw->clk[clk_ref].selected & CLOCKS_CLK_REF_SELECTED_CLK_XOSC));
    hw_write_masked(&clocks_hw->clk[clk_sys].ctrl, CLOCKS_CLK_SYS_CTRL_SRC_REF, CLOCKS_CLK_SYS_CTRL_SRC_BITS);
    while (!(clocks_hw->clk[clk_sys].selected & CLOCKS_CLK_SYS_SELECTED_CLK_REF));

    pll_sys_init();
    pll_usb_init();

    hw_write_masked(&clocks_hw->clk[clk_sys].ctrl, CLOCKS_CLK_SYS_CTRL_AUXSRC_PLL_SYS, CLOCKS_CLK_CTRL_AUXSRC_BITS);
    hw_write_masked(&clocks_hw->clk[clk_sys].ctrl, CLOCKS_CLK_SYS_CTRL_SRC_AUX, CLOCKS_CLK_SYS_CTRL_SRC_BITS);
    while (!(clocks_hw->clk[clk_sys].selected & CLOCKS_CLK_SYS_SELECTED_CLK_AUX));

    clocks_hw->clk[clk_peri].ctrl = CLOCKS_CLK_ENABLE | CLOCKS_CLK_PERI_CTRL_AUXSRC_PLL_USB;
    clocks_hw->clk[clk_usb].ctrl  = CLOCKS_CLK_ENABLE | CLOCKS_CLK_USB_CTRL_AUXSRC_PLL_USB;
    clocks_hw->clk[clk_adc].ctrl  = CLOCKS_CLK_ENABLE | CLOCKS_CLK_ADC_CTRL_AUXSRC_PLL_USB;

    // Enable tick generation every 1us for timer (clk_ref: 12MHz)
    watchdog_hw->tick = (1<<9) | 12;
}


internal u64 time_us(void) {
    u32 hi_prev = timer_hw->timerawh;
    u32 low = timer_hw->timerawl;
    u32 high = timer_hw->timerawh;
    if (high != hi_prev) {
        low = timer_hw->timerawl;
    }
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


#endif
