#include <stdio.h>
#include <string.h>
// #include <stdlib.h> // strtoul()

#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include "glad.h"

#include <sys/mman.h>

#include <libusb-1.0/libusb.h>

#include "my_types.h"
#include "my_ui.h"
#include "arena.c"
#include "usb.c"

#include "shaders.h"


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


typedef struct {
    union {
        Rectangle rect;
        MyRect my_rect;
        struct {
            union {
                Vector2 pos;
                struct { f32 x, y;};
            };
            f32 width;
            f32 height;
        };
    };

    struct {
        f32 x_min;
        f32 x_max;
        f32 y_min;
        f32 y_max;

        f32 x1_new;
        f32 x2_new;
        f32 y1_new;
        f32 y2_new;

        Vector2 scr_pos;
        f32 scr_x_prev;
        f32 scr_y_prev;
    } zoom;

    struct {
        GLuint frame_buf;
        GLuint tex;
        GLuint win_vao,win_vbo;
    } gl;

} window_t;


b32 init_win_gl_stuff(window_t *win) {
    win->gl.frame_buf = 0;
    glGenFramebuffers(1, &(win->gl.frame_buf));
    glBindFramebuffer(GL_FRAMEBUFFER, win->gl.frame_buf);

    win->gl.tex = 0;
    glGenTextures(1, &(win->gl.tex));
    glBindTexture(GL_TEXTURE_2D, win->gl.tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, win->width, win->height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, win->gl.tex, 0);
    // GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
    // glDrawBuffers(1, DrawBuffers);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        return -1;
    }

    float win_vert_data[] = {
        0.0f, 0.0f, 0.0f,   0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        0.0f, 1.0f, 0.0f,   0.0f, 1.0f,
        1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,   1.0f, 1.0f,
    };

    win->gl.win_vao = 0;
    win->gl.win_vbo = 0;
    glGenVertexArrays(1, &(win->gl.win_vao));
    glGenBuffers(1, &(win->gl.win_vbo));
    glBindVertexArray(win->gl.win_vao);
    glBindBuffer(GL_ARRAY_BUFFER, win->gl.win_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(win_vert_data), win_vert_data, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return 0;
}


