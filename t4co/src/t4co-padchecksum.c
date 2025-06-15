#include "my_types.h"
#include "my_file.c"
#include "arena.c"


#define RESET   "\033[0m"

#define BLACK   "\033[30m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"


#define BOOT2_SIZE 256
#define BOOT2_PROGRAM_SIZE 252

typedef struct {
    u8 bytes[BOOT2_PROGRAM_SIZE];
    u32 crc;
} program_crc32_t;



// Translated from pico-bootrom-rp2040 assembly.
// https://github.com/raspberrypi/pico-bootrom-rp2040/blob/master/bootrom/bootrom_misc.S
u32 get_crc_sum(u8 *bytes, u32 num_bytes, u32 seed) {
    u32 poly = 0x04c11db7;
    u32 csum = seed;

    for (u32 i = 0; i < num_bytes; i++) {
        u32 byte = bytes[i];
        byte <<= 24;
        byte ^= (csum & 0xFF000000);
        for (u8 j = 0; j < 8; j++) {
            byte = (byte & 1<<31) ? ((byte<<1) ^ poly) : (byte<<1);
        }
        csum <<= 8;
        csum ^= byte;
    }

    return csum;
}


int main(int argc, char** argv) {

    char *in_file_name;
    char *out_file_name;

    if (argc == 3) {
        in_file_name = argv[1];
        out_file_name = argv[2];
    }
    else {
        fprintf(stderr, RED "ERROR:" RESET " [tico-makeboot2 takes 2 arguments]  tico-makeboot2 file1.elf.bin file2.uf2\n");
        return 1;
    }

    file_contents_t program_file = ReadEntireFile(in_file_name);

    if (program_file.contents == NULL) {
        fprintf(stderr, RED "ERROR:" RESET " Could not read file \"" YELLOW "%s" RESET "\".\n", in_file_name);
        return 2;
    }

    if (program_file.contents_size > BOOT2_SIZE) {
        fprintf(stderr, RED "ERROR:" RESET " The contents of file \"" YELLOW "%s" RESET "\" are too big to fit in to boot2 (max 252 bytes + 4 byte crc).\n", in_file_name);
        return 3;
    }

    arena_t program_arena = get_new_arena(BOOT2_SIZE);
    program_crc32_t *boot2_program = (program_crc32_t *)arena_push(&program_arena, BOOT2_SIZE);

    for (u32 i = 0; i < program_file.contents_size; i++) {
        boot2_program->bytes[i] = ((u8 *)program_file.contents)[i];
    }
    boot2_program->crc = get_crc_sum(boot2_program->bytes, BOOT2_PROGRAM_SIZE, 0xFFFFFFFF);

    WriteEntireFile(out_file_name, program_arena.memory, program_arena.used);

    return 0;
}
