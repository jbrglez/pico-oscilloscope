#ifndef GRAPHICS_C
#define GRAPHICS_C

#include "raylib.h"
#include "rlgl.h"
#include "glad.h"

#include "my_types.h"
#include "my_ui.h"

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

#endif
