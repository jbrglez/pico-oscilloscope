#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
// #include <stdlib.h> // strtoul()

#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include "glad.h"

#include <sys/mman.h>

#include "my_types.h"
#include "my_ui.h"
// #include "arena.c"
#ifdef USB_LIBUSB
#include "usb.c"
#else
#ifdef USB_MODULE
#include "usb_mod.c"
#endif
#endif

#include "shaders.h"

#include "graphics.c"


#define samples_buf_len         5120
#define ADC_MAX_VAL             (1<<8)


static u16 active_channels = 1;
static u16 active_channels_rec = 1;


void *get_mem_page_align(u64 length) {
    void *addr = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    return (addr != MAP_FAILED) ? addr : NULL;
}


double get_secs(void) {
    struct timespec tp = {0};
    int ret = clock_gettime(CLOCK_MONOTONIC, &tp);
    ASSERT(ret == 0);
    (void )ret;
    return (double)tp.tv_sec + (double)tp.tv_nsec*1e-9;
}



void get_new_samples(transfer_args_t *args, f32 *dest, i32 size) {
    args->processed = args->transferred;
    i32 idx_completed = args->processed % args->len;

    if (size < idx_completed) {
        f32 *src = &args->data[idx_completed - size];
        for (int i = 0; i < size; i++) {
            dest[i] = src[i];
        }
    }
    else {
        i32 wrapped_right_size = size - idx_completed;
        f32 *src = &args->data[args->len - wrapped_right_size];
        for (int i = 0; i < wrapped_right_size; i++) {
            dest[i] = src[i];
        }
        src = &args->data[0];
        dest = &dest[wrapped_right_size];
        for (int i = 0; i < size - wrapped_right_size; i++) {
            dest[i] = src[i];
        }
    }
}


typedef enum {
    SIGNAL_SINE,
    SIGNAL_TRIANGLE,
    SIGNAL_SAW,
    SIGNAL_SQUARE,
} signal_shape_t;

typedef struct {
    // u32 sample_buf[4096];
    u8 sample_buf[4*4096];
    u32 num_samples;
    signal_shape_t shape;
} sig_generator_t;


