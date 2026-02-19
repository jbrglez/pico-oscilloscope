#include "my_memory.c"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#include <fcntl.h>

#include <string.h>

#include <immintrin.h>

#include "my_types.h"
#include "my_math.h"

#include "my_assert.h"

#include "pico_usb/pico_osci_ioctl.h"

#include <sys/ioctl.h>

#include <errno.h>

#define ID_VENDOR  0
#define ID_PRODUCT 1

#define NUM_USB_TRANSFERS 4
#define NUM_USB_ISO_PACKETS 4
#define USB_ISO_PACKET_SIZE 960
#define recording_buf_len 0x20000
#define data_buf_len 0x10000



typedef struct {
    u64 transferred;
    u64 processed;

    u16 *active_ch;
    u16 *active_ch_rec;

    u32 len;
    f32 *data;

    f32 *record_buf;
    u32 record_len;
    u32 recorded;

    b32 start_triggered_recording;
    f32 trig_val_relative;
    u8 trig_val;
    u8 trig_channel;
} transfer_args_t;

typedef struct {
    u8 iso_buf[NUM_USB_TRANSFERS][NUM_USB_ISO_PACKETS * USB_ISO_PACKET_SIZE] __attribute__ ((aligned (64)));
    u8 ctrl_buf[64] __attribute__ ((aligned (2)));
    u8 ctrl_buf_signal[0x48]   __attribute__ ((aligned (2)));
    u8 ctrl_buf_signal_2[0x48] __attribute__ ((aligned (2)));
    transfer_args_t *args;
    pthread_t th_USB;
    int fd;
    int counter;
} usb_stuf_t;



// typedef struct {
//     i32 id;
//     i32 size;
//     void *data;
// } block_t;
//
//
// typedef struct {
//     ring_buffer_t rbuf;
// } block_queue_t;
//
//
//
// internal block_t *push_block(block_queue_t *queue, u64 size, u32 align) {
//     void *buf = rbuf_push(&queue->rbuf, sizeof(block_t) + size + align);
//     if (buf == NULL)  {
//         return NULL;
//     }
//
//     block_t *block = (block_t *)buf;
//     void *block_data = align_pow2((void *)((uintptr_t)buf + sizeof(block_t)), align);
//
//     *block = (block_t){
//         .size = sizeof(block_t) + size + align,
//         .data = block_data
//     };
//
//     return block;
// }
//
//
// internal u64 write_block(block_queue_t *queue, u64 size, void *data, u32 align) {
//     block_t *block = push_block(queue, size, align);
//     if (block == NULL) {
//         return 0;
//     }
//     memcpy(block->data, data, size);
//     return size;
// }
//
//
// internal void queue_free_block(block_queue_t *queue, block_t *block) {
//     block->data = NULL;
//     block_t *next_tail = rbuf_get_tail(&queue->rbuf);
//     next_tail = rbuf_peak_read(&queue->rbuf, next_tail->size);
//     while ((next_tail->data != NULL) && (next_tail != NULL)) {
//         next_tail = rbuf_pop(&queue->rbuf, next_tail->size);
//     }
// }
//
//
// #define DEFAULT_BLOCK_QUEUE_SIZE    MB(2)
// block_queue_t g_block_queue;
//
// __attribute__((constructor(202)))
// void setup_queue(void) {
//     g_block_queue = (block_queue_t){
//     .rbuf = get_ring_buffer(DEFAULT_BLOCK_QUEUE_SIZE),
//     };
// }
//


