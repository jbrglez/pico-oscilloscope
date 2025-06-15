#ifndef MY_UI_H
#define MY_UI_H

#include "raylib.h"

#include "my_types.h"
#include "my_math.h"



const Color COLOR_STATE_NORMAL = {.r=44, .g=42, .b=42, .a=255};
const Color COLOR_STATE_FOCUSED = {.r=64, .g=62, .b=62, .a=255};
const Color COLOR_STATE_PRESSED = {.r=54, .g=62, .b=52, .a=255};
const Color COLOR_BORDER = {.r=0x26, .g=0x43, .b=0x48, .a=0xff};
const Color COLOR_VALUE = {.r=0x98, .g=0xbb, .b=0x6c, .a=0xff};
const Color COLOR_FONT = {.r=0xaa, .g=0xaa, .b=0xaa, .a=0xff};

const f32 UI_BORDER_WIDTH = 2;
const i32 FONT_SPACE = 10;


typedef enum {
    STATE_NORMAL = 0,
    STATE_FOCUSED,
    STATE_PRESSED
} ElementState;


typedef struct {
    union {
        Vector2 pos;
        struct { f32 x, y;};
    };
    f32 width;
    f32 height;
} MyRect;


internal b32 IsPosInRect(Vector2 pos, MyRect rect)
{
    if ((rect.x <= pos.x) & (pos.x <= (rect.x + rect.width)) & (rect.y <= pos.y) & (pos.y <= (rect.y + rect.height))) {
        return 1;
    } else {
        return 0;
    }
}


internal void DrawRectangleWithBorder(f32 x, f32 y, f32 width, f32 height, f32 border_width, Color color, Color border_color)
{
    DrawRectangleV( (Vector2){x, y}, (Vector2){width, height}, color );

    DrawRectangleV((Vector2){x, y}, (Vector2){width, border_width}, border_color);
    DrawRectangleV((Vector2){x, y + height - border_width}, (Vector2){width, border_width}, border_color);
    DrawRectangleV((Vector2){x, y}, (Vector2){border_width, height}, border_color);
    DrawRectangleV((Vector2){x + width - border_width, y}, (Vector2){border_width, height}, border_color);
}

internal void DrawRectangleBorder(f32 x, f32 y, f32 width, f32 height, f32 border_width, Color border_color)
{
    DrawRectangleV((Vector2){x, y}, (Vector2){width, border_width}, border_color);
    DrawRectangleV((Vector2){x, y + height - border_width}, (Vector2){width, border_width}, border_color);
    DrawRectangleV((Vector2){x, y}, (Vector2){border_width, height}, border_color);
    DrawRectangleV((Vector2){x + width - border_width, y}, (Vector2){border_width, height}, border_color);
}


internal b32 DoButton(f32 x, f32 y, f32 width, f32 height)
{
    b32 is_clicked = 0;
    ElementState state = STATE_NORMAL;

    Vector2 mouse_pos = GetMousePosition();

    if (IsPosInRect(mouse_pos, (MyRect){ .x=x, .y=y, .width=width, .height=height }))
    {
        state = STATE_FOCUSED;
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) state = STATE_PRESSED;
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) is_clicked = 1;
    }

    Color color = COLOR_STATE_NORMAL;
    Color color_border = COLOR_BORDER;
    if (state == STATE_FOCUSED) color = COLOR_STATE_FOCUSED;
    if (state == STATE_PRESSED) color = COLOR_STATE_PRESSED;

    DrawRectangleWithBorder(x, y, width, height, UI_BORDER_WIDTH, color, color_border);

    return is_clicked;
}


internal b32 DoButtonActive(f32 x, f32 y, f32 width, f32 height, b32 condition_active, Color color_active)
{
    b32 is_clicked = 0;
    ElementState state = STATE_NORMAL;

    Vector2 mouse_pos = GetMousePosition();

    if (IsPosInRect(mouse_pos, (MyRect){ .x=x, .y=y, .width=width, .height=height }))
    {
        state = STATE_FOCUSED;
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) state = STATE_PRESSED;
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) is_clicked = 1;
    }

    Color color = (condition_active) ? color_active : COLOR_STATE_NORMAL;
    Color color_border = COLOR_BORDER;
    if ((state == STATE_FOCUSED) && (condition_active == 0)) color = COLOR_STATE_FOCUSED;
    if (state == STATE_PRESSED) color = COLOR_STATE_PRESSED;

    DrawRectangleWithBorder(x, y, width, height, UI_BORDER_WIDTH, color, color_border);

    return is_clicked;
}


