#ifndef AREAN_C
#define AREAN_C

#include <stdlib.h>
#include <stdio.h>

#include "my_types.h"


#define ARENA_BLOCK_SIZE 4096
#define GET_ALIGNED_ARENA_CAPACITY(new_capacity) (ARENA_BLOCK_SIZE * ((new_capacity + ARENA_BLOCK_SIZE - 1)/ARENA_BLOCK_SIZE))


typedef struct {
    u8 *memory;
    u64 capacity;
    u64 used;
} arena_t;


arena_t get_new_arena(u64 new_capacity) {
    new_capacity = GET_ALIGNED_ARENA_CAPACITY(new_capacity);

    arena_t new_arena = {
        .memory = calloc(new_capacity, 1),
        .capacity = new_capacity,
        .used = 0
    };
    if (new_arena.memory == NULL) {
        new_arena.capacity = 0;
        fprintf(stderr, "ERROR [ARENA]: Failed to allocate new memory arena");
    }

    return new_arena;
}


void *arena_push(arena_t *arena, u64 size) {
    if (arena->used + size > arena->capacity) {
        return NULL;
    }

    void *mem = &((u8 *)arena->memory)[arena->used];
    arena->used += size;

    return mem;
}


void free_arena(arena_t *arena) {
    arena->capacity = 0;
    arena->used = 0;
    free(arena->memory);
    arena->memory = NULL;
}


#endif