internal void convert_copy_u8_to_f32(f32 *dest, u8 *src, i32 count) {

#ifdef __AVX__
    ASSERT(((intptr_t)src  % (128/8)) == 0);
    ASSERT(((intptr_t)dest % (128/8)) == 0);
    ASSERT((count % (128/8)) == 0);

    __m128i chars;
    __m256 floats, floats2;

    for (i32 i = 0; i < count; i += 16) {
        chars = _mm_loadu_si128((__m128i*)&src[i]);

        floats = _mm256_cvtepi32_ps(_mm256_cvtepu8_epi32(chars));
        _mm256_store_ps(&dest[i + 0], floats);
        // _mm256_storeu_ps(&dest[i + 0], floats);

        floats2 = _mm256_cvtepi32_ps(_mm256_cvtepu8_epi32(_mm_bsrli_si128(chars, 8)));
        _mm256_store_ps(&dest[i + 8], floats2);
        // _mm256_storeu_ps(&dest[i + 8], floats2);

    }

#elif __SSE2__
    ASSERT(((intptr_t)src  % (128/8)) == 0);
    ASSERT(((intptr_t)dest % (128/8)) == 0);
    ASSERT((count % (128/8)) == 0);

    __m128i zeros = _mm_setzero_si128();
    __m128i chars, shorts;
    __m128 floats;

    for (i32 i = 0; i < count; i += 16) {
        chars = _mm_loadu_si128((__m128i*)&src[i]);

        shorts = _mm_unpacklo_epi8(chars, zeros);
        floats = _mm_cvtepi32_ps(_mm_unpacklo_epi16(shorts, zeros));
        _mm_storeu_ps(&dest[i + 0], floats);
        floats = _mm_cvtepi32_ps(_mm_unpackhi_epi16(shorts, zeros));
        _mm_storeu_ps(&dest[i + 4], floats);

        shorts = _mm_unpackhi_epi8(chars, zeros);
        floats = _mm_cvtepi32_ps(_mm_unpacklo_epi16(shorts, zeros));
        _mm_storeu_ps(&dest[i + 8], floats);
        floats = _mm_cvtepi32_ps(_mm_unpackhi_epi16(shorts, zeros));
        _mm_storeu_ps(&dest[i + 12], floats);
    }
#else
    for (i32 i = 0; i < count; i++) {
        dest[i] = (f32)src[i];
    }
#endif
}


internal b32 has_greater(u8 val, u8 *buf, i32 len)  {
#ifdef __AVX2__
    __m128i uchars;
    __m256i shorts, cmp_res;
    __m256i vals = _mm256_set1_epi16(val);

    for (i32 i = 0; i < len; i += 16) {
        uchars = _mm_loadu_si128((__m128i*)&buf[i]);
        shorts = _mm256_cvtepu8_epi16(uchars);
        cmp_res = _mm256_cmpgt_epi16(shorts, vals);

        int res = _mm256_movemask_epi8(cmp_res);
        if (res != 0) {
            return 1;
        }
    }
    return 0;
#else
    for (i32 i = 0; i < len; i++) {
        if (buf[i] > val) {
            return 1;
        }
    }
    return 0;
#endif
}


internal b32 has_lower(u8 val, u8 *buf, i32 len)  {
#ifdef __AVX2__
    __m128i uchars;
    __m256i shorts, cmp_res;
    __m256i vals = _mm256_set1_epi16(val);

    for (i32 i = 0; i < len; i += 16) {
        uchars = _mm_loadu_si128((__m128i*)&buf[i]);
        shorts = _mm256_cvtepu8_epi16(uchars);
        cmp_res = _mm256_cmpgt_epi16(shorts, vals);

        int res = _mm256_movemask_epi8(cmp_res);
        if (res != 0) {
            return 1;
        }
    }
    return 0;
#else
    for (i32 i = 0; i < len; i++) {
        if (buf[i] < val) {
            return 1;
        }
    }
    return 0;
#endif
}




static int usb_should_run = 1;