void update_window_size(window_t *win) {
    glBindTexture(GL_TEXTURE_2D, win->gl.tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, win->width, win->height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
}


typedef struct {
    f32 *data;
    u32 len;

    f32 x_min;
    f32 x_max;
    f32 y_min;
    f32 y_max;

    struct {
        GLuint frame_buf;
        GLuint tex;
        GLuint vao1, vbo;
        GLuint vao2_1, vao2_2;
    } gl;
} graph_data_t;


void get_buf_data_from_gl(graph_data_t *graph, f32 *buf, u32 buf_len) {
    glBindBuffer(GL_ARRAY_BUFFER, graph->gl.vbo);
    f32 *ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
    for (u32 i = 0; i < MIN(2*graph->len, buf_len); i++) {
        buf[i] = ptr[i];
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);
}


void init_graph_gl_stuff(graph_data_t *graph) {
    graph->gl.vbo = 0;
    glGenBuffers(1, &(graph->gl.vbo));
    glBindBuffer(GL_ARRAY_BUFFER, graph->gl.vbo);
    glBufferData(GL_ARRAY_BUFFER, graph->len*3*sizeof(f32), NULL, GL_DYNAMIC_DRAW);
    f32 *ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    for (u32 i = 0; i < graph->len; i++) {
        ptr[i] = (f32)i;
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);

    graph->gl.vao1 = 0;
    glGenVertexArrays(1, &(graph->gl.vao1));
    glBindVertexArray(graph->gl.vao1);
    glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, sizeof(f32), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(f32), (void*)(graph->len * sizeof(f32)));
    glEnableVertexAttribArray(1);

    graph->gl.vao2_1 = 0;
    glGenVertexArrays(1, &(graph->gl.vao2_1));
    glBindVertexArray(graph->gl.vao2_1);
    glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 2*sizeof(f32), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 2*sizeof(f32), (void*)(graph->len * sizeof(f32)));
    glEnableVertexAttribArray(1);

    graph->gl.vao2_2 = 0;
    glGenVertexArrays(1, &(graph->gl.vao2_2));
    glBindVertexArray(graph->gl.vao2_2);
    glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, 2*sizeof(f32), (void*)(sizeof(f32)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 2*sizeof(f32), (void*)((graph->len + 1) * sizeof(f32)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}


void draw_graph_window(window_t *win, graph_data_t graph, u16 active_ch, char *name, Shader shader_line, Shader shader_point, f32 *trig_pos) {
    if (trig_pos != NULL) {
        Color trig_color = (Color){.r=84, .g=82, .b=122, .a=155};
        DrawLineV((Vector2){.x = win->x, .y = win->y + LERP(*trig_pos, 0, 1, win->height, 0)},
                  (Vector2){.x = win->x + win->width, .y = win->y + LERP(*trig_pos, 0, 1, win->height, 0)},
                  trig_color);
    }

    if (IsPosInRect(GetMousePosition(), win->my_rect)) {

        Matrix conv = MatrixTranslate(-win->x, -(win->y + win->height), 0.0);
        conv = MatrixMultiply(conv, MatrixScale( 1.0/win->width, -1.0/win->height, 0.0));
        Vector2 rel_pos = Vector2Transform(GetMousePosition(), conv);
        conv = MatrixMultiply(conv, MatrixScale((win->zoom.x_max - win->zoom.x_min), (win->zoom.y_max - win->zoom.y_min), 1.0));
        conv = MatrixMultiply(conv, MatrixTranslate(win->zoom.x_min, win->zoom.y_min, 0.0));
        Vector2 pos = Vector2Transform(GetMousePosition(), conv);

        Vector2 mouse_pos = GetMousePosition();
        Color pos_color = (Color){.r=84, .g=82, .b=82, .a=255};

        DrawLineV((Vector2){.x = win->x, .y = mouse_pos.y},
                  (Vector2){.x = win->x + win->width, .y = mouse_pos.y},
                  pos_color);
        DrawLineV((Vector2){.x = mouse_pos.x, .y = win->y},
                  (Vector2){.x = mouse_pos.x, .y = win->y + win->height},
                  pos_color);

        DrawText(TextFormat("(%.3fms, %.2fV)", pos.x * 0.002, pos.y * 3.3  / 0x100), win->x + 10, win->y + 40, 20, GRAY);

        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            win->zoom.x1_new = pos.x;
            win->zoom.scr_pos = GetMousePosition();
        }
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            f32 x1 = MIN(win->zoom.scr_pos.x, GetMousePosition().x);
            f32 x2 = MAX(win->zoom.scr_pos.x, GetMousePosition().x);
            f32 y1 = win->y;
            f32 y2 = win->y + win->height;
            DrawRectangle(x1, y1, (x2-x1), (y2-y1), (Color){.r=34, .g=32, .b=32, .a=155});
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) {
            win->zoom.x2_new = pos.x;

            win->zoom.x_min = MAX(MIN(win->zoom.x1_new, win->zoom.x2_new), 0);
            win->zoom.x_max = MIN(MAX(win->zoom.x1_new, win->zoom.x2_new), graph.len);
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            Vector2 dr = Vector2Transform(GetMouseDelta(), MatrixScale(conv.m0, conv.m5, 1));
            if (((win->zoom.x_min - dr.x) >= 0) && ((win->zoom.x_max - dr.x) <= graph.len)) {
                win->zoom.x_min -= dr.x;
                win->zoom.x_max -= dr.x;
            }
        }

        f32 dzoom = GetMouseWheelMoveV().y;
        if (dzoom != 0) {
            f32 new_width = (win->zoom.x_max - win->zoom.x_min);
            #define ZOOM_SCALE 1.13
            new_width *= (dzoom > 0) ? ZOOM_SCALE : 1/ZOOM_SCALE;
            win->zoom.x_min = MAX(pos.x - rel_pos.x * new_width, graph.x_min);
            win->zoom.x_max = MIN(pos.x + (1 - rel_pos.x) * new_width, graph.x_max);
        }

        if (IsKeyPressed(KEY_SPACE)) {
            win->zoom.x_min = graph.x_min;
            win->zoom.x_max = graph.x_max;
            win->zoom.y_min = graph.y_min;
            win->zoom.y_max = graph.y_max;
        }
    }

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    glBindFramebuffer(GL_FRAMEBUFFER, win->gl.frame_buf);
    glViewport(0, 0, win->width, win->height);
    glEnable(GL_PROGRAM_POINT_SIZE);

    Matrix mtx = MatrixTranslate(-win->zoom.x_min, -win->zoom.y_min, 0.0);
    mtx = MatrixMultiply(mtx, MatrixScale(2.0/(win->zoom.x_max - win->zoom.x_min), 2.0/(win->zoom.y_max - win->zoom.y_min), 0.0));
    mtx = MatrixMultiply(mtx, MatrixTranslate(-1.0, -1.0, 0.0));

    glUseProgram(shader_point.id);
    glUniformMatrix4fv(shader_point.locs[SHADER_LOC_MATRIX_MVP], 1, false, MatrixToFloat(mtx));
    GLuint plot_color_point = glGetUniformLocation(shader_point.id, "plotColor");

    glUseProgram(shader_line.id);
    glUniformMatrix4fv(shader_line.locs[SHADER_LOC_MATRIX_MVP], 1, false, MatrixToFloat(mtx));
    GLuint plot_color = glGetUniformLocation(shader_line.id, "plotColor");

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);


    f32 colors[2][4] = {
        {0.5, 0.2, 0.0, 1.0},
        {0.2, 0.3, 0.1, 1.0},
    };

    f32 num_points = (win->zoom.x_max - win->zoom.x_min);
    f32 pixels_per_dx = win->width / (num_points);

    glBindBuffer(GL_ARRAY_BUFFER, graph.gl.vbo);

    glBufferSubData(GL_ARRAY_BUFFER, graph.len*sizeof(f32), graph.len*sizeof(f32), graph.data);

    // NOTE: Draw only vertices that are inside the zoomed area.
    if ((active_ch == 0b01) || (active_ch == 0b10)) {
        glBindVertexArray(graph.gl.vao1);
        glUniform4fv(plot_color, 1, colors[active_ch-1]);
        glDrawArrays(GL_LINE_STRIP, 0, graph.len);
        if (pixels_per_dx > 10) {
            glUseProgram(shader_point.id);
            glUniform4fv(plot_color_point, 1, colors[active_ch-1]);
            glDrawArrays(GL_POINTS, 0, graph.len);
        }
    }
    else if (active_ch == 0b11) {
        glBindVertexArray(graph.gl.vao2_1);
        glUniform4fv(plot_color, 1, colors[0]);
        glDrawArrays(GL_LINE_STRIP, 0, graph.len/2);

        glBindVertexArray(graph.gl.vao2_2);
        glUniform4fv(plot_color, 1, colors[1]);
        glDrawArrays(GL_LINE_STRIP, 0, graph.len/2);

        if (pixels_per_dx > 10) {
            glUseProgram(shader_point.id);

            glBindVertexArray(graph.gl.vao2_1);
            glUniform4fv(plot_color_point, 1, colors[0]);
            glDrawArrays(GL_POINTS, 0, graph.len/2);

            glBindVertexArray(graph.gl.vao2_2);
            glUniform4fv(plot_color_point, 1, colors[1]);
            glDrawArrays(GL_POINTS, 0, graph.len/2);
        }
    }

    glBindVertexArray(0);
    glDisable(GL_PROGRAM_POINT_SIZE);
    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);

    DrawText(name, win->x + 10, win->y + 5, 20, GREEN);
}


