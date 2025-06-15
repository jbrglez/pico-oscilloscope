#ifndef ARENA_C
#define ARENA_C

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "my_types.h"

#define ARENA_MIN_BLOCK_SIZE 0x1000
#define GET_ALIGNED_ARENA_CAPACITY(new_capacity) (ARENA_MIN_BLOCK_SIZE * ((new_capacity + ARENA_MIN_BLOCK_SIZE - 1)/ARENA_MIN_BLOCK_SIZE))

typedef struct {
    void *memory;
    u64 capacity;
    u64 used;
} arena_t;


arena_t get_new_arena(u64 capacity) {
    capacity = GET_ALIGNED_ARENA_CAPACITY(capacity);

    arena_t new_arena = {
        .memory = calloc(capacity, 1),
        .capacity = capacity,
        .used = 0
    };
    if (new_arena.memory == NULL) {
        new_arena.capacity = 0;
        // perror("ERROR [ARENA]: Failed to allocate new memory arena");
    }

    return new_arena;
}


void *arena_push_size(arena_t *arena, u64 size) {
    if (arena->capacity < arena->used + size) {
        printf("ERROR: Remaining arena capacity too small.");
        return NULL;
    }
    void *memory = &((u8 *)arena->memory)[arena->used] ;
    arena->used += size;
    return memory;
}


void *arena_push_size_zero(arena_t *arena, u64 size) {
    void *ptr = arena_push_size(arena, size);
    memset(ptr, 0, size);
    return NULL;
}


void arena_pop_size(arena_t *arena, u64 size) {
    if (arena->used > size) {
        arena->used -= size;
    } else {
        arena->used = 0;
    }
}


void free_arena(arena_t *arena) {
    arena->capacity = 0;
    arena->used = 0;
    free(arena->memory);
    arena->memory = NULL;
}

#endif
