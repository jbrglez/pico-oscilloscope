#ifndef PIO_PROGS_C
#define PIO_PROGS_C

#include "hardware.h"
#include "config.h"
#include "signal_buffers.c"
#include "usb.h"


typedef union {
    struct {
        union {
            struct jmp  { u8  addr : 5, cond : 3; } jmp;
            struct out  { u8 count : 5, dest : 3; } out;
            struct set  { u8  data : 5, dest : 3; } set;
            struct in   { u8 count : 5,  src : 3; } in;
            struct mov  { u8   src : 3,   op : 2, dest : 3; } mov;
            struct wait { u8   idx : 5,  src : 2,  pol : 1; } wait;
            struct irq  { u8   idx : 5, wait : 1,  clr : 1, zero : 1; } irq;
            struct push { u8 zeros : 5,  blk : 1,  iff : 1, zero : 1; } push;
            struct pull { u8 zeros : 5,  blk : 1,  ife : 1,  one : 1; } pull;
        };
        u8 delay_side : 5;
        u8 opcode : 3;
    };
    u16 hex;
} pio_op_t;


typedef struct {
    i8 origin;
    i8 side_set;
    i8 side_set_opt;
    i8 side_set_pindirs;
    i8 length;
    i8 wrap_target;
    i8 wrap;
} pio_settings_t;


typedef struct {
    pio_settings_t settings;
    pio_op_t *instructions_hex;
} pio_program_t;


// NOTE: Is this too many parameters for one function? FIX IT?
internal void load_pio_program(pio_program_t *program, pio_hw_t *pio, u8 sm_num,
                      u8 out_base, u8 out_count, u8 set_base, u8 set_count,
                      u8 side_set_base, u8 in_base,
                      u16 clkdiv_int, u8 clkdiv_frac) {
    for (i32 i = 0; i < program->settings.length; i++) {
        pio->instr_mem[program->settings.origin + i] = program->instructions_hex[i].hex;
    }

    pio->sm[sm_num].pinctrl =
        (out_base << 0)  | (out_count << 20) |      // OUT
        (set_base << 5)  | (set_count << 26) |      // SET
        (side_set_base << 10) | (program->settings.side_set << 29) |     // SIDE_SET
        (in_base  << 15);                 // IN

    pio->sm[sm_num].clkdiv = (clkdiv_int << 16) | (clkdiv_frac << 8);

    pio->sm[sm_num].execctrl =
        (program->settings.side_set_opt << 30) |        // NOTE: Check if this is correct.
        (program->settings.side_set_pindirs << 29) |
        (program->settings.wrap << 12) |
        (program->settings.wrap_target << 7);
}


internal void init_reset_PIO(void) {
    hw_set_bits(&resets_hw->reset, RESET_PIO0);
    hw_clear_bits(&resets_hw->reset, RESET_PIO0);
    while (!(resets_hw->reset_done & RESET_PIO0));
}


internal void pio_mask_begin_sm_exec(pio_hw_t *pio, u8 sm_num) {
    hw_set_bits(&pio->ctrl, (sm_num<<8) | (sm_num<<4) | (sm_num<<0));
}


internal void init_pio_out_pin(pio_hw_t *pio, u8 sm_num, u8 n) {
    // hw_write_masked(&pads_bank0_hw->io[n], (1<<6)| (2<<4), (1<<7) | (1<<6) | (0b11<<4));
    hw_write_masked(&pads_bank0_hw->io[n], (1<<6), (1<<7) | (1<<6));
    io_bank0_hw->io[n].ctrl = (pio == pio0_hw) ? GPIO_FUNC_PIO0 : GPIO_FUNC_PIO1;

    u32 saved_execctrl = pio->sm[sm_num].execctrl;
    u32 saved_pinctrl  = pio->sm[sm_num].pinctrl;

    hw_clear_bits(&pio->sm[sm_num].execctrl, (1<<17));
    pio->sm[sm_num].pinctrl = (1<<26) | (n<<5);

    // pio->sm[sm_num].instr = 0xE081;
    pio->sm[sm_num].instr = 0xE09F;

    pio->sm[sm_num].execctrl = saved_execctrl;
    pio->sm[sm_num].pinctrl  = saved_pinctrl;
}


internal void init_pio_in_pin(pio_hw_t *pio, u8 sm_num, u8 n) {
    hw_write_masked(&pads_bank0_hw->io[n], (1<<6), (1<<7) | (1<<6));
    io_bank0_hw->io[n].ctrl = (pio == pio0_hw) ? GPIO_FUNC_PIO0 : GPIO_FUNC_PIO1;

    u32 saved_execctrl = pio->sm[sm_num].execctrl;
    u32 saved_pinctrl  = pio->sm[sm_num].pinctrl;

    hw_clear_bits(&pio->sm[sm_num].execctrl, (1<<17));
    pio->sm[sm_num].pinctrl = (1<<26) | (n<<5);

    pio->sm[sm_num].instr = 0xE080;

    pio->sm[sm_num].execctrl = saved_execctrl;
    pio->sm[sm_num].pinctrl  = saved_pinctrl;
}


internal void pio_dma_configure_ch(i32 ch) {
    dma_hw->ch[ch].transfer_count = (u32)sig_buffers_PIO.active_buf->length;
    dma_hw->ch[ch].read_addr = (u32)(sig_buffers_PIO.active_buf->data);
}


internal void init_signal_gen_program(pio_hw_t *pio, u8 sm_num) {

    for (i32 i = 0; i < sizeof(signal_data_1)/sizeof(signal_data_1[0]); i++) {
        signal_data_1[i] = i / 16;
    }
    sig_piousb_1.length = sizeof(signal_data_1)/sizeof(signal_data_1[0]);

    for (i32 i = 0; i < sizeof(signal_data_2)/sizeof(signal_data_2[0]); i++) {
        signal_data_2[i] = i / 32;
    }
    sig_piousb_2.length = sizeof(signal_data_2)/sizeof(signal_data_2[0]);


    // Put a number into TX-FIFO and load it into X register.
    pio0_hw->txf[0] = 0x80 - 3;
    pio->sm[sm_num].instr = 0x80A0; // pull
    pio->sm[sm_num].instr = 0x6020; // out X 32

    #define PIO_SHIFTCTRL_FJOIN_TX      30
    #define PIO_SHIFTCTRL_PULL_THRESH   25
    #define PIO_SHIFTCTRL_OUT_SHIFTDIR  19
    #define PIO_SHIFTCTRL_AUTOPULL      17
    pio->sm[sm_num].shiftctrl = (1<<30) | (PIO_NUM_OUT_PINS<<25) | (1<<19) | (1<<17);
    pio_mask_begin_sm_exec(pio0_hw, 0b0001);
}


#include "generated/pio_programs.h"


#endif