void render_tex(window_t *win, Shader *shader) {
    float win_wh = GetScreenWidth()/2;
    float win_hh = GetScreenHeight()/2;
    Matrix mtx = {
        win->width/win_wh, 0.0,   0.0, -1.0 + win->x/win_wh,
        0.0, win->height/win_hh, 0.0, 1.0 - (win->height + win->y)/win_hh,
        0.0, 0.0, 1.0, 0.0,
        0.0, 0.0, 0.0, 1.0,
    };

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, win->gl.tex);

    glUseProgram(shader->id);
    glUniformMatrix4fv(shader->locs[SHADER_LOC_MATRIX_MVP], 1, false, MatrixToFloat(mtx));

    glBindVertexArray(win->gl.win_vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
    glUseProgram(0);
};


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
    int r = libusb_init_context(NULL, NULL, 0);
    if (r < 0) {
        fprintf(stderr, "failed to initializa libusb %d - %s\n", r, libusb_strerror(r));
        exit(1);
    }

    libusb_device_handle *dev = NULL;
    dev = get_dev();
    printf("Device %p\n", dev);
    if(dev == NULL) {
        fprintf(stderr, "Error! Could not find USB device!\n");
        libusb_exit(NULL);
        return -1;
    }

    struct libusb_transfer *ctrl_transfer = libusb_alloc_transfer(0);
    if(ctrl_transfer == NULL) {
        fprintf(stderr, "Error! Could not allocate the transfer!\n");
        libusb_close(dev);
        libusb_exit(NULL);
        return -1;
    }

    u8 ctrl_buf[64] __attribute__ ((aligned (2))) = { 0 };
    libusb_fill_control_setup((unsigned char *)ctrl_buf, 0x40, 3, 1, 0, 0);
    libusb_fill_control_transfer(ctrl_transfer, dev, (unsigned char *)ctrl_buf, ctrl_cb_buf, NULL, 1000);


    struct libusb_transfer *ctrl_transfer_signal = libusb_alloc_transfer(0);
    if(ctrl_transfer_signal == NULL) {
        fprintf(stderr, "Error! Could not allocate the transfer!\n");
        libusb_close(dev);
        libusb_exit(NULL);
        return -1;
    }

    u8 ctrl_buf_signal[0x48] __attribute__ ((aligned (2))) = { 0 };
    u8 *signal_data = &ctrl_buf_signal[8];
    #define SIG_LEN 0x40
    for (i32 i = 0; i < SIG_LEN/4; i++) {
        ((i32 *)signal_data)[i] = i*4;
    }
    libusb_fill_control_setup((unsigned char *)ctrl_buf_signal, 0x40, 5, 3, SIG_LEN, SIG_LEN);
    libusb_fill_control_transfer(ctrl_transfer_signal, dev, (unsigned char *)ctrl_buf_signal, ctrl_cb_buf, NULL, 1000);

    u8 ctrl_buf_signal_2[0x48] __attribute__ ((aligned (2))) = { 0 };
    u8 *signal_data_2 = &ctrl_buf_signal_2[8];
    for (i32 i = 0; i < SIG_LEN/4; i++) {
        ((i32 *)signal_data_2)[i] = i;
    }
    libusb_fill_control_setup((unsigned char *)ctrl_buf_signal_2, 0x40, 5, 3,  SIG_LEN, SIG_LEN);


    #define NUM_USB_TRANSFERS 4
    #define NUM_USB_ISO_PACKETS 4
    #define USB_ISO_PACKET_SIZE 960

    u8 iso_buf[NUM_USB_TRANSFERS][NUM_USB_ISO_PACKETS * USB_ISO_PACKET_SIZE] __attribute__ ((aligned (64))) = { 0 };
    struct libusb_transfer *USB_transfers[NUM_USB_TRANSFERS];

    for (int i = 0; i < NUM_USB_TRANSFERS; i++) {
        USB_transfers[i]= libusb_alloc_transfer(NUM_USB_ISO_PACKETS);
        if(USB_transfers[i] == NULL) {
            fprintf(stderr, "Error! Could not allocate the transfer!\n");
            libusb_close(dev);
            libusb_exit(NULL);
            return -1;
        }
    }


    #define recording_buf_len 0x20000
    #define data_buf_len 0x10000

    f32 *recording_buf = get_mem_page_align(recording_buf_len * sizeof(f32));
    if (recording_buf == NULL) {
        fprintf(stderr, "Error? Failed to allocate memory for recording buffer.\n");
        libusb_close(dev);
        libusb_exit(NULL);
        return -1;
    }
    f32 *data_buf = get_mem_page_align(data_buf_len * sizeof(f32));
    if (data_buf == NULL) {
        fprintf(stderr, "Error? Failed to allocate memory for data buffer.\n");
        libusb_close(dev);
        libusb_exit(NULL);
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


    if (libusb_submit_transfer(ctrl_transfer) != 0) {
        fprintf(stderr, "Error! Could not submit the ctrl_transfer!\n");
        // libusb_free_transfer(ctrl_transfer);
        libusb_close(dev);
        libusb_exit(NULL);
        return -1;
    }
    libusb_handle_events(0);


    for (int i = 0; i < NUM_USB_TRANSFERS; i++) {
        libusb_fill_iso_transfer(USB_transfers[i], dev, 0x84, iso_buf[i], sizeof(iso_buf[i]),
                                 NUM_USB_ISO_PACKETS, cb_buf_iso, &transfer_args, 1000);
        libusb_set_iso_packet_lengths(USB_transfers[i], USB_ISO_PACKET_SIZE);

        if (libusb_submit_transfer(USB_transfers[i]) != 0) {
            fprintf(stderr, "Error! Could not submit the transfer!\n");
            libusb_free_transfer(USB_transfers[i]);
            libusb_close(dev);
            libusb_exit(NULL);
            return -1;
        }
        else {
        }
    }

    pthread_t th_USB;
    if (pthread_create(&th_USB, NULL, event_thread_func, NULL) != 0) {
        perror("Thread creation failed");
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
        send_signal_buf(signal.sample_buf, signal.num_samples, dev, 1);
        send_signal_restart(dev, active_channels);
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
                    libusb_fill_control_setup((unsigned char *)ctrl_buf, 0x40, 3, active_channels, 0, 0);
                    if (libusb_submit_transfer(ctrl_transfer) != 0) {
                        fprintf(stderr, "Error! Could not submit the RESTART ctrl_transfer!\n");
                    }
                }

                DrawText("CH 2", ch_pos.x + 7.1 * sppm, ch_pos.y + 8.3 * sppm, text_size_1, ch_colors[1]);
                if (DoButtonActive(ch_pos.x, ch_pos.y + 7.1 * sppm, button_size_1, button_size_1, (active_channels & 2), ch_colors[1])) {
                    active_channels ^= 0b10;
                    libusb_fill_control_setup((unsigned char *)ctrl_buf, 0x40, 3, active_channels, 0, 0);
                    if (libusb_submit_transfer(ctrl_transfer) != 0) {
                        fprintf(stderr, "Error! Could not submit the RESTART ctrl_transfer!\n");
                    }
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
                    send_signal_buf(signal.sample_buf, len, dev, 1);
                    send_signal_restart(dev, active_channels);
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
                    send_signal_buf(signal.sample_buf, len, dev, 1);
                    send_signal_restart(dev, active_channels);
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
                    send_signal_buf(signal.sample_buf, len, dev, 1);
                    send_signal_restart(dev, active_channels);
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
                    send_signal_buf(signal.sample_buf, len, dev, 1);
                    send_signal_restart(dev, active_channels);
                }

            }
            EndDrawing();
        }


        // glDeleteBuffers(1, &vbo);
        // glDeleteVertexArrays(1, &vao);
        UnloadShader(shader_plot_line);
    }
    CloseWindow();

    usb_should_run = 0;
    libusb_close(dev);
    if (pthread_join(th_USB, NULL) != 0) {
        perror("USB thread joining failed");
    }

    for (int i = 0; i < NUM_USB_TRANSFERS; i++) {
        libusb_free_transfer(USB_transfers[i]);
    }
    libusb_exit(NULL);

    return 0;
}
