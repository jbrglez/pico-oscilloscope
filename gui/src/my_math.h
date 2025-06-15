#ifndef MY_MATH_H
#define MY_MATH_H

#include "my_types.h"


#define MIN(a,b)    (((a)<(b)) ? (a) : (b))
#define MAX(a,b)    (((a)>(b)) ? (a) : (b))


internal f32 LERP_t(f32 min, f32 max, f32 val) {
    return (val - min)/(max - min);
}


internal f32 LERP(f32 x, f32 x0, f32 x1, f32 y0, f32 y1) {
    return y0 + (y1 - y0) * (x - x0) / (x1 - x0);
}


internal f32 LERP_LIM(f32 x, f32 x0, f32 x1, f32 y0, f32 y1) {
    f32 t = (x - x0) / (x1 - x0);
    f32 val =  y0 + (y1 - y0) * t;
    if (t < 0) return y0;
    if (t > 1) return y1;
    return val;
}


#endif
