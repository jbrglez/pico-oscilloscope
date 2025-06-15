#ifndef MY_STRINGS_C
#define MY_STRINGS_C

#include <stdlib.h>
#include <stdio.h>

#include "my_types.h"
#include "arena.c"


typedef struct {
    char *str;
    i32 len;
} slice_t;

#define SLICE_FROM_STRING(string) (slice_t){ .str = string, .len = sizeof(string) - 1 }


typedef struct {
    slice_t *items;
    i32 length;
    i32 capacity;
} slice_array_t;


void print_slice(slice_t slice) {
    for (i32 i = 0; i < slice.len; i++) {
        printf("%c", slice.str[i]);
    }
}


slice_array_t get_new_slice_array(u32 cap) {
    cap = 64 * ( (cap/64) + 1);

    slice_array_t new = {
        .length = 0,
        .capacity = cap,
    };
    new.items = calloc(new.capacity, sizeof(slice_t));

    if (new.items == NULL) {
        new.capacity = 0;
        fprintf(stderr, "ERROR: new slice array allocation failed");
    }

    return new;
}


i32 append_slice_to_array(slice_array_t *array, slice_t slice) {
    if (array->length >= array->capacity) {
        void *new_loc = reallocarray(array->items, array->capacity + 64, sizeof(slice_t));
        if (new_loc == NULL) {
            fprintf(stderr, "ERROR: Failed to append to slice array");
            return -1;
        }
        else {
            array->items = new_loc;
            array->capacity += 64;
        }
    }
    array->items[array->length] = slice;
    array->length++;
    return 0;
}


i32 free_slice_array(slice_array_t *array) {
    array->length = 0;
    array->capacity = 0;
    free(array->items);
    array->items = 0;

    return 0;
}


i32 get_str_len(char *str) {
    i32 i = 0;
    while (str[i] != 0) i++;
    return i;
}


slice_t slice_from_str(char *str) {
    return (slice_t){ .str = str, .len = get_str_len(str) };
}


i32 are_slices_equal(slice_t a, slice_t b) {
    if (a.len != b.len) {
        return FALSE;
    }
    for (i32 i = 0; i < a.len; i++) {
        if (a.str[i] != b.str[i]){
            return FALSE;
        }
    }

    return TRUE;
}


i32 is_slice_equal_to_str(slice_t slice, char *str) {
    int i = 0;
    while ((i < slice.len) && (str[i] != 0)) {
        if (slice.str[i] != str[i]) {
            return FALSE;
        }
        i++;
    }
    if ((i == slice.len) && (str[i] != 0)) {
        return FALSE;
    }

    return TRUE;
}


i32 slice_begins_with(slice_t sequence, slice_t pattern) {
    // NOTE: Decide what to do if pattern is empty (len = 0).
    if (pattern.len > sequence.len)
        return FALSE;

    for (i32 i = 0; i < pattern.len; i++) {
        if (sequence.str[i] != pattern.str[i])
            return FALSE;
    }

    return TRUE;
}


i32 match_slice_in_slice(slice_t sequence, slice_t pattern) {
    if (pattern.len > sequence.len) 
        return -1;

    for (i32 i = 0; pattern.len + i <= sequence.len; i++) {
        slice_t n_puzzle = { .str = &sequence.str[i], .len = sequence.len - 1 };
        if (slice_begins_with(n_puzzle, pattern))
            return i;
    }

    return -1;
}


i32 match_str_in_slice(slice_t sequence, char *pattern_str) {
    i32 len = get_str_len(pattern_str);
    slice_t pattern = { .str = pattern_str, .len = len };
    return match_slice_in_slice(sequence, pattern);
}


void split_slice(slice_array_t *array, slice_t sequence, char split_symbol, b32 keep_symbol) {
    slice_t tmp_slice = { .str = sequence.str, .len = 0 };

    for (i32 i = 0; i < sequence.len; i++) {
        if (sequence.str[i] == split_symbol) {
            if (keep_symbol) {
                tmp_slice.len++;
            }
            if (tmp_slice.len != 0) {
                append_slice_to_array(array, tmp_slice);
            }
            tmp_slice.str = &sequence.str[i+1];
            tmp_slice.len = 0;
        }
        else {
            tmp_slice.len++;
        }
    }

    if (tmp_slice.len != 0) {
        append_slice_to_array(array, tmp_slice);
    }
}


