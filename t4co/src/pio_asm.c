#include <stdio.h>

#include "pio_instructions.c"
#include "my_file.c"
#include "my_strings.c"
#include "arena.c"

#include "error_print.c"



typedef struct {
    slice_t name;
    i8 val;
} pio_label_t;


typedef struct {
    pio_label_t *items;
    u32 capacity;
    u32 length;
} pio_label_array_t;


pio_label_array_t get_new_pio_label_array(void) {

    pio_label_array_t new = {
        .length = 0,
        .capacity = 64,
    };
    new.items = calloc(new.capacity, sizeof(pio_label_t));

    if (new.items == NULL) {
        new.capacity = 0;
        fprintf(stderr, "ERROR: new PIO LABEL array allocation failed");
    }

    return new;
}


i32 append_to_pio_label_array(pio_label_array_t *array, pio_label_t label) {
    if (array->length >= array->capacity) {
        void *new_loc = reallocarray(array->items, array->capacity + 64, sizeof(pio_label_t));
        if (new_loc == NULL) {
            fprintf(stderr, "ERROR: Failed to append to PIO array");
            return -1;
        }
        else {
            array->items = new_loc;
            array->capacity += 64;
        }
    }

    array->items[array->length] = label;
    array->length += 1;
    return 0;
}


typedef struct {
    u8 origin;
    u8 side_set;
    u8 side_set_opt;
    u8 side_set_pindirs;
    u8 length;
    u8 wrap_target;
    u8 wrap;
} pio_settings_t;


typedef struct {
    // char *name;
    slice_t name;
    pio_settings_t settings;
    pio_op_array_t operations;
} pio_program_t;


typedef struct {
    slice_t name;
    pio_settings_t settings;
    pio_label_array_t labels;
} parse_prog_t;


b32 get_label_value_from_parse_prog(parse_prog_t *parse_prog, slice_t label_name, u8 *result) {
    for (u32 i = 0; i < parse_prog->labels.length; i++) {
        if (are_slices_equal(parse_prog->labels.items[i].name, label_name)) {
            *result = parse_prog->labels.items[i].val;
            return TRUE;
        }
    }
    return FALSE;
}


void write_program_to_file(pio_program_t program, char *FileName) {
    arena_t arena = get_new_arena(1000);

    write_str_to_arena(&arena, "pio_op_t ");
    write_slice_to_arena(&arena, program.name);
    write_str_to_arena(&arena, "_pio_hex[] = {\n");
    for (i32 i = 0; i < program.settings.length; i++) {
        write_str_to_arena(&arena, "\t{ .hex = ");
        write_hex16_to_arena(&arena, program.operations.items[i].hex);
        write_str_to_arena(&arena, " },\n");
    }
    write_str_to_arena(&arena, "};\n\n");


    write_str_to_arena(&arena, "pio_program_t ");
    write_slice_to_arena(&arena, program.name);
    write_str_to_arena(&arena, " = {\n");
    write_str_to_arena(&arena, "\t.settings = (pio_settings_t) {\n");

    write_str_to_arena(&arena, "\t\t.origin = ");
    write_decimal_to_arena(&arena, program.settings.origin);
    write_str_to_arena(&arena, ",\n");
    write_str_to_arena(&arena, "\t\t.side_set = ");
    write_decimal_to_arena(&arena, program.settings.side_set);
    write_str_to_arena(&arena, ",\n");
    write_str_to_arena(&arena, "\t\t.side_set_opt = ");
    write_decimal_to_arena(&arena, program.settings.side_set_opt);
    write_str_to_arena(&arena, ",\n");
    write_str_to_arena(&arena, "\t\t.side_set_pindirs = ");
    write_decimal_to_arena(&arena, program.settings.side_set_pindirs);
    write_str_to_arena(&arena, ",\n");
    write_str_to_arena(&arena, "\t\t.length = ");
    write_decimal_to_arena(&arena, program.settings.length);
    write_str_to_arena(&arena, ",\n");
    write_str_to_arena(&arena, "\t\t.wrap_target = ");
    write_decimal_to_arena(&arena, program.settings.wrap_target);
    write_str_to_arena(&arena, ",\n");
    write_str_to_arena(&arena, "\t\t.wrap = ");
    write_decimal_to_arena(&arena, program.settings.wrap);
    write_str_to_arena(&arena, ",\n");

    write_str_to_arena(&arena, "\t},\n");

    write_str_to_arena(&arena, "\t.instructions_hex = (pio_op_t *)&");
    write_slice_to_arena(&arena, program.name);
    write_str_to_arena(&arena, "_pio_hex,\n");
    write_str_to_arena(&arena, "};\n");

    WriteEntireFile(FileName, arena.memory, arena.used);

    free_arena(&arena);
}


slice_array_t get_lines_from_file(file_contents_t file) {
    slice_array_t lines = get_new_slice_array(500);
    slice_t sequence = { .str = file.contents, .len = file.contents_size };

    split_slice(&lines, sequence, '\n', FALSE);

    return lines;
}


