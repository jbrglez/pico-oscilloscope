#ifndef MY_MEMORY_C
#define MY_MEMORY_C

#define _GNU_SOURCE

#include <sys/mman.h>   // mmap(), memfd_create()
#include <unistd.h>     // ftruncate()
#include <string.h>     // memcpy()

#include "my_types.h"
#include "my_math.h"

#include <stdio.h>


#define KB(n) (((u64)(n)) << 10)
#define MB(n) (((u64)(n)) << 20)



u64 g_page_size;

__attribute__((constructor(201)))
void setup_page_size(void) {
    i64 page_size_ = sysconf(_SC_PAGESIZE);
    if (page_size_ == -1) {
        perror("Failed to get page size.");
    }
    g_page_size = page_size_;
}


internal u64 align_to_page(u64 size) {
    return (size + g_page_size - 1) & (~(g_page_size - 1));
}


// Return size aligned to power of 2
internal u64 align_to_page_pow2(u64 size) {
    if (size < g_page_size) {
        if (size == 0)
            return 0;
        else
            return g_page_size;
    }

    size = size - 1;
    size = size | (size >> 1);
    size = size | (size >> 2);
    size = size | (size >> 4);
    size = size | (size >> 8);
    size = size | (size >> 16);
    size = size | (size >> 32);

    return size + 1;
}


internal void *align_pow2(void *addr, u64 block) {
    return (void *)(((uintptr_t)addr + block - 1) & (~((uintptr_t)block - 1)));
}



#define ARENA_MIN_SIZE  MB(16)

typedef struct {
    void *memory;
    u64 size;
    u64 used;
    i32 flags;
} arena_t;



internal arena_t get_arena(u64 size, i32 may_move) {
    size = align_to_page(size);

    arena_t arena = {};

    arena.size  = MAX(size, ARENA_MIN_SIZE);

    arena.memory = mmap(NULL, arena.size, PROT_READ | PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (arena.memory == MAP_FAILED) {
        arena.memory = 0;
        arena.size = 0;
        // perror("[get_arena]: Failed to create an arena (mmap).");
        return arena;
    }
    arena.used = 0;
    arena.flags = (may_move) ? (MREMAP_MAYMOVE) : 0;

    return arena;
}


internal void *arena_push(arena_t *arena, u64 size) {
    if (arena->used + size > arena->size) {
        u64 new_arena_size = MAX(align_to_page(arena->used + size), arena->used + ARENA_MIN_SIZE);
        void *new_memory = mremap(arena->memory, arena->size, new_arena_size, arena->flags);
        if (new_memory == MAP_FAILED) {
            return NULL;
        }
        arena->memory = new_memory;
        arena->size = new_arena_size;
    }

    void *mem = (void *)((uintptr_t)arena->memory + arena->used);
    arena->used += size;

    return mem;
}


internal void arena_pop(arena_t *arena, u64 size) {
    size = MIN(size, arena->used);
    arena->used -= size;
}


internal void arena_pop_zero(arena_t *arena, u64 size) {
    size = MIN(size, arena->used);
    arena->used -= size;
    memset(arena->memory + arena->used, 0, size);
}



typedef struct {
    void *buf;
    u64 size;
    u64 write_pos;
    u64 read_pos;
} ring_buffer_t;



internal ring_buffer_t get_ring_buffer(u64 size) {
    ring_buffer_t rbuf = {};
    size = align_to_page(size);

    int fd = memfd_create("ring_buffer", 0);
    if (fd == -1) {
        // perror("[init_ring_buffer]: Failed to create ring_buffer (memfd_create).");
        return rbuf;
    }
    ftruncate(fd, size);

    rbuf.buf = mmap(NULL, 2*size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (rbuf.buf == MAP_FAILED) {
        rbuf.buf = 0;
        // perror("[init_ring_buffer]: Failed to create ring_buffer (mmap).");
        return rbuf;
    }

    if ((mmap(rbuf.buf,        size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0) == MAP_FAILED) ||
        (mmap(rbuf.buf + size, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0) == MAP_FAILED)) {
        rbuf.buf = 0;
        // perror("[init_ring_buffer]: Failed to create ring_buffer (mmap).");
        return rbuf;
    }

    // if (close(fd) == -1) {
    //     perror("[init_ring_buffer]: Failed to close file descrioptor.");
    // }
    // CHECK: Do we still need "fd" after this, or can we close it here?

    rbuf.size  = size;
    rbuf.write_pos = 0;
    rbuf.read_pos  = 0;

    return rbuf;
}


internal void *rbuf_push(ring_buffer_t *rbuf, u64 size) {
    u64 free = rbuf->size - (rbuf->write_pos - rbuf->read_pos);
    if (size > free) {
        return NULL;
    }
    void *p = (void *)((uintptr_t)rbuf->buf + rbuf->write_pos);
    rbuf->write_pos += size;
    return p;
}


internal i64 rbuf_write(ring_buffer_t *rbuf, void *src, u64 size) {
    u64 free = rbuf->size - (rbuf->write_pos - rbuf->read_pos);
    if (size > free) {
        return -1;
    }
    memcpy((void *)((uintptr_t)rbuf->buf + rbuf->write_pos), src, size);
    rbuf->write_pos += size;
    return size;
}


internal i64 rbuf_write_force(ring_buffer_t *rbuf, void *src, u64 size) {
    u64 sz = size;
    if (size > rbuf->size) {
        size = rbuf->size;
        src = (void *)((uintptr_t)src + (size - rbuf->size));
    }

    u64 free = rbuf->size - (rbuf->write_pos - rbuf->read_pos);
    if (size > free) {
        rbuf->read_pos += size - free;
        if (rbuf->read_pos >= rbuf->size) {
            rbuf->read_pos -= rbuf->size;
            rbuf->write_pos -= rbuf->size;
        }
    }

    memcpy((void *)((uintptr_t)rbuf->buf + rbuf->write_pos), src, size);
    rbuf->write_pos += size;

    if (rbuf->read_pos > rbuf->size) {
        rbuf->read_pos  -= rbuf->size;
        rbuf->write_pos -= rbuf->size;
    }

    return sz;
}


internal i64 rbuf_read(ring_buffer_t *rbuf, void *buf, u64 size) {
    size = MIN(size, rbuf->write_pos - rbuf->read_pos);
    memcpy(buf, (void *)((uintptr_t)rbuf->buf + rbuf->read_pos), size);
    rbuf->read_pos += size;
    if (rbuf->read_pos >= rbuf->size) {
        rbuf->write_pos -= rbuf->size;
        rbuf->read_pos  -= rbuf->size;
    }
    return size;
}


internal i64 rbuf_release_from_end(ring_buffer_t *rbuf, u64 size) {
    size = MIN(size, rbuf->write_pos - rbuf->read_pos);
    rbuf->read_pos += size;
    if (rbuf->read_pos >= rbuf->size) {
        rbuf->write_pos -= rbuf->size;
        rbuf->read_pos  -= rbuf->size;
    }
    return size;
}


internal void *rbuf_pop(ring_buffer_t *rbuf, u64 size) {
    size = MIN(size, rbuf->write_pos - rbuf->read_pos);
    rbuf->read_pos += size;
    if (rbuf->read_pos >= rbuf->size) {
        rbuf->write_pos -= rbuf->size;
        rbuf->read_pos  -= rbuf->size;
    }
    return (void *)((uintptr_t)rbuf->buf + rbuf->read_pos);
}

#endif
