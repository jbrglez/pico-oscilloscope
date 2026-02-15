#include "my_memory.c"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <libusb-1.0/libusb.h>

#include <string.h>

#include <immintrin.h>

#include "my_types.h"
#include "my_math.h"

#include "my_assert.h"

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
    libusb_device_handle *dev;
    struct libusb_transfer *ctrl_transfer;
    struct libusb_transfer *ctrl_transfer_signal;
    struct libusb_transfer *USB_transfers[NUM_USB_TRANSFERS];
    transfer_args_t *args;
    pthread_t th_USB;
} usb_stuf_t;



typedef struct {
    i32 id;
    i32 size;
    void *data;
} block_t;


typedef struct {
    ring_buffer_t rbuf;
    i32 id_head_next;
    i32 id_tail;
    i32 num_blocks;
} block_queue_t;



internal block_t *push_block(block_queue_t *queue, u64 size, u32 align) {
    void *buf = rbuf_push(&queue->rbuf, sizeof(block_t) + size + align);
    if (buf == NULL)  {
        return NULL;
    }

    block_t *block = (block_t *)buf;
    void *block_data = align_pow2((void *)((uintptr_t)buf + sizeof(block_t)), align);

    *block = (block_t){
        .id = queue->id_head_next,
        .size = sizeof(block_t) + size + align,
        .data = block_data
    };
    queue->id_head_next++;
    queue->num_blocks++;

    return block;
}


internal u64 write_block(block_queue_t *queue, u64 size, void *data, u32 align) {
    block_t *block = push_block(queue, size, align);
    if (block == NULL) {
        return 0;
    }
    memcpy(block->data, data, size);
    return size;
}


internal void queue_free_block(block_queue_t *queue, block_t *block) {
    block->data = NULL;
    if (block->id == queue->id_tail) {
        queue->num_blocks--;

        block_t *next_tail = rbuf_pop(&queue->rbuf, block->size);
        while ((next_tail->data != NULL) && (queue->num_blocks > 0)) {
            queue->num_blocks--;
            next_tail = rbuf_pop(&queue->rbuf, next_tail->size);
        }

        queue->id_tail = (queue->num_blocks != 0) ? next_tail->id : queue->id_head_next;
    }
}


#define DEFAULT_BLOCK_QUEUE_SIZE    MB(2)
block_queue_t g_block_queue;

__attribute__((constructor(202)))
void setup_queue(void) {
    g_block_queue = (block_queue_t){
    .rbuf = get_ring_buffer(DEFAULT_BLOCK_QUEUE_SIZE),
    .id_head_next = 0,
    .id_tail = 0,
    .num_blocks = 0,
    };
}



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


libusb_device_handle *get_dev(void) {
    libusb_device **list;
    libusb_device *found = NULL;
    libusb_device_handle *handle = NULL;

    ssize_t count = libusb_get_device_list(NULL, &list);
    if (count < 0) {
        perror("Failed to get device list.\n");
    }

    for (ssize_t i = 0; i < count; i++) {
        libusb_device *device = list[i];
        struct libusb_device_descriptor device_descriptor;
        if (libusb_get_device_descriptor(device, &device_descriptor) != 0) {
            perror("Failed to get device descriptor.\n");
        }
        else {
            if (device_descriptor.idVendor == ID_VENDOR && device_descriptor.idProduct == ID_PRODUCT) {
                found = device;
                break;
            }
        }
    }

    if (found) {
        if (libusb_open(found, &handle) != 0) {
            perror("Failed to open the device.\n");
        }
        printf("Device SPEED = %d\n", libusb_get_device_speed(found));
    }

    libusb_free_device_list(list, 1);

    return handle;
}


static int usb_should_run = 1;

void *event_thread_func(void *ctx) {
    while (usb_should_run) {
        libusb_handle_events(ctx);
    }

    return NULL;
}