void print_array(slice_array_t array) {
    for (int i = 0; i < array.length; i++) {
        printf("slice %3d,  len: %3d,   |", i, array.items[i].len);
        print_slice(array.items[i]);
        printf("\n");
    }
}


pio_op_t parse_instruction(slice_array_t words, parse_prog_t parse_prog, pio_program_t *program) {
    pio_op_t instruction;
    slice_array_t line_words = words;

    u8 delay_value = 0;
    if (words.length >= 2) {
        slice_t delay_sl = words.items[words.length - 1];
        if (delay_sl.str[0] == '[') {
            if (delay_sl.str[delay_sl.len-1] == ']') {
                if (parse_number_u8((slice_t){ .str = &delay_sl.str[1], .len = delay_sl.len - 2 }, &delay_value)) {
                    words.length -= 1;
                    if (31 < delay_value) {
                        print_std_error("Incorrect value", "Delay must have value from 0 to 31 inclusive.\n");
                        print_stderror_line_parse_failed_at(line_words, delay_sl);
                    }
                    if (parse_prog.settings.side_set != 0) {
                        if (delay_value >= (1 << (5 - parse_prog.settings.side_set - parse_prog.settings.side_set_opt))) {
                            print_std_error("Value too big", "When the side_set is in use, the delay value range shrinks.\n");
                            print_stderror_line_parse_failed_at(line_words, delay_sl);
                            fprintf(stderr, YELLOW "INFO:  " RESET "Side_set is using %d + %d bits. Found value %d, should be betwee 0 and %d\n",
                                    parse_prog.settings.side_set, parse_prog.settings.side_set_opt, delay_value,
                                    (1 << (5 - parse_prog.settings.side_set + parse_prog.settings.side_set_opt)) - 1 );
                        }
                    }
                }
                else {
                    print_std_error("Incorrect value", "Delay value must be a number.\n");
                }
            }
            else {
                print_line_parse_failed(words);
                print_std_error("Incorrect Formating", "Found no matching ] in this word.\n");
            }
        }
        else {
            // CHECK: What should happen here?
        }
    }


    u8 side_set_value = 0;
    if (words.length >= 3) {
        slice_t side_sl = words.items[words.length - 2];
        slice_t side_set_val_sl = words.items[words.length - 1];
        if (is_slice_equal_to_str(side_sl, "side")) {
            if (parse_prog.settings.side_set == 0) {
                print_std_error("Invalid usage", "Using side, when .side_set directive was not configured.\n");
                print_stderror_line_parse_failed_at(line_words, side_sl);
            }

            if (parse_number_u8(side_set_val_sl, &side_set_value)) {
                words.length -= 2;

                if ((side_set_value >> parse_prog.settings.side_set) != 0) {
                    print_std_error("Invalid value", "Number of bits in use by side_set is too small.\n");
                    print_stderror_line_parse_failed_at(line_words, side_set_val_sl);
                    fprintf(stderr, YELLOW "INFO:  " RESET "Can't represent value %d with currently used %d bits.\n",
                            side_set_value, parse_prog.settings.side_set);
                }
            }
            else {
                print_std_error("Incorrect value", "Side set value must be a number.\n");
                print_stderror_line_parse_failed_at(line_words, side_set_val_sl);
            }
        }
        else if (is_slice_equal_to_str(side_set_val_sl, "side")) {
                print_line_parse_failed(words);
                print_std_error("Incorrect Formating", "Found no matching value for side.\n");
        }
    }


    u8 delay_side_value = delay_value + (side_set_value << (5 - parse_prog.settings.side_set - parse_prog.settings.side_set_opt));


    if (is_slice_equal_to_str(words.items[0], "jmp")) {
        u8 cond = 0;
        u8 addr = 0;

        if (print_line_failed_many_few(2, 3, words)) {
            print_std_error_format("jmp (!X|X--|!Y|Y--|X!=Y|PIN|!OSRE) <target_label>", line_words);
        }
        else if (words.length == 3) {
            slice_t condition = words.items[1];
            if (is_slice_equal_to_str(condition, "!X")) { cond = 0b001; }
            else if (is_slice_equal_to_str(condition, "X--")) { cond = 0b010; }
            else if (is_slice_equal_to_str(condition, "!Y")) { cond = 0b011; }
            else if (is_slice_equal_to_str(condition, "Y--")) { cond = 0b100; }
            else if (is_slice_equal_to_str(condition, "X!=Y")) { cond = 0b101; }
            else if (is_slice_equal_to_str(condition, "PIN")) { cond = 0b110; }
            else if (is_slice_equal_to_str(condition, "!OSRE")) { cond = 0b111; }
            else {
                print_std_error("Invalid condition for JMP",
                                "Expected one of: !X, X--, !Y, Y--, X!=Y, PIN, !OSRE.\n");
                print_stderror_line_parse_failed_at(line_words, condition);
            }
        }
        else {
            cond = 0;
        }

        slice_t target_sl = words.items[words.length - 1];
        if (parse_number_u8(target_sl, &addr)) {
        }
        else if (get_label_value_from_parse_prog(&parse_prog, target_sl, &addr)) {
            addr += program->settings.origin;
        }
        else {
            print_std_error("Incorrect value", "JMP address must be a number or defined label\n");
            print_stderror_line_parse_failed_at(line_words, target_sl);
        }

        if (31 < addr) {
            print_std_error("Incorrect value", "JMP address must be between 0 and 31 included.\n");
            print_stderror_line_parse_failed_at(line_words, target_sl);
            print_number_out_of_range(0, 31, addr);
        }

        instruction = encode_jmp(delay_side_value, cond, addr);
    }

    else if (is_slice_equal_to_str(words.items[0], "wait")) {

        if (print_line_failed_many_few(4, 5, words)) {
            print_std_error_format("wait <0|1> <GPIO|PIN|IRQ> <gpio_num|pin_num|irq_num>", line_words);
        }

        u8 rel = 0;

        if (words.length == 5) {
            slice_t rel_sl = words.items[4];
            if (is_slice_equal_to_str(rel_sl, "rel")) {
                rel = 0x10; // CHECK: Is this actully correct? Just set bit 4 of Index field? Same as for irq instruction?
                words.length -= 1;
            }
            else {
                print_std_error("Incorrect value", "Should probably be 'rel'.\n");
                print_stderror_line_parse_failed_at(line_words, rel_sl);
            }
        }

        u8 pol;
        slice_t polarity_sl = words.items[1];
        if (parse_number_u8(polarity_sl, &pol)) {
            if (pol != 0 && pol != 1) {
                print_std_error("Incorrect value", "Polarity must be 0 or 1.\n");
                print_stderror_line_parse_failed_at(line_words, polarity_sl);
            }
        }
        else {
            print_std_error("Incorrect value", "Polarity must be a number, 0 or 1.\n");
            print_stderror_line_parse_failed_at(line_words, polarity_sl);
        }


        u8 src;
        slice_t source_sl = words.items[2];
        if (is_slice_equal_to_str(source_sl, "GPIO")) { src = 0b00; }
        else if (is_slice_equal_to_str(source_sl, "PIN")) { src = 0b01; }
        else if (is_slice_equal_to_str(source_sl, "IRQ")) { src = 0b10; }
        else {
            print_std_error("Invalid argument for JMP", "Expected one of: GPIO, PIN, IRQ.\n");
            print_stderror_line_parse_failed_at(line_words, source_sl);
        }


        u8 idx;
        slice_t index_sl = words.items[3];
        if (parse_number_u8(index_sl, &idx)) {
            if ((is_slice_equal_to_str(source_sl, "GPIO")) ||
                (is_slice_equal_to_str(source_sl, "PIN"))) {
                if (31 < idx) {
                    print_number_out_of_range(0, 31, idx);
                    print_stderror_line_parse_failed_at(line_words, index_sl);
                }
            }
            else if (is_slice_equal_to_str(source_sl, "IRQ")) {
                if (7 < idx) {
                    print_number_out_of_range(0, 7, idx);
                    print_stderror_line_parse_failed_at(line_words, index_sl);
                }
                idx |= rel;
            }
        }
        else {
            print_std_error("Incorrect value", "Index must be a number.\n");
            print_stderror_line_parse_failed_at(line_words, index_sl);
        }

        instruction = encode_wait(delay_side_value, pol, src, idx);
    }

    else if (is_slice_equal_to_str(words.items[0], "in")) {

        if (print_line_failed_many_few(3, 3, words)) {
            print_std_error_format("in <PINS|X|Y|NULL|ISR|OSR> <bit_count>", line_words);
        }


        u8 src;
        slice_t source_sl = words.items[1];
        if (is_slice_equal_to_str(source_sl, "PINS")) { src = 0b000; }
        else if (is_slice_equal_to_str(source_sl, "X")) { src = 0b001; }
        else if (is_slice_equal_to_str(source_sl, "Y")) { src = 0b010; }
        else if (is_slice_equal_to_str(source_sl, "NULL")) { src = 0b011; }
        else if (is_slice_equal_to_str(source_sl, "ISR")) { src = 0b110; }
        else if (is_slice_equal_to_str(source_sl, "OSR")) { src = 0b111; }
        else {
            print_std_error("Invalid argument for IN", "Expected one of: PINS, X, Y, NULL, ISR, OSR.\n");
            print_stderror_line_parse_failed_at(line_words, source_sl);
        }

        u8 count;
        slice_t count_sl = words.items[2];
        if (parse_number_u8(count_sl, &count)) {
            if ((count < 1) || (32 < count)) {
                print_number_out_of_range(1, 32, count);
                print_stderror_line_parse_failed_at(line_words, count_sl);
            }
        }
        else {
            print_std_error("Incorrect value", "Count must be a number.\n");
            print_stderror_line_parse_failed_at(line_words, count_sl);
        }

        instruction = encode_in(delay_side_value, src, count);
    }

    else if (is_slice_equal_to_str(words.items[0], "out")) {

        if (print_line_failed_many_few(3, 3, words)) {
            print_std_error_format("out <PINS|X|Y|NULL|PINDIRS|PC|ISR|OSR> <bit_count>", line_words);
        }

        u8 dest;
        slice_t destination_sl = words.items[1];
        if (is_slice_equal_to_str(destination_sl, "PINS")) { dest = 0b000; }
        else if (is_slice_equal_to_str(destination_sl, "X")) { dest = 0b001; }
        else if (is_slice_equal_to_str(destination_sl, "Y")) { dest = 0b010; }
        else if (is_slice_equal_to_str(destination_sl, "NULL")) { dest = 0b011; }
        else if (is_slice_equal_to_str(destination_sl, "PINDIRS")) { dest = 0b100; }
        else if (is_slice_equal_to_str(destination_sl, "PC")) { dest = 0b101; }
        else if (is_slice_equal_to_str(destination_sl, "ISR")) { dest = 0b110; }
        else if (is_slice_equal_to_str(destination_sl, "EXEC")) { dest = 0b111; }
        else {
            print_std_error("Invalid argument for OUT",
                            "Expected one of: PINS, X, Y, NULL, PINDIRS, PC, ISR, EXEC.\n");
            print_stderror_line_parse_failed_at(line_words, destination_sl);
        }

        u8 count;
        slice_t count_sl = words.items[2];
        if (parse_number_u8(count_sl, &count)) {
            if ((count < 1) || (32 < count)) {
                print_number_out_of_range(1, 32, count);
                print_stderror_line_parse_failed_at(line_words, count_sl);
            }
        }
        else {
            print_std_error("Incorrect value", "Count must be a number.\n");
            print_stderror_line_parse_failed_at(line_words, count_sl);
        }

        instruction = encode_out(delay_side_value, dest, count);
    }

    else if (is_slice_equal_to_str(words.items[0], "push")) {

        if (print_line_failed_many_few(1, 3, words)) {
            print_std_error_format("push (iffull) (block|noblock)", line_words);
        }

        u8 iff = 0;
        u8 blk = 1;

        if (words.length == 2) {

            slice_t second_sl = words.items[1];

            if (is_slice_equal_to_str(second_sl, "iffull")) { iff = 1; }
            else if (is_slice_equal_to_str(second_sl, "block")) { blk = 1; }
            else if (is_slice_equal_to_str(second_sl, "noblock")) { blk = 0; }
            else {
                print_std_error("Invalid argument for PUSH", "Expected one of: iffull, block, noblock.\n");
                print_stderror_line_parse_failed_at(line_words, second_sl);
            }
        }
        else if (words.length == 3) {

            slice_t iffull_sl = words.items[1];
            slice_t block_sl = words.items[2];

            if (is_slice_equal_to_str(iffull_sl, "iffull")) { iff = 1; }
            else {
                print_std_error("Invalid argument for PUSH", "Expected: iffull.\n");
                print_stderror_line_parse_failed_at(line_words, iffull_sl);
            }

            if (is_slice_equal_to_str(block_sl, "block")) { blk = 1; }
            else if (is_slice_equal_to_str(block_sl, "noblock")) { blk = 0; }
            else {
                print_std_error("Invalid argument for PUSH", "Expected one of: block, noblock.\n");
                print_stderror_line_parse_failed_at(line_words, block_sl);
            }
        }

        instruction = encode_push(delay_side_value, iff, blk);
    }

    else if (is_slice_equal_to_str(words.items[0], "pull")) {

        if (print_line_failed_many_few(1, 3, words)) {
            print_std_error_format("pull (ifempty) (block|noblock)", line_words);
        }

        u8 ife = 0;
        u8 blk = 1;

        if (words.length == 2) {
            slice_t second_sl = words.items[1];

            if (is_slice_equal_to_str(second_sl, "ifempty")) { ife = 1; }
            else if (is_slice_equal_to_str(second_sl, "block")) { blk = 1; }
            else if (is_slice_equal_to_str(second_sl, "noblock")) { blk = 0; }
            else {
                print_std_error("Invalid argument for PULL", "Expected one of: ifempty, block, noblock.\n");
                print_stderror_line_parse_failed_at(line_words, second_sl);
            }
        }
        else if (words.length == 3) {
            slice_t ifempty_sl = words.items[1];
            slice_t block_sl = words.items[2];

            if (is_slice_equal_to_str(ifempty_sl, "ifempty")) { ife = 1; }
            else {
                print_std_error("Invalid argument for PULL", "Expected 2nd argument: ifempty.\n");
                print_stderror_line_parse_failed_at(line_words, ifempty_sl);
            }

            if (is_slice_equal_to_str(block_sl, "block")) { blk = 1; }
            else if (is_slice_equal_to_str(block_sl, "noblock")) { blk = 0; }
            else {
                print_std_error("Invalid argument for PULL", "Expected 3rd argument is one of: block, noblock.\n");
                print_stderror_line_parse_failed_at(line_words, block_sl);
            }
        }

        instruction = encode_pull(delay_side_value, ife, blk);
    }

    else if (is_slice_equal_to_str(words.items[0], "nop")) {
        if (print_line_failed_many_few(1, 1, words)) {
            print_std_error_format("nop", line_words);
        }
        instruction = encode_mov(delay_side_value, 2, 0, 2);
    }

    else if (is_slice_equal_to_str(words.items[0], "mov")) {

        if (print_line_failed_many_few(3, 4, words)) {
            print_std_error_format("mov <PINS|X|Y|EXEC|PC|ISR|OSR> (!|~|::) <PINS|X|Y|NULL|STATUS|ISR|OSR>", line_words);
        }

        u8 dest;
        slice_t destination_sl = words.items[1];
        if (is_slice_equal_to_str(destination_sl, "PINS")) { dest = 0b000; }
        else if (is_slice_equal_to_str(destination_sl, "X")) { dest = 0b001; }
        else if (is_slice_equal_to_str(destination_sl, "Y")) { dest = 0b010; }
        else if (is_slice_equal_to_str(destination_sl, "EXEC")) { dest = 0b100; }
        else if (is_slice_equal_to_str(destination_sl, "PC")) { dest = 0b101; }
        else if (is_slice_equal_to_str(destination_sl, "ISR")) { dest = 0b110; }
        else if (is_slice_equal_to_str(destination_sl, "OSR")) { dest = 0b111; }
        else {
            print_std_error("Invalid destination for MOV",
                            "Expected one of: X, Y, EXEC, PC, ISR, OSR.\n");
            print_stderror_line_parse_failed_at(line_words, destination_sl);
        }


        u8 src;
        slice_t source_sl = words.items[words.length - 1];
        if (is_slice_equal_to_str(source_sl, "PINS")) { src = 0b000; }
        else if (is_slice_equal_to_str(source_sl, "X")) { src = 0b001; }
        else if (is_slice_equal_to_str(source_sl, "Y")) { src = 0b010; }
        else if (is_slice_equal_to_str(source_sl, "NULL")) { src = 0b011; }
        else if (is_slice_equal_to_str(source_sl, "STATUS")) { src = 0b0101; }
        else if (is_slice_equal_to_str(source_sl, "ISR")) { src = 0b110; }
        else if (is_slice_equal_to_str(source_sl, "OSR")) { src = 0b111; }
        else {
            print_std_error("Invalid source for MOV",
                            "Expected one of: PINS, X, Y, NULL, STATUS, ISR, OSR.\n");
            print_stderror_line_parse_failed_at(line_words, source_sl);
        }

        u8 op = 0;
        if (words.length == 4) {
            slice_t operation = words.items[2];
            if (is_slice_equal_to_str(operation, "-")) { op = 0b01; }
            else if (is_slice_equal_to_str(operation, "~")) { op = 0b01; }
            else if (is_slice_equal_to_str(operation, "::")) { op = 0b10; }
            else {
                print_std_error("Invalid operation for MOV",
                                "Expected one of: -, ~, ::.\n");
                print_stderror_line_parse_failed_at(line_words, source_sl);
            }
        }


        instruction = encode_mov(delay_side_value, dest, op, src);
    }

    else if (is_slice_equal_to_str(words.items[0], "irq")) {

        if (print_line_failed_many_few(2, 4, words)) {
            print_std_error_format("irq (set|nowait|wait|clear) <irq_num> (rel)", line_words);
        }

        u8 rel = 0;
        slice_t rel_sl = words.items[words.length - 1];
        if (is_slice_equal_to_str(rel_sl, "rel")) {
            rel = 0x10; // CHECK: Is this actually correct? Just set bit 4 of Index field?
            words.length -= 1;
        }

        if (words.length > 3) {
            print_std_error_format("irq (set|nowait|wait|clear) <irq_num> (rel)", line_words);
        }

        u8 clr = 0;
        u8 wait = 0;
        if (words.length == 3) {
            slice_t type_sl = words.items[1];
            if (is_slice_equal_to_str(type_sl, "set")) { clr = 0; }
            else if (is_slice_equal_to_str(type_sl, "nowait")) { wait = 0; }
            else if (is_slice_equal_to_str(type_sl, "wait")) { wait = 1; }
            else if (is_slice_equal_to_str(type_sl, "clear")) { clr = 1; }
            else {
                print_std_error("Invalid type for IRQ",
                                "Expected one of: set, nowait wait clear.\n");
                print_stderror_line_parse_failed_at(line_words, type_sl);
            }
        }

        u8 idx = 0;
        slice_t irq_num_sl = words.items[words.length - 1];
        if (parse_number_u8(irq_num_sl, &idx)) {
            if (7 < idx) {
                print_number_out_of_range(0, 7, idx);
                print_stderror_line_parse_failed_at(line_words, irq_num_sl);
            }
        }
        else {
            print_std_error("Incorrect value", "Irq_num must be a number.\n");
            print_stderror_line_parse_failed_at(line_words, irq_num_sl);
        }

        instruction = encode_irq(delay_side_value, clr, wait, rel | idx);
    }

    else if (is_slice_equal_to_str(words.items[0], "set")) {

        if (print_line_failed_many_few(3, 3, words)) {
            print_std_error_format("set <PINS|X|Y|PINDIRS> <val>", line_words);
        }

        u8 dest;
        slice_t destination_sl = words.items[1];
        if (is_slice_equal_to_str(destination_sl, "PINS")) { dest = 0b000; }
        else if (is_slice_equal_to_str(destination_sl, "X")) { dest = 0b001; }
        else if (is_slice_equal_to_str(destination_sl, "Y")) { dest = 0b010; }
        else if (is_slice_equal_to_str(destination_sl, "PINDIRS")) { dest = 0b100; }
        else {
            print_std_error("Invalid destination for SET",
                            "Expected one of: PINS, X, Y, PINDIRS.\n");
            print_stderror_line_parse_failed_at(line_words, destination_sl);
        }

        u8 data;
        slice_t value_sl = words.items[2];
        if (parse_number_u8(value_sl, &data)) {
            if (31 < data) {
                print_number_out_of_range(0, 31, data);
                print_stderror_line_parse_failed_at(line_words, value_sl);
            }
        }
        else {
            print_std_error("Incorrect value", "Index must be a number.\n");
            print_stderror_line_parse_failed_at(line_words, value_sl);
        }

        instruction = encode_set(delay_side_value, dest, data);
    }

    else {
        print_std_error("Invalid instruction", "\n");
        print_stderror_line_parse_failed_at(line_words, words.items[0]);
    }

    append_op_to_pio_array(&program->operations, instruction);
    return instruction;
}


