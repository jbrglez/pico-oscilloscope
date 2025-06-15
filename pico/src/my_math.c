#ifndef MY_MATH_C
#define MY_MATH_C

#include "my_types.h"


internal u32 multiply_u32(u32 a, u32 b) {
    u32 res = 0;
    for (i32 i = 0; i < 32; i++) {
        if (b & (1<<i)) {
            res += a << i;
        }
    }
    return res;
}


internal u64 multiply_u64(u64 a, u64 b) {
    u64 res = 0;
    u64 one = 1;
    for (i32 i = 0; i < 64; i++) {
        if (b & (one<<i)) {
            res += a << i;
        }
    }
    return res;
}


internal u32 divide_u32(u32 a, u32 b, u32 *rem) {
    u32 q = 0;
    u32 r = a;;
    if (b == 0) {
        return ~(u32)0;
    }

    i32 max_pow2 = 0;
    while (((b<<max_pow2) < a) && !((b<<max_pow2) & (1<<31))) {
        max_pow2++;
    }

    for (i32 i = max_pow2; i >= 0 && r >= b; i--) {
        if (r >= (b<<i)) {
            r -= (b<<i);
            q += (1<<i);
        }
    }

    if (rem != 0) {
        *rem = r;
    }

    return q;
}


internal u64 divide_u64(u64 a, u64 b, u64 *rem) {
    u64 q = 0;
    u64 r = a;;
    if (b == 0) {
        return ~(u64)0;
    }

    i32 max_pow2 = 0;
    u64 max_bit = 0x8000000000000000;
    u64 c = (b<<max_pow2);
    while ((c < a) && !(c & max_bit)) {
        max_pow2++;
        c = (b<<max_pow2);
    }

    for (i32 i = max_pow2; i >= 0 && r >= b; i--) {
        if (r >= (b<<i)) {
            r -= (b<<i);
            u64 one = 1;
            q += (one<<i);
        }
    }

    if (rem != 0) {
        *rem = r;
    }

    return q;
}


#endif