void split_slice_multiple(slice_array_t *array, slice_t sequence, slice_t split_symbols, b32 keep_symbol) {
    slice_t tmp_slice = { .str = sequence.str, .len = 0 };

    for (i32 i = 0; i < sequence.len; i++) {
        i32 is_matched = 0;
        for (i32 j = 0; j < split_symbols.len; j++) {
            if (sequence.str[i] == split_symbols.str[j]) {
                is_matched = 1;
                break;
            }
        }
        if (is_matched) {
            if (keep_symbol) {
                tmp_slice.len++;
            }

            if (tmp_slice.len != 0) {
                append_slice_to_array(array, tmp_slice);
            }

            tmp_slice.str = &sequence.str[i+1];
            tmp_slice.len = 0;
        }
        else {
            tmp_slice.len++;
        }
    }

    if (tmp_slice.len != 0) {
        append_slice_to_array(array, tmp_slice);
    }
}


b32 parse_number_u8(slice_t slice, u8 *result) {
    u8 val = 0;
    if (slice_begins_with(slice, SLICE_FROM_STRING("0x"))) {
        for (int i = 0; i < slice.len; i++) {
            val *= 16;
            char ch = slice.str[i];

            if ('0' <= ch && ch <= '9') { val += ch - '0'; }
            else if ('A' <= ch && ch <= 'F') { val += 10 + ch - 'A'; }
            else if ('a' <= ch && ch <= 'f') { val += 10 + ch - 'a'; }
            else { return FALSE; }
        }
        *result = val;
        return TRUE;
    }
    else if (slice_begins_with(slice, SLICE_FROM_STRING("0o"))) {
        for (int i = 0; i < slice.len; i++) {
            val *= 8;
            char ch = slice.str[i];
            if ('0' <= ch && ch <= '8') { val += ch - '0'; }
            else { return FALSE; }
        }
        *result = val;
        return TRUE;
    }
    else if (slice_begins_with(slice, SLICE_FROM_STRING("0b"))) {
        for (int i = 0; i < slice.len; i++) {
            val *= 2;
            if (slice.str[i] == '1') { val += 1; }
            if (slice.str[i] != '0') { return FALSE; }
        }
        *result = val;
        return TRUE;
    }
    else {
        for (int i = 0; i < slice.len; i++) {
            val *= 10;
            char ch = slice.str[i];
            if ('0' <= ch && ch <= '9') { val += ch - '0'; }
            else { return FALSE; }
        }
        *result = val;
        return TRUE;
    }

    return FALSE;
}


i32 write_slice_to_arena(arena_t *arena, slice_t slice) {
    char *dest = (char *)arena_push(arena, slice.len);
    if (dest == NULL) {
        return -1;
    }

    for (i32 i = 0; i < slice.len; i++) {
        dest[i] = slice.str[i];
    }

    return slice.len;
}


i32 write_str_to_arena(arena_t *arena, char *str) {
    slice_t slice = slice_from_str(str);
    return write_slice_to_arena(arena, slice);
}


i32 write_hex16_to_arena(arena_t *arena, u16 val) {
    char *dest = (char *)arena_push(arena, 6);
    if (dest == NULL) {
        return -1;
    }
    dest[0] = '0';
    dest[1] = 'x';

    i16 n;
    for (int i = 0; i < 4; i++) {
        n = (val>>(i*4)) & 0xF;
        n += (n < 10) ? '0' : ('A' - 10);
        dest[5-i] = (char)n;
    }

    return 6;
}


i32 write_decimal_to_arena(arena_t *arena, u16 val) {
    i32 num_digits = 1;
    i32 power_10 = 10;
    while (val > power_10) {
        num_digits++;
        power_10 *= 10;
    }

    char *dest = (char *)arena_push(arena, num_digits);
    if (dest == NULL) {
        return -1;
    }

    i32 n;
    for (i32 i = 0; i < num_digits; i++) {
        n = val % 10;
        n += '0';
        dest[num_digits - 1 - i] = (char)n;
        val /= 10;
    }

    return num_digits;
}


i32 write_slice_to_buffer(slice_t slice, char *buf, i32 len_buf) {
    if (len_buf < slice.len + 1) {
        return -1;
    }

    for (i32 i = 0; i < slice.len; i++) {
        buf[i] = slice.str[i];
    }
    buf[slice.len] = 0;

    return slice.len;
}


#endif