void erase_comment(slice_t *line) {
    for (i32 i = 0; i < line->len; i++) {
        if (line->str[i] == ';') {
            line->len = i;
            break;
        }
    }
    for (i32 i = 0; i < line->len - 1; i++) {
        if (line->str[i] == '/' && line->str[i+1] == '/') {
            line->len = i;
            break;
        }
    }
}


void erase_start_end_whitespace(slice_t *line) {
    if (line->len == 0) {
        return;
    }

    i32 i;

    for (i = 0; i < line->len; i++) {
        if ((line->str[i] != ' ') && (line->str[i] != '\t')) {
            line->str = &line->str[i];
            line->len = line->len - i;
            break;
        }
    }

    if (i == line->len) {
        line->len = 0;
        return;
    }

    for (i = line->len - 1; i >= 0; i--) {
        if ((line->str[i] != ' ') && (line->str[i] != '\t')) {
            line->len = i+1;
            break;
        }
    }
}


void remove_empty_slices(slice_array_t *array) {
    i32 new_length = 0;
    for (i32 i = 0; i < array->length; i++) {
        if (array->items[i].len != 0) {
            array->items[new_length] = array->items[i];
            new_length++;
        }
    }
    array->length = new_length;
}


void delete_non_code(slice_array_t *array) {
    for (i32 i = 0; i < array->length; i++) {
        erase_comment(&array->items[i]);
        erase_start_end_whitespace(&array->items[i]);
    }
    remove_empty_slices(array);
}