internal b32 DoButton_WithState(f32 x, f32 y, f32 width, f32 height, i32 val, Color *val_colors)
{
    b32 is_clicked = 0;
    ElementState state = STATE_NORMAL;

    Vector2 mouse_pos = GetMousePosition();

    if (IsPosInRect(mouse_pos, (MyRect){ .x=x, .y=y, .width=width, .height=height }))
    {
        state = STATE_FOCUSED;
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) state = STATE_PRESSED;
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) is_clicked = 1;
    }

    Color color = val_colors[val];
    Color color_border = COLOR_BORDER;
    if (state == STATE_PRESSED) color = COLOR_STATE_PRESSED;

    DrawRectangleWithBorder(x, y, width, height, UI_BORDER_WIDTH, color, color_border);
    if (state == STATE_FOCUSED) {
        color_border = COLOR_STATE_FOCUSED;
        DrawRectangleBorder(x, y, width, height, 2*UI_BORDER_WIDTH, color_border);
    }

    return is_clicked;
}


internal b32 DoSliderH(f32 x, f32 y, f32 width, f32 height, f32 *val, f32 min, f32 max)
{
    b32 is_pressed = 0;

    f32 border_width = UI_BORDER_WIDTH;

    ElementState state = STATE_NORMAL;

    Vector2 mouse_pos = GetMousePosition();

    if (IsPosInRect(mouse_pos, (MyRect){ .x=x, .y=y, .width=width, .height=height }))
    {
        state = STATE_FOCUSED;
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) state = STATE_PRESSED;
    }


    if (state == STATE_PRESSED) {
        *val = LERP_LIM(mouse_pos.x, x+border_width, x+width - border_width, min, max);
        is_pressed = 1;
    }

    f32 pos = LERP(*val, min, max, 0, width - 2*border_width);

    Color color = COLOR_STATE_NORMAL;
    Color color_border = COLOR_BORDER;
    Color color_val = COLOR_VALUE;
    if (state == STATE_FOCUSED || state == STATE_PRESSED) color = COLOR_STATE_FOCUSED;

    DrawRectangleWithBorder(x, y, width, height, border_width, color, color_border);
    DrawRectangleV((Vector2){x+border_width, y+border_width}, (Vector2){pos, height - 2*border_width}, color_val );

    return is_pressed;
}


internal b32 DoSliderV(f32 x, f32 y, f32 width, f32 height, f32 *val, f32 min, f32 max)
{

    b32 is_pressed = 0;

    f32 border_width = UI_BORDER_WIDTH;

    ElementState state = STATE_NORMAL;

    Vector2 mouse_pos = GetMousePosition();

    if (IsPosInRect(mouse_pos, (MyRect){ .x=x, .y=y, .width=width, .height=height }))
    {
        state = STATE_FOCUSED;
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) state = STATE_PRESSED;
    }

    if (state == STATE_PRESSED) {
        *val = LERP_LIM(mouse_pos.y, y+height - border_width, y+border_width, min, max);
        is_pressed = 1;
    }

    f32 pos = LERP(*val, min, max, 0, height - 2*border_width);

    Color color = COLOR_STATE_NORMAL;
    Color color_border = COLOR_BORDER;
    Color color_val = COLOR_VALUE;
    if (state == STATE_FOCUSED || state == STATE_PRESSED) color = COLOR_STATE_FOCUSED;

    DrawRectangleWithBorder(x, y, width, height, border_width, color, color_border);
    DrawRectangleV( (Vector2){x+border_width, y+height-border_width - pos}, (Vector2){width - 2*border_width, pos}, color_val );

    return is_pressed;
}


#endif