int main(void)
{
    usb_stuf_t usb_stuf;
    f32 *recording_buf = get_mem_page_align(recording_buf_len * sizeof(f32));
    f32 *data_buf = get_mem_page_align(data_buf_len * sizeof(f32));

    if ((data_buf == NULL) || (recording_buf == NULL)) {
        fprintf(stderr, "Error allocating memory buffers.\n");
        return -1;
    }

    transfer_args_t transfer_args = {
        .data = data_buf,
        .len = data_buf_len,
        .record_buf = recording_buf,
        .record_len = recording_buf_len,
        .recorded = recording_buf_len,
        .trig_val_relative = 0.5,
        .trig_val = 128,
        .active_ch = &active_channels,
        .active_ch_rec = &active_channels_rec,
    };

    int retval = usb_init(&usb_stuf, &transfer_args);
    if (retval != 0) {
        return -1;
    }


    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1600, 950, "Game");
    if (IsWindowReady())
    {
        SetExitKey(KEY_CAPS_LOCK);
        SetTargetFPS(60);


        // u8 samples_buf[samples_buf_len] = {0};
        f32 samples_buf[samples_buf_len] __attribute__((aligned(64))) = {0};

        graph_data_t live_data = {
            // .data = samples_buf,
            .data = samples_buf,
            .len = samples_buf_len,
            .y_min = 0,
            .y_max = 0x100,
            .x_min = 0,
            .x_max = samples_buf_len - 1
        };
        graph_data_t rec_data = {
            // .data = recording_buf,
            .data = recording_buf,
            .len = recording_buf_len,
            .y_min = 0,
            .y_max = 0x100,
            .x_min = 0,
            .x_max = recording_buf_len - 1,
        };

        window_t live_win = { .x = 50, .y = 50, .width = 1200, .height = 400 };
        window_t rec_win = { .x = 50, .y = 500, .width = 1500, .height = 400 };

        live_win.zoom.x_min = live_data.x_min;
        live_win.zoom.x_max = live_data.x_max;
        live_win.zoom.y_min = live_data.y_min;
        live_win.zoom.y_max = live_data.y_max;

        rec_win.zoom.x_min = rec_data.x_min;
        rec_win.zoom.x_max = rec_data.x_max;
        rec_win.zoom.y_min = rec_data.y_min;
        rec_win.zoom.y_max = rec_data.y_max;


        // --------------------------------------------------------
        //                      OpenGl stuff
        // --------------------------------------------------------

        Shader shader_plot_line  = LoadShaderFromMemory(graph_plot_vs, graph_plot_line_fs);
        Shader shader_plot_point = LoadShaderFromMemory(graph_plot_vs, graph_plot_point_fs);
        Shader shader_win = LoadShaderFromMemory(graph_window_vs, graph_window_fs);

        if(init_win_gl_stuff(&live_win)) {
            fprintf(stderr, "Error initializing window framebuffer\n");
            return -1;
        }
        if(init_win_gl_stuff(&rec_win)) {
            fprintf(stderr, "Error initializing window framebuffer\n");
            return -1;
        }

        init_graph_gl_stuff(&rec_data);
        init_graph_gl_stuff(&live_data);

        // --------------------------------------------------------


        // arena_t scrach_arena = get_new_arena(4 * 1024*1024);


        f32 freq = 440;
        f32 freq_min = 126;
        f32 freq_max = 4000;
        f32 amp = 101;
        f32 amp_min = 0;
        f32 amp_max = (1<<8) - 1;


        sig_generator_t signal = {
            .shape = SIGNAL_SINE,
            .num_samples = (u16)(1.0 / ((0x80 / (126.0 * 1000 * 1000)) * freq)),
        };

        ASSERT(signal.num_samples <= (sizeof(signal.sample_buf) / sizeof(signal.sample_buf[0])));


        // Send initial 440Hz sine wave signal.
        for (u32 i = 0; i < signal.num_samples; i++) {
            f32 sig = (sinf(2*PI * i / signal.num_samples) + 1) / 2;
            signal.sample_buf[i] = (u8)(amp * sig + 0.5);
        }
        send_signal_buf(&usb_stuf, signal.sample_buf, signal.num_samples, 1);
        // transfer_args.recorded = 0; // This triggers first recording.


        graph_data_t signal_data = {
            // .data = (u8 *)signal.sample_buf,
            .len = signal.num_samples,
            .x_min = 0,
            .x_max = signal.num_samples - 1,
            .y_min = amp_min,
            .y_max = amp_max,
        };
        window_t signal_win = { .pos = (Vector2){.x = 50, .y = 50}, .width = 300, .height = 150, };
        signal_win.zoom.x_min = signal_data.x_min;
        signal_win.zoom.x_max = signal_data.x_max;
        signal_win.zoom.y_min = signal_data.y_min;
        signal_win.zoom.y_max = signal_data.y_max;

        if(init_win_gl_stuff(&signal_win)) {
            fprintf(stderr, "Error initializing window framebuffer\n");
            return -1;
        }
        init_graph_gl_stuff(&signal_data);


        // ---------------------------------------
        //              UI size/scale
        // ---------------------------------------

        f32 pix_per_mm = (f32)GetMonitorWidth(GetCurrentMonitor()) / (f32)GetMonitorPhysicalWidth(GetCurrentMonitor());
        f32 ui_scale = 1.0;
        b32 is_ui_rescaled = 0;

        f32 sppm = ui_scale * pix_per_mm;

        f32 pad = 6.0 * sppm;

        f32 controls_width = (i32)(30.7 * sppm);
        Vector2 ctrls_pos = {(f32)GetScreenWidth() - controls_width - pad, pad};

        i32 text_size_1 = (i32)(3.0 * sppm);
        i32 text_size_2 = (i32)(2.4 * sppm);
        i32 button_size_1 = (i32)(4.7 * sppm);
        i32 button_size_2 = (i32)(3.5 * sppm);

        i32 slider_h = (i32)(3.5 * sppm);
        i32 slider_w = (i32)(30.7 * sppm);


        Color ch_colors[2] = {
            {125, 50, 0, 255},
            {50, 75, 25, 255},
        };

        while(!WindowShouldClose()) {

            if(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_EQUAL)) {
                ui_scale += 0.1;
                is_ui_rescaled = 1;
            }
            if(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_MINUS)) {
                ui_scale -= 0.1;
                is_ui_rescaled = 1;
            }
            if(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_ZERO)) {
                ui_scale = 1.0;
                is_ui_rescaled = 1;
            }

            if (IsWindowResized() || is_ui_rescaled) {
                is_ui_rescaled = 0;
                sppm = ui_scale * pix_per_mm;

                controls_width = (i32)(30.7 * sppm);
                ctrls_pos = (Vector2){(f32)GetScreenWidth() - controls_width - pad, pad};

                text_size_1 = (i32)(3.0 * sppm);
                text_size_2 = (i32)(2.4 * sppm);
                button_size_1 = (i32)(4.7 * sppm);
                button_size_2 = (i32)(3.5 * sppm);

                slider_h = (i32)(3.5 * sppm);
                slider_w = (i32)(30.7 * sppm);


                f32 ui_height = 50 * sppm;

                f32 graph_height = ((f32)GetScreenHeight() - 3*pad) / 2;
                f32 graph_width  = ((f32)GetScreenWidth() - 2*pad);

                live_win.x = pad;
                live_win.y = pad;
                live_win.width = graph_width - controls_width - pad;
                live_win.height = graph_height;
                update_window_size(&live_win);

                rec_win.x = pad;
                rec_win.y = 2*pad + graph_height;
                rec_win.width = (graph_height > ui_height) ? graph_width : graph_width - controls_width - pad;
                rec_win.height = graph_height;
                update_window_size(&rec_win);
            }

            get_new_samples(&transfer_args, samples_buf, samples_buf_len);

            BeginDrawing();
            {

                ClearBackground((Color){.r=24, .g=22, .b=22, .a=255});

                //---------------------------------------------------------
                //                      OpenGl
                //---------------------------------------------------------

                rlDrawRenderBatchActive();

                draw_graph_window(&live_win, live_data, active_channels, "LIVE", shader_plot_line, shader_plot_point, &transfer_args.trig_val_relative);
                draw_graph_window(&rec_win, rec_data, active_channels_rec, "CAPTURE", shader_plot_line, shader_plot_point, &transfer_args.trig_val_relative);

                render_tex(&live_win, &shader_win);
                render_tex(&rec_win, &shader_win);

                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBindVertexArray(0);
                glUseProgram(0);

                //---------------------------------------------------------


                Vector2 ch_pos = {ctrls_pos.x, ctrls_pos.y};
                DrawText("CH 1", ch_pos.x + 7.1 * sppm, ch_pos.y + 1.2 * sppm, text_size_1, ch_colors[0]);
                if (DoButtonActive(ch_pos.x, ch_pos.y, button_size_1, button_size_1, (active_channels & 1), ch_colors[0])) {
                    active_channels ^= 0b01;
                    start_active_channels(&usb_stuf, active_channels);
                }

                DrawText("CH 2", ch_pos.x + 7.1 * sppm, ch_pos.y + 8.3 * sppm, text_size_1, ch_colors[1]);
                if (DoButtonActive(ch_pos.x, ch_pos.y + 7.1 * sppm, button_size_1, button_size_1, (active_channels & 2), ch_colors[1])) {
                    active_channels ^= 0b10;
                    start_active_channels(&usb_stuf, active_channels);
                }

                Vector2 rec_pos = {ch_pos.x + 18.9 * sppm, ch_pos.y};
                DrawText("Rec", rec_pos.x + 7.1 * sppm, rec_pos.y + 1.2 * sppm, text_size_1, GRAY);
                if (DoButtonActive(rec_pos.x, rec_pos.y, button_size_1, button_size_1,
                                   (transfer_args.recorded != transfer_args.record_len), GREEN) || IsKeyPressed(KEY_R)) {
                    memset(recording_buf, 0, recording_buf_len * sizeof(f32));
                    transfer_args.recorded = 0;
                    transfer_args.start_triggered_recording = 0;
                }

                DrawText("Trig", rec_pos.x + 7.1 * sppm, rec_pos.y + 8.3 * sppm, text_size_1, GRAY);
                if (DoButtonActive(rec_pos.x, rec_pos.y + 7.1 * sppm, button_size_1, button_size_1,
                                   transfer_args.start_triggered_recording, GREEN) || IsKeyPressed(KEY_T)) {
                    memset(recording_buf, 0, recording_buf_len * sizeof(f32));
                    transfer_args.start_triggered_recording = 1;
                }

                Vector2 trig_val_pos = {ch_pos.x, ch_pos.y + 15 * sppm};
                DrawText("Trig level", trig_val_pos.x, trig_val_pos.y, text_size_2, GRAY);
                if (DoSliderH(trig_val_pos.x, trig_val_pos.y + 3.5 * sppm, slider_w, slider_h, &transfer_args.trig_val_relative, 0.0, 1.0)) {
                    transfer_args.trig_val = (u8)(0xFF * transfer_args.trig_val_relative);
                }


                // ------------------------------------------
                //             SIGNAL GENERATION
                // ------------------------------------------

                f64 dt = 0x80 / (126.0 * 1000 * 1000);
                u16 len = (u16)(1.0 / (dt * freq));
                ASSERT(len <= (sizeof(signal.sample_buf) / sizeof(signal.sample_buf[0])),
                       "len = %u, buffer size = %lu\n", len, (sizeof(signal.sample_buf) / sizeof(signal.sample_buf[0])));

                Vector2 sig_pos = {trig_val_pos.x, trig_val_pos.y + 16 * sppm};

                DrawText(TextFormat("%.0f Hz", freq), sig_pos.x, sig_pos.y - 3.0 * sppm, text_size_2, GREEN);
                DoSliderH(sig_pos.x, sig_pos.y, slider_w, slider_h, &freq, freq_min, freq_max);

                DrawText(TextFormat("%.2f V", 3.3 * LERP_t(amp_min, amp_max, amp)), sig_pos.x, sig_pos.y + 5.3 * sppm, text_size_2, GREEN);
                DoSliderH(sig_pos.x, sig_pos.y + 8.3 * sppm, slider_w, slider_h, &amp, amp_min, amp_max);

                if (IsKeyPressed(KEY_RIGHT) && (freq > freq_min)) freq++;
                if (IsKeyPressed(KEY_LEFT)  && (freq < freq_max)) freq--;

                if (IsKeyPressed(KEY_UP)   && (amp > amp_min)) amp++;
                if (IsKeyPressed(KEY_DOWN) && (amp < amp_max)) amp--;


                i32 sig_button_dist = (i32)((controls_width - button_size_2) / 3);
                i32 sig_button_y = (i32)(14.0 * sppm);
                i32 sig_text_y = (i32)(sig_button_y + button_size_2 + 1.2 * sppm);

                DrawText("SIN", sig_pos.x + 0*sig_button_dist, sig_pos.y + sig_text_y, text_size_2, GRAY);
                if (DoButtonActive(sig_pos.x, sig_pos.y + sig_button_y, button_size_2, button_size_2,
                                   signal.shape == SIGNAL_SINE, GREEN) || IsKeyPressed(KEY_ONE)) {
                    signal.shape = SIGNAL_SINE;
                    signal.num_samples = len;
                    for (i32 i = 0; i < len; i++) {
                        f32 sig = (sinf(2*PI * i / len) + 1) / 2;
                        signal.sample_buf[i] = (u8)(amp * sig + 0.5);
                    }
                    send_signal_buf(&usb_stuf, signal.sample_buf, len, 1);
                }

                DrawText("TRI", sig_pos.x + 1*sig_button_dist, sig_pos.y + sig_text_y, text_size_2, GRAY);
                if (DoButtonActive(sig_pos.x + sig_button_dist, sig_pos.y + sig_button_y, button_size_2, button_size_2,
                                   signal.shape == SIGNAL_TRIANGLE, GREEN) || IsKeyPressed(KEY_TWO)) {
                    signal.shape = SIGNAL_TRIANGLE;
                    signal.num_samples = len;
                    for (i32 i = 0; i < len; i++) {
                        // f32 sig = 2 * (2.0 * i / len - 1);
                        f32 sig = (2.0 * i / (len - 1) - 1);
                        sig = (sig > 0.0) ? sig : -sig;
                        signal.sample_buf[i] = (u8)(amp * sig + 0.5);
                    }
                    send_signal_buf(&usb_stuf, signal.sample_buf, len, 1);
                }

                DrawText("SAW", sig_pos.x + 2*sig_button_dist, sig_pos.y + sig_text_y, text_size_2, GRAY);
                if (DoButtonActive(sig_pos.x + 2*sig_button_dist, sig_pos.y + sig_button_y, button_size_2, button_size_2,
                                   signal.shape == SIGNAL_SAW, GREEN) || IsKeyPressed(KEY_THREE)) {
                    signal.shape = SIGNAL_SAW;
                    signal.num_samples = len;
                    for (i32 i = 0; i < len; i++) {
                        f32 sig = (f32)i / (len - 1);
                        signal.sample_buf[i] = (u8)(amp * sig + 0.5);
                    }
                    send_signal_buf(&usb_stuf, signal.sample_buf, len, 1);
                }

                DrawText("SQ", sig_pos.x + 3*sig_button_dist, sig_pos.y + sig_text_y, text_size_2, GRAY);
                if (DoButtonActive(sig_pos.x +  3*sig_button_dist, sig_pos.y + sig_button_y, button_size_2, button_size_2,
                                   signal.shape == SIGNAL_SQUARE, GREEN) || IsKeyPressed(KEY_FOUR)) {
                    signal.shape = SIGNAL_SQUARE;
                    signal.num_samples = len;
                    for (i32 i = 0; i < len/2; i++) {
                        signal.sample_buf[i] = (u8)(amp + 0.5);
                    }
                    for (i32 i = len/2; i < len; i++) {
                        signal.sample_buf[i] = 0;
                    }
                    send_signal_buf(&usb_stuf, signal.sample_buf, len, 1);
                }

            }
            EndDrawing();
        }


        // glDeleteBuffers(1, &vbo);
        // glDeleteVertexArrays(1, &vao);
        UnloadShader(shader_plot_line);
    }
    CloseWindow();

    usb_close(&usb_stuf);

    return 0;
}