b32 is_legal_program_name(slice_t name) {
    char ch = name.str[0];
    if ('0' <= ch && ch <= '9')
        return FALSE;

    for (i32 i = 0; i < name.len; i++) {
        char ch = name.str[i];
        if (
            !('0' <= ch && ch <= '9') &&
            !('A' <= ch && ch <= 'Z') &&
            !('a' <= ch && ch <= 'z') &&
            !(ch == '_')
        ) {
            return FALSE;
        }
    }
    return TRUE;
}


void parse_directive(slice_array_t *words, parse_prog_t *prog) {
    slice_array_t line_words = *words;
    slice_t directive = words->items[0];

    if (directive.str[0] != '.') {
        printf("Error: This is not a directive: ");
        print_std_error("Wrong formating", "This is not a directive.\n");
        print_line_parse_failed(line_words);
        exit(-1);
    }


    if (is_slice_equal_to_str(directive, ".program")) {
        // NOTE:: Decide what to do here.
        if (words->length == 2) {
            slice_t name_sl = words->items[1];
            if (is_legal_program_name(name_sl)) {
                prog->name = words->items[1];
            }
            else {
                print_std_error("Wrong format", "The only valid characters in program name are: '_', '0-9', 'A-Z', 'a-z'.\n");
                print_stderror_line_parse_failed_at(line_words, name_sl);
            }
        }
        else {
            print_std_error("Wrong format", "Directive .program takes one argument (.program <program_name>).");
            print_line_parse_failed(*words);
        }
    }

    else if (is_slice_equal_to_str(directive, ".wrap_target")) {
        if (words->length == 1) {
            prog->settings.wrap_target = prog->settings.length;
        }
        else {
            print_std_error("Wrong format", "Directive .wrap_target takes no arguments.");
            print_line_parse_failed(*words);
        }
    }

    else if (is_slice_equal_to_str(directive, ".wrap")) {
        if (words->length == 1) {
            prog->settings.wrap = (prog->settings.length - 1) & 0x1F;
        }
        else {
            print_std_error("Wrong format", "Directive .wrap takes no arguments.");
            print_line_parse_failed(*words);
        }
    }

    else if (is_slice_equal_to_str(directive, ".origin")) {
        u8 origin;
        slice_t value_sl = words->items[1];
        if (parse_number_u8(value_sl, &origin)) {
            if (origin <= 31) {
                prog->settings.origin = origin;
            }
            else {
                print_number_out_of_range(0, 31, origin);
                print_stderror_line_parse_failed_at(line_words, value_sl);
            }
        }
        else {
            print_std_error("Incorrect value", "Value of .origin must be a number\n");
            print_line_parse_failed(*words);
        }
    }

    // NOTE: Decide whether to support this directive.
    //
    // else if (is_slice_equal_to_str(directive, ".word")) {
    //     // prog->settings.length += 1;
    // }

    else if (is_slice_equal_to_str(directive, ".side_set")) {

        i8 side_set_opt_val = 0;
        i8 side_set_pindirs_val = 0;

        if (words->length == 2) {
        }
        else if (words->length == 3) {
            slice_t arg1 = words->items[2];
            if (is_slice_equal_to_str(arg1, "opt")) {
                side_set_opt_val = 1;
            }
            else if (is_slice_equal_to_str(arg1, "pindirs")) {
                side_set_pindirs_val = 1;
            }
            else {
                print_std_error("Invalid value", "Expected one of: opt, pindirs.\n");
                print_stderror_line_parse_failed_at(line_words, arg1);
            }
        }
        else if (words->length == 4) {
            slice_t arg_opt = words->items[2];
            slice_t arg_pindirs = words->items[3];

            if (is_slice_equal_to_str(arg_opt, "opt")) {
                side_set_opt_val = 1;
            }
            else {
                print_std_error("Invalid value", "Expected opt.\n");
                print_stderror_line_parse_failed_at(line_words, arg_opt);
            }

            if (is_slice_equal_to_str(arg_pindirs, "pindirs")) {
                side_set_pindirs_val = 1;
            }
            else {
                print_std_error("Invalid value", "Expected pindirs.\n");
                print_stderror_line_parse_failed_at(line_words, arg_pindirs);
            }
        }
        else {
            print_std_error("Incorrect format", "Expected format .side_set <count> (opt) (pindirs).\n");
            print_line_parse_failed(line_words);
        }


        u8 side_set_val = 0;

        slice_t side_set_value_sl = words->items[1];
        if (parse_number_u8(side_set_value_sl, &side_set_val)) {
            if ((1 <= side_set_val) && (side_set_val <= 5 - side_set_opt_val)) {
                prog->settings.side_set = side_set_val;
                prog->settings.side_set_opt = side_set_opt_val;
                prog->settings.side_set_pindirs = side_set_pindirs_val;
            }
            else {
                print_number_out_of_range(1, 5 - side_set_opt_val, side_set_val);
                print_stderror_line_parse_failed_at(line_words, side_set_value_sl);
            }
        }
        else {
            print_std_error("Incorrect value", "Value of a side_set must be a number.\n");
            print_stderror_line_parse_failed_at(line_words, side_set_value_sl);
        }
    }

    else if (is_slice_equal_to_str(directive, ".define")) {
        if (words->length == 3) {
            u8 val;
            slice_t symbol_sl = words->items[1];
            slice_t value_sl = words->items[2];
            if (parse_number_u8(value_sl, &val)) {
                append_to_pio_label_array(&prog->labels,
                                          (pio_label_t) { .name = symbol_sl, .val = val });
            }
            else {
                print_std_error("Incorrect value", "Value of a symbol should be a number.\n");
                print_stderror_line_parse_failed_at(line_words, value_sl);
            }
        }
        else {
            print_std_error("Incorrect format", "Expected format .define <symbol> <value>.\n");
            print_line_parse_failed(line_words);
        }
    }

    else {
        print_std_error("Unsupported directive", "Usage of unsupported directive.\n");
        print_line_parse_failed(line_words);
    }
}


