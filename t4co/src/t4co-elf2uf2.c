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


#define UF2_BLOCK_SIZE 0x200
#define BLOCK_DATA_SIZE 0x100


typedef struct {
    u32 magicStart0;
    u32 magicStart1;
    u32 flags;
    u32 targetAddress;
    u32 payloadSize;
    u32 blockNo;
    u32 numBlocks;
    union {
        u32 fileSize;
        u32 familyID;
    };
    u8 data[476];
    u32 magicEnd;
} uf2_block_t;


void write_uf2_block(uf2_block_t *block, u32 blockNo, u32 numBlocks, u32 targetAddress, u8 *data, u32 copy_len) {
    block->magicStart0 = 0x0A324655;
    block->magicStart1 = 0x9E5D5157;
    block->magicEnd    = 0x0AB16F30;

    block->flags       = 0x00002000;    // familyID flag
    block->familyID    = 0xe48bff56;    // rp2040
    block->payloadSize = BLOCK_DATA_SIZE;

    block->blockNo   = blockNo;
    block->numBlocks = numBlocks;

    block->targetAddress = targetAddress;

    // ASSERT(copy_len <= BLOCK_DATA_SIZE);
    for (u32 i = 0; i < copy_len; i++) {
        block->data[i] = data[i];
    }
}


i32 write_program_to_uf2_file(char *FileName, file_contents_t program_flash_binary) {
    arena_t program_uf2 = get_new_arena(program_flash_binary.contents_size * 2);

    u32 numBlocks = (program_flash_binary.contents_size + 0xff) / 0x100;
    u32 blockNo = 0;
    u32 targetAddr = 0x10000000;

    u32 prog_copy_pos = 0;

    for (u32 i = 0; i < numBlocks - 1; i++) {
        uf2_block_t *block = (uf2_block_t *)arena_push(&program_uf2, UF2_BLOCK_SIZE);
        u8 *binary = &((u8 *)program_flash_binary.contents)[prog_copy_pos];
        write_uf2_block(block, blockNo, numBlocks, targetAddr, binary, BLOCK_DATA_SIZE);

        blockNo += 1;
        targetAddr += BLOCK_DATA_SIZE;
        prog_copy_pos += BLOCK_DATA_SIZE;
    }

    uf2_block_t *block = (uf2_block_t *)arena_push(&program_uf2, UF2_BLOCK_SIZE);
    u8 *binary = &((u8 *)program_flash_binary.contents)[prog_copy_pos];
    write_uf2_block(block, blockNo, numBlocks, targetAddr, binary, program_flash_binary.contents_size - prog_copy_pos);

    WriteEntireFile(FileName, program_uf2.memory, program_uf2.used);
    free_arena(&program_uf2);

    return 0;
}


int main(int argc, char** argv) {

    char *in_file_name;
    char *out_file_name;

    if (argc == 3) {
        in_file_name = argv[1];
        out_file_name = argv[2];
    }
    else {
        fprintf(stderr, RED "ERROR:" RESET " [tico-elf2uf2 takes 2 arguments]  tico-elf2uf2 file1.elf.bin file2.uf2\n");
        return 1;
    }

    file_contents_t program_file = ReadEntireFile(in_file_name);

    if (program_file.contents == NULL) {
        fprintf(stderr, RED "ERROR:" RESET " Could not read file \"" YELLOW "%s" RESET "\".\n", in_file_name);
        return 2;
    }

    write_program_to_uf2_file(out_file_name, program_file);

    return 0;
}