void close_handle(libusb_device_handle *dev_handle, int *open_devs, pthread_t *event_thread) {
    if (*open_devs == 1) {
        usb_should_run = 0;
    }

    libusb_close(dev_handle);

    if (*open_devs == 1) {
        pthread_join(*event_thread, NULL);
    }

    *open_devs -= 1;
}


static void cb_buf_iso(struct libusb_transfer *transfer) {
    transfer_args_t *args = (transfer_args_t *)transfer->user_data;

    if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
        fprintf(stderr, "ERR: transfer status: %d %s\n", transfer->status, libusb_error_name(transfer->status));
        fprintf(stderr, "TRANSFER:\n");
        fprintf(stderr, "\tdev_handle: %p\n", transfer->dev_handle);
        fprintf(stderr, "\tflags: %d\n", transfer->flags);
        fprintf(stderr, "\tendpoint: %d\n", transfer->endpoint);
        fprintf(stderr, "\ttype: %d\n", transfer->type);
        fprintf(stderr, "\ttimeout: %d\n", transfer->timeout);
        fprintf(stderr, "\tstatus: %d\n", transfer->status);
        fprintf(stderr, "\tlength: %d\n", transfer->length);
        fprintf(stderr, "\tactual_length: %d\n", transfer->actual_length);
        fprintf(stderr, "\tuser_data: %p\n", transfer->user_data);
        fprintf(stderr, "\tbuffer: %p\n", transfer->buffer);
        fprintf(stderr, "\tnum_iso_packets: %d\n", transfer->num_iso_packets);
    }
    else {
        if (transfer->type == LIBUSB_TRANSFER_TYPE_ISOCHRONOUS) {
            for (int i = 0; i < transfer->num_iso_packets; i++) {
                struct libusb_iso_packet_descriptor *pkt = &transfer->iso_packet_desc[i];

                if (pkt->status != LIBUSB_TRANSFER_COMPLETED) {
                    printf("\tERROR (ISO Packet): i = %d   status: %d,  %s\n", i, pkt->status, libusb_error_name(pkt->status));
                }
                else {
                    unsigned char *this_buf = libusb_get_iso_packet_buffer_simple(transfer, i);
                    u32 idx_transferred = args->transferred % args->len;

                    if (args->start_triggered_recording) {
                        u32 prev_last_data_idx = (idx_transferred) ? (idx_transferred - 1) : (args->len - 1);
                        f32 last_data = args->data[prev_last_data_idx];

                        if (last_data < args->trig_val) {
                            if (has_greater(args->trig_val, this_buf, pkt->actual_length)){
                                args->start_triggered_recording = 0;
                                args->recorded = 0;
                            }
                        }
                        else {
                            if (has_lower(args->trig_val, this_buf, pkt->actual_length)){
                                args->start_triggered_recording = 0;
                                args->recorded = 0;
                            }
                        }
                    }


                    if (idx_transferred + pkt->actual_length < args->len) {
                        convert_copy_u8_to_f32(&args->data[idx_transferred], this_buf, pkt->actual_length);
                    }
                    else {
                        u32 size_right = args->len - idx_transferred;
                        u32 size_left = pkt->actual_length - size_right;
                        convert_copy_u8_to_f32(&args->data[idx_transferred], this_buf, size_right);
                        convert_copy_u8_to_f32(&args->data[0], &this_buf[size_right], size_left);
                    }
                    args->transferred += pkt->actual_length;

                    if (args->recorded < args->record_len) {
                        if (args->recorded == 0) {
                                *(args->active_ch_rec) = *(args->active_ch);
                        }
                        u8 *src = this_buf;
                        i32 size = MIN(pkt->actual_length, args->record_len - args->recorded);
                        ASSERT((args->recorded + size) <= args->record_len);
                        ASSERT(size >= 0);
                        convert_copy_u8_to_f32(&args->record_buf[args->recorded], src, size);
                        args->recorded += size;
                    }
                }
            }
        }
    }

    if (usb_should_run) {
        memset(transfer->buffer, 0, transfer->length);
        if (libusb_submit_transfer(transfer) != 0) {
            perror("Error! Could not submit the transfer!\n");
            libusb_free_transfer(transfer);
        }
    }
}