void *event_thread_func(void *context) {

    usb_stuf_t *usb_stuf = (usb_stuf_t *)context;
    transfer_args_t *args = usb_stuf->args;


    u8 buf[8192];

    printf("thread : addr usb_stuf       = %p\n", usb_stuf);
    printf("thread : addr transfer_args  = %p\n", args);
    printf("thread : addr buf            = %p\n", buf);

    ioctl(usb_stuf->fd, PICO_IOCTL_START_RUNNING, 0b01);

    while (usb_should_run) {
        usleep(1000);
        int bytes_read = read(usb_stuf->fd, buf, 8192);
        if (bytes_read == -1) {
            printf("error number %d\n", errno);
            perror("Error reading data from device.");
            exit(-1);
            continue;
        }

        if (bytes_read > 0) {

            unsigned char *this_buf = buf;
            u32 idx_transferred = args->transferred % args->len;


            if (args->start_triggered_recording) {
                u32 prev_last_data_idx = (idx_transferred) ? (idx_transferred - 1) : (args->len - 1);
                f32 last_data = args->data[prev_last_data_idx];

                if (last_data < args->trig_val) {
                    if (has_greater(args->trig_val, this_buf, bytes_read)){
                        args->start_triggered_recording = 0;
                        args->recorded = 0;
                    }
                }
                else {
                    if (has_lower(args->trig_val, this_buf, bytes_read)){
                        args->start_triggered_recording = 0;
                        args->recorded = 0;
                    }
                }
            }


            if (idx_transferred + bytes_read < args->len) {
                convert_copy_u8_to_f32(&args->data[idx_transferred], this_buf, bytes_read);
            }
            else {
                u32 size_right = args->len - idx_transferred;
                u32 size_left = bytes_read - size_right;
                convert_copy_u8_to_f32(&args->data[idx_transferred], this_buf, size_right);
                convert_copy_u8_to_f32(&args->data[0], &this_buf[size_right], size_left);
            }
            args->transferred += bytes_read;


            if (args->recorded < args->record_len) {
                if (args->recorded == 0) {
                    *(args->active_ch_rec) = *(args->active_ch);
                }
                u8 *src = this_buf;
                i32 size = MIN((u32)bytes_read, args->record_len - args->recorded);
                ASSERT((args->recorded + size) <= args->record_len);
                ASSERT(size >= 0);
                convert_copy_u8_to_f32(&args->record_buf[args->recorded], src, size);
                args->recorded += size;
            }

        }

        // printf("counter %d\t: read %d bytes\n", usb_stuf->counter, bytes_read);
    }

    return NULL;
}


static int usb_init(usb_stuf_t *usb_stuf, transfer_args_t *transfer_args)
{
    int fd;
    char *names[2] = {"/dev/pico_osci0", "/dev/pico_osci1"};
    for (int i = 0; i < 2; i++) {
        fd = open(names[i], O_RDWR);
        if (fd > 0) {
            break;
        }
    }
    if (fd < 0) {
        perror("Failed to open file");
        return -1;
    }
    usb_stuf->fd = fd;
    usb_stuf->counter = 0;
    usb_stuf->args = transfer_args;

    // ioctl(fd, PICO_IOCTL_START_RUNNING, 0b01);


    if (pthread_create(&usb_stuf->th_USB, NULL, event_thread_func, usb_stuf) != 0) {
        perror("Thread creation failed");
        return -1;
    }

    return 0;
}

static void usb_close(usb_stuf_t *usb_stuf) {
    usb_should_run = 0;
    if (pthread_join(usb_stuf->th_USB, NULL) != 0) {
        perror("USB thread joining failed");
    }

    ioctl(usb_stuf->fd, PICO_IOCTL_STOP_RUNNING, 0);
    close(usb_stuf->fd);
}



static void send_signal_restart(usb_stuf_t *usb_stuf, u16 active_channels) {
    (void)usb_stuf;
    (void)active_channels;
}


static void send_signal_buf(usb_stuf_t *usb_stuf, void *sig_buf, u16 len, b32 switch_buffers) {

    (void)switch_buffers;

    ioctl(usb_stuf->fd, PICO_IOCTL_STOP_RUNNING, 0);

    struct buffer waveform = {
        .len = len,
        .buf = sig_buf,
    };

    int retval = ioctl(usb_stuf->fd, PICO_IOCTL_SET_NEW_SIGNAL, &waveform);
    if (retval) {
        printf("Error sending new waveform\n");
    }

    ioctl(usb_stuf->fd, PICO_IOCTL_START_RUNNING, *usb_stuf->args->active_ch);
}


static void start_active_channels(usb_stuf_t *usb_stuf, u16 active_channels) {
    ioctl(usb_stuf->fd, PICO_IOCTL_START_RUNNING, active_channels);
}
