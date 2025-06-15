#ifndef RW_PICO_C
#define RW_PICO_C

#include "my_types.h"


typedef enum {
    PICO_RW_READ_ONE = 1,
    PICO_RW_WRITE_ONE = 2,
    PICO_RW_READ_MULTIPLE = 3,
    PICO_RW_WRITE_MULTIPLE = 4,
} pico_rw_instruction_t;


typedef struct __attribute__((packed)) {
    u32 action;
    union {
        struct {
            u32 read_addr;
        };
        struct {
            u32 write_addr;
            u32 write_value;
        };
        struct {
            u32 read_addr_start;
            u32 num_reads;
        };
        struct {
            u32 write_addr_start;
            u32 num_writes;
            u32 write_values[13];
        };
    };
} pico_rw_t;


#endif