static void ctrl_cb_buf(struct libusb_transfer *transfer) {
    if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
        fprintf(stderr, "ERROR in ctrl_cb_buf transfer\n");
        fprintf(stderr, "ERR: transfer status: %d %s\n", transfer->status, libusb_error_name(transfer->status));
        fprintf(stderr, "TRANSFER:\n");
        fprintf(stderr, "\tdev_handle: %p\n", transfer->dev_handle);
        fprintf(stderr, "\tflags: %d\n", transfer->flags);
        fprintf(stderr, "\tendpoint: %d\n", transfer->endpoint);
        fprintf(stderr, "\ttype: %d\n", transfer->type);
        fprintf(stderr, "\ttimeout: %d\n", transfer->timeout);
        fprintf(stderr, "\tstatus: %d\n", transfer->status);
        fprintf(stderr, "\tlength: %d\n", transfer->length);
        fprintf(stderr, "\tactual_length: %d\n", transfer->actual_length);
        fprintf(stderr, "\tuser_data: %p\n", transfer->user_data);
        fprintf(stderr, "\tbuffer: %p\n", transfer->buffer);
        fprintf(stderr, "\tnum_iso_packets: %d\n", transfer->num_iso_packets);
    }

}


static void usb_cb_free(struct libusb_transfer *transfer) {

    if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
        fprintf(stderr, "ERROR in usb_cb_free transfer\n");
        fprintf(stderr, "ERR: transfer status: %d %s\n", transfer->status, libusb_error_name(transfer->status));
        fprintf(stderr, "TRANSFER:\n");
        fprintf(stderr, "\tdev_handle: %p\n", transfer->dev_handle);
        fprintf(stderr, "\tflags: %d\n", transfer->flags);
        fprintf(stderr, "\tendpoint: %d\n", transfer->endpoint);
        fprintf(stderr, "\ttype: %d\n", transfer->type);
        fprintf(stderr, "\ttimeout: %d\n", transfer->timeout);
        fprintf(stderr, "\tstatus: %d\n", transfer->status);
        fprintf(stderr, "\tlength: %d\n", transfer->length);
        fprintf(stderr, "\tactual_length: %d\n", transfer->actual_length);
        fprintf(stderr, "\tuser_data: %p\n", transfer->user_data);
        fprintf(stderr, "\tbuffer: %p\n", transfer->buffer);
        fprintf(stderr, "\tnum_iso_packets: %d\n", transfer->num_iso_packets);
    }

    if (transfer->user_data != NULL) {
        block_t *block = (block_t *)transfer->user_data;
        queue_free_block(&g_block_queue, block);
    }
    libusb_free_transfer(transfer);
}

