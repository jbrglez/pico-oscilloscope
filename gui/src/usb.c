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


void convert_copy_u8_to_f32(f32 *dest, u8 *src, i32 count) {

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


b32 has_greater(u8 val, u8 *buf, i32 len)  {
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


b32 has_lower(u8 val, u8 *buf, i32 len)  {
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
        free(transfer->user_data);
    }
    libusb_free_transfer(transfer);
}


static void send_signal_restart(libusb_device_handle *dev, u16 active_channels) {
    void *transfer_buf_ = malloc(16);
    if (transfer_buf_ == NULL) {
        fprintf(stderr, "Error: Malloc failed to allocate buffer for sending\n");
        return;
    }
    // NOTE: Buffer address must be 2-byte aligned
    u8 *transfer_buf = (u8 *)(((uintptr_t)transfer_buf_ + 1) & (~1));

    struct libusb_transfer *transfer_reset = libusb_alloc_transfer(0);
    if(transfer_reset == NULL) {
        perror("Error! Could not allocate the transfer!\n");
    }

    libusb_fill_control_setup(transfer_buf, 0x40, 3, active_channels, 0, 0);
    libusb_fill_control_transfer(transfer_reset, dev, transfer_buf, usb_cb_free, transfer_buf_, 1000);

    if (libusb_submit_transfer(transfer_reset) != 0) {
        perror("Error! Could not submit the RESTART ctrl_transfer!\n");
        free(transfer_buf_);
        libusb_free_transfer(transfer_reset);
    }
}


static void send_signal_buf(void *sig_buf, u16 len, libusb_device_handle *dev, b32 switch_buffers) {
    const u32 pkt_size = 8 + 0x40;
    const u32 pkt_buf_len = 0x40;

    u32 num_pkt = (len + pkt_buf_len - 1) / pkt_buf_len;

    void *transfer_buf_ = malloc(pkt_size * num_pkt + 1);
    if (transfer_buf_ == NULL) {
        fprintf(stderr, "Error: Malloc failed to allocate buffer for sending\n");
        return;
    }
    u8 *transfer_buf = (u8 *)(((uintptr_t)transfer_buf_ + 2) & (~1));

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
        libusb_fill_control_transfer(transfer, dev, (unsigned char *)buf, usb_cb_free,
                                     (i == (num_pkt-1)) ? transfer_buf_ : NULL, 1000);

        if (libusb_submit_transfer(transfer) != 0) {
            printf("Error! Could not submit the signal transfer i = %d.\n", i);
            free(transfer_buf_);
            libusb_free_transfer(transfer);
            break;
        }
    }
}