void parse_label(slice_array_t *words, parse_prog_t *prog) {
    if (words->length == 1) {
        slice_t label = words->items[0];
        append_to_pio_label_array(&prog->labels,
                                  (pio_label_t) {
                                      .name = (slice_t) { .str = label.str, .len = label.len -1 },
                                      .val = prog->settings.length
                                  });
    }
    else {
        print_std_warn("Too Many Words", "");
        print_line_parse_failed(*words);
    }
}


b32 is_instruction(slice_t word) {
    if (is_slice_equal_to_str(word, "jmp"))         return TRUE;
    else if (is_slice_equal_to_str(word, "wait"))   return TRUE;
    else if (is_slice_equal_to_str(word, "in"))     return TRUE;
    else if (is_slice_equal_to_str(word, "out"))    return TRUE;
    else if (is_slice_equal_to_str(word, "push"))   return TRUE;
    else if (is_slice_equal_to_str(word, "pull"))   return TRUE;
    else if (is_slice_equal_to_str(word, "mov"))    return TRUE;
    else if (is_slice_equal_to_str(word, "irq"))    return TRUE;
    else if (is_slice_equal_to_str(word, "set"))    return TRUE;
    else if (is_slice_equal_to_str(word, "nop"))    return TRUE;
    else return FALSE;
}