static int usb_init(usb_stuf_t *usb_stuf, transfer_args_t *transfer_args)
{
    usb_stuf->args = transfer_args;

    int retval = libusb_init_context(NULL, NULL, 0);
    if (retval < 0) {
        fprintf(stderr, "failed to initializa libusb %d - %s\n", retval, libusb_strerror(retval));
        exit(1);
    }

    usb_stuf->dev = NULL;
    usb_stuf->dev = get_dev();
    printf("Device %p\n", usb_stuf->dev);
    if(usb_stuf->dev == NULL) {
        fprintf(stderr, "Error! Could not find USB device!\n");
        libusb_exit(NULL);
        return -1;
    }

    usb_stuf->ctrl_transfer = NULL;
    usb_stuf->ctrl_transfer = libusb_alloc_transfer(0);
    if(usb_stuf->ctrl_transfer == NULL) {
        fprintf(stderr, "Error! Could not allocate the transfer!\n");
        libusb_close(usb_stuf->dev);
        libusb_exit(NULL);
        return -1;
    }

    libusb_fill_control_setup((unsigned char *)usb_stuf->ctrl_buf, 0x40, 3, 1, 0, 0);
    libusb_fill_control_transfer(usb_stuf->ctrl_transfer, usb_stuf->dev, (unsigned char *)usb_stuf->ctrl_buf, ctrl_cb_buf, NULL, 1000);


    usb_stuf->ctrl_transfer_signal = NULL;
    usb_stuf->ctrl_transfer_signal = libusb_alloc_transfer(0);
    if(usb_stuf->ctrl_transfer_signal == NULL) {
        fprintf(stderr, "Error! Could not allocate the transfer!\n");
        libusb_close(usb_stuf->dev);
        libusb_exit(NULL);
        return -1;
    }

    u8 *signal_data = &usb_stuf->ctrl_buf_signal[8];
#define SIG_LEN 0x40
    for (i32 i = 0; i < SIG_LEN/4; i++) {
        ((i32 *)signal_data)[i] = i*4;
    }
    libusb_fill_control_setup((unsigned char *)usb_stuf->ctrl_buf_signal, 0x40, 5, 3, SIG_LEN, SIG_LEN);
    libusb_fill_control_transfer(usb_stuf->ctrl_transfer_signal, usb_stuf->dev, (unsigned char *)usb_stuf->ctrl_buf_signal, ctrl_cb_buf, NULL, 1000);

    u8 *signal_data_2 = &usb_stuf->ctrl_buf_signal_2[8];
    for (i32 i = 0; i < SIG_LEN/4; i++) {
        ((i32 *)signal_data_2)[i] = i;
    }
    libusb_fill_control_setup((unsigned char *)usb_stuf->ctrl_buf_signal_2, 0x40, 5, 3,  SIG_LEN, SIG_LEN);


    for (int i = 0; i < NUM_USB_TRANSFERS; i++) {
        usb_stuf->USB_transfers[i]= libusb_alloc_transfer(NUM_USB_ISO_PACKETS);
        if(usb_stuf->USB_transfers[i] == NULL) {
            fprintf(stderr, "Error! Could not allocate the transfer!\n");
            libusb_close(usb_stuf->dev);
            libusb_exit(NULL);
            return -1;
        }
    }

    if (libusb_submit_transfer(usb_stuf->ctrl_transfer) != 0) {
        fprintf(stderr, "Error! Could not submit the ctrl_transfer!\n");
        // libusb_free_transfer(usb_stuf->ctrl_transfer);
        libusb_close(usb_stuf->dev);
        libusb_exit(NULL);
        return -1;
    }
    libusb_handle_events(0);


    for (int i = 0; i < NUM_USB_TRANSFERS; i++) {
        libusb_fill_iso_transfer(usb_stuf->USB_transfers[i], usb_stuf->dev, 0x84, usb_stuf->iso_buf[i], sizeof(usb_stuf->iso_buf[i]),
                                 NUM_USB_ISO_PACKETS, cb_buf_iso, transfer_args, 1000);
        libusb_set_iso_packet_lengths(usb_stuf->USB_transfers[i], USB_ISO_PACKET_SIZE);

        if (libusb_submit_transfer(usb_stuf->USB_transfers[i]) != 0) {
            fprintf(stderr, "Error! Could not submit the transfer!\n");
            libusb_free_transfer(usb_stuf->USB_transfers[i]);
            libusb_close(usb_stuf->dev);
            libusb_exit(NULL);
            return -1;
        }
        else {
        }
    }

    if (pthread_create(&usb_stuf->th_USB, NULL, event_thread_func, NULL) != 0) {
        perror("Thread creation failed");
    }

    return 0;
}

static void usb_close(usb_stuf_t *usb_stuf) {
    usb_should_run = 0;
    libusb_close(usb_stuf->dev);
    if (pthread_join(usb_stuf->th_USB, NULL) != 0) {
        perror("USB thread joining failed");
    }

    for (int i = 0; i < NUM_USB_TRANSFERS; i++) {
        libusb_free_transfer(usb_stuf->USB_transfers[i]);
    }
    libusb_exit(NULL);
}


