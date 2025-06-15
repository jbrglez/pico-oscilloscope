#ifndef PIO_INSTRUCTIONS_C
#define PIO_INSTRUCTIONS_C

#include <stdlib.h>
#include <stdio.h>

#include "my_types.h"


typedef enum {
    OPCODE_JMP = 0,
    OPCODE_WAIT = 1,
    OPCODE_IN = 2,
    OPCODE_OUT = 3,
    OPCODE_PUSH = 4,
    OPCODE_PULL = 4,
    OPCODE_MOV = 5,
    OPCODE_IRQ = 6,
    OPCODE_SET = 7,
} pio_opcode_num;


typedef enum {
    DIRECT_DEFINE,
    DIRECT_ORIGIN,
    DIRECT_SET_SIDE,
    DIRECT_WRAP_TARGET,
    DIRECT_WRAP,
    DIRECT_WORD,
} pio_directive_num;


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
    pio_op_t *items;
    i32 capacity;
    i32 length;
} pio_op_array_t;


void print_pio_program(pio_op_array_t program) {
    for (i32 i = 0; i < program.length; i++) {
        printf("0x%04X\n", program.items[i].hex);
    }
}


pio_op_array_t get_new_pio_array(u32 cap) {
    cap = 64 * ( (cap/64) + 1);

    pio_op_array_t new = {
        .length = 0,
        .capacity = cap,
    };
    new.items = calloc(new.capacity, sizeof(pio_op_t));

    if (new.items == NULL) {
        new.capacity = 0;
        fprintf(stderr, "ERROR: new PIO array allocation failed");
    }

    return new;
}

i32 append_op_to_pio_array(pio_op_array_t *array, pio_op_t operation) {
    if (array->length >= array->capacity) {
        void *new_loc = reallocarray(array->items, array->capacity + 64, sizeof(pio_op_t));
        if (new_loc == NULL) {
            fprintf(stderr, "ERROR: Failed to append to PIO array");
            return -1;
        }
        else {
            array->items = new_loc;
            array->capacity += 64;
        }
    }
    array->items[array->length] = operation;
    array->length++;
    return 0;
}


i32 free_pio_array(pio_op_array_t *array) {
    array->length = 0;
    array->capacity = 0;
    free(array->items);
    array->items = 0;

    return 0;
}


pio_op_t encode_jmp(u8 delay_side, u8 cond, u8 addr) {
    return (pio_op_t){
        .opcode = OPCODE_JMP,
        .delay_side = delay_side,
        .jmp = {
            .cond = cond,
            .addr = addr,
        },
    };
}


pio_op_t encode_wait(u8 delay_side, u8 pol, u8 src, u8 idx) {
    return (pio_op_t){
        .opcode = OPCODE_WAIT,
        .delay_side = delay_side,
        .wait = {
            .pol = pol,
            .src = src,
            .idx = idx,
        },
    };
}


pio_op_t encode_in(u8 delay_side, u8 src, u8 count) {
    return (pio_op_t){
        .opcode = OPCODE_IN,
        .delay_side = delay_side,
        .in = {
            .src = src,
            .count = count,
        },
    };
}


pio_op_t encode_out(u8 delay_side, u8 dest, u8 count) {
    return (pio_op_t){
        .opcode = OPCODE_OUT,
        .delay_side = delay_side,
        .out = {
            .dest = dest,
            .count = count,
        },
    };
}


pio_op_t encode_push(u8 delay_side, u8 iff, u8 blk) {
    return (pio_op_t){
        .opcode = OPCODE_PUSH,
        .delay_side = delay_side,
        .push = {
            .zero = 0,
            .iff = iff,
            .blk = blk,
            .zeros = 0,
        },
    };
}


pio_op_t encode_pull(u8 delay_side, u8 ife, u8 blk) {
    return (pio_op_t){
        .opcode = OPCODE_PULL,
        .delay_side = delay_side,
        .pull = {
            .one = 1,
            .ife = ife,
            .blk = blk,
            .zeros = 0,
        },
    };
}


pio_op_t encode_mov(u8 delay_side, u8 dest, u8 op, u8 src) {
    return (pio_op_t){
        .opcode = OPCODE_MOV,
        .delay_side = delay_side,
        .mov = {
            .dest = dest,
            .op = op,
            .src = src,
        },
    };
}


pio_op_t encode_irq(u8 delay_side, u8 clr, u8 wait, u8 idx) {
    return (pio_op_t){
        .opcode = OPCODE_IRQ,
        .delay_side = delay_side,
        .irq = {
            .zero = 0,
            .clr = clr,
            .wait = wait,
            .idx = idx,
        },
    };
}


pio_op_t encode_set(u8 delay_side, u8 dest, u8 data) {
    return (pio_op_t){
        .opcode = OPCODE_SET,
        .delay_side = delay_side,
        .set = {
            .dest = dest,
            .data = data,
        },
    };
}

#endif