void print_settings(pio_settings_t settings) {
    printf("Program Settings\n");
    printf("\torigin: \t%d\n", settings.origin);
    printf("\tside-set: \t%d\n", settings.side_set);
    printf("\tlength: \t%d\n", settings.length);
    printf("\twrap_target: \t%d\n", settings.wrap_target);
    printf("\twraptarget: \t%d\n", settings.wrap);
}


void print_program(pio_program_t program) {
    printf("\n\n=============================================\n");
    printf("---------------------------------------------\n");
    printf("PROGRAM: %.*s\n", program.name.len, program.name.str);
    printf("---------------------------------------------\n");
    print_settings(program.settings);
    printf("---------------------------------------------\n");
    printf("HEX:\n");
    printf("----------\n");
    print_pio_program(program.operations);
    printf("---------------------------------------------\n");
    printf("=============================================\n\n\n");
}


pio_program_t parse_program_file(file_contents_t program_file) {
    slice_array_t lines = get_lines_from_file(program_file);
    slice_array_t words = get_new_slice_array(10);

    delete_non_code(&lines);

    slice_t blank_space = SLICE_FROM_STRING("\t ,");

    parse_prog_t parse_prog = {
        .name = SLICE_FROM_STRING("test_pio_program"),
        .labels = get_new_pio_label_array() };


    for (int i = 0; i < lines.length; i++) {
        split_slice_multiple(&words, lines.items[i], blank_space, FALSE);

        slice_t operation = words.items[0];

        if (operation.str[0] == '.') {
            parse_directive(&words, &parse_prog);
            lines.items[i].len = 0;
        }

        else if (operation.str[operation.len-1] == ':') {
            parse_label(&words, &parse_prog);
            lines.items[i].len = 0;
        }
        else if (is_instruction(operation)) {
            if (parse_prog.settings.length > 31) {
            print_std_error("Program too large", "There are too many instructions in the program.\n");
            print_stderror_slice_array("Failed on line: ", words, "\n");
            }
            parse_prog.settings.length += 1;
        }
        else {
        }

        words.length = 0;
    }

    remove_empty_slices(&lines);
    pio_program_t program = {
        .name = parse_prog.name,
        .settings = parse_prog.settings,
        .operations = get_new_pio_array(100)
    };

    for (i32 i = 0; i < lines.length; i++) {
        split_slice_multiple(&words, lines.items[i], blank_space, FALSE);
        parse_instruction(words, parse_prog, &program);
        words.length = 0;
    }

    if (program.settings.origin + program.settings.length > 32) {
        fprintf(stderr, "ERROR: [Out of memory]: Program with origin at %d and with %d instructions gets out of available memory.\n",
                program.settings.origin, program.settings.length);
    }

    free_slice_array(&words);
    free_slice_array(&lines);

    return program;
}


int main(int argc, char** argv) {

    char *in_file_name;
    char *out_file_name;

    if (argc == 3) {
        in_file_name = argv[2];
        out_file_name = argv[1];
    }
    else {
        fprintf(stderr, RED "ERROR:" RESET " [pio_asm takes 2 arguments]  pio_asm out_file in_file\n");
        return 1;
    }

    file_contents_t program_file = ReadEntireFile(in_file_name);

    if (program_file.contents == NULL) {
        fprintf(stderr, RED "ERROR:" RESET " File " YELLOW "%s" RESET " does not exist.\n", in_file_name);
        return 2;
    }

    pio_program_t program = parse_program_file(program_file);

    write_program_to_file(program, out_file_name);

    return 0;
}