static void send_signal_restart(usb_stuf_t *usb_stuf, u16 active_channels) {
    block_t *transfer_buf_block = push_block(&g_block_queue, 16, 2);
    if (transfer_buf_block == NULL) {
        fprintf(stderr, "Error: Malloc failed to allocate buffer for sending\n");
        return;
    }
    u8 *transfer_buf = (u8 *)(transfer_buf_block->data);

    struct libusb_transfer *transfer_reset = libusb_alloc_transfer(0);
    if(transfer_reset == NULL) {
        perror("Error! Could not allocate the transfer!\n");
    }

    libusb_fill_control_setup(transfer_buf, 0x40, 3, active_channels, 0, 0);
    libusb_fill_control_transfer(transfer_reset, usb_stuf->dev, transfer_buf, usb_cb_free, transfer_buf_block, 1000);

    if (libusb_submit_transfer(transfer_reset) != 0) {
        perror("Error! Could not submit the RESTART ctrl_transfer!\n");
        queue_free_block(&g_block_queue, transfer_buf_block);
        libusb_free_transfer(transfer_reset);
    }
}


static void send_signal_buf(usb_stuf_t *usb_stuf, void *sig_buf, u16 len, b32 switch_buffers) {
    const u32 pkt_size = 8 + 0x40;
    const u32 pkt_buf_len = 0x40;

    u32 num_pkt = (len + pkt_buf_len - 1) / pkt_buf_len;

    block_t *transfer_buf_block = push_block(&g_block_queue, pkt_size * num_pkt + 1, 2);
    if (transfer_buf_block == NULL) {
        fprintf(stderr, "Error: Malloc failed to allocate buffer for sending\n");
        return;
    }
    u8 *transfer_buf = (u8 *)(transfer_buf_block->data);

    for (u32 i = 0; i < num_pkt; i++) {
        struct libusb_transfer *transfer = libusb_alloc_transfer(0);
        if(transfer == NULL) {
            printf("Error! Could not allocate the transfer!\n");
        }

        u16 offset = pkt_buf_len * i;
        u16 length = MIN(pkt_buf_len, len - pkt_buf_len*i);
        u16 wValue = 0;
        wValue |= ((i == (num_pkt-1)) && switch_buffers) ? (1) : (0);
        wValue |= (i == 0) ? (2) : (0);
        u16 wIndex = (i == 0) ? len : offset;

        u8 *buf = &transfer_buf[pkt_size * i];
        u8 *buf_data = &buf[8];
        for (i32 j = 0; j < length; j++) {
            buf_data[j] = ((u8 *)sig_buf)[pkt_buf_len*i + j];
        }
        libusb_fill_control_setup((unsigned char *)buf, 0x40, 5, wValue, wIndex, length);
        libusb_fill_control_transfer(transfer, usb_stuf->dev, (unsigned char *)buf, usb_cb_free,
                                     (i == (num_pkt-1)) ? transfer_buf_block : NULL, 1000);

        if (libusb_submit_transfer(transfer) != 0) {
            printf("Error! Could not submit the signal transfer i = %d.\n", i);
            queue_free_block(&g_block_queue, transfer_buf_block);
            libusb_free_transfer(transfer);
            break;
        }
    }
    send_signal_restart(usb_stuf, *usb_stuf->args->active_ch);
}


static void start_active_channels(usb_stuf_t *usb_stuf, u16 active_channels) {
    libusb_fill_control_setup((unsigned char *)usb_stuf->ctrl_buf, 0x40, 3, active_channels, 0, 0);
    if (libusb_submit_transfer(usb_stuf->ctrl_transfer) != 0) {
        fprintf(stderr, "Error! Could not submit the RESTART ctrl_transfer!\n");
    }
}
