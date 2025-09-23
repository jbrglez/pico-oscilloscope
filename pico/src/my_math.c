#ifndef MY_MATH_C
#define MY_MATH_C

#include "my_types.h"
#include "hardware.h"


internal u32 nlz(u32 x) {
    if (x == 0) return 32;

    i32 n = 1;
    if ((x >> 16) == 0) { n += 16; x = x << 16; }
    if ((x >> 24) == 0) { n +=  8; x = x <<  8; }
    if ((x >> 28) == 0) { n +=  4; x = x <<  4; }
    if ((x >> 30) == 0) { n +=  2; x = x <<  2; }
    n -= (x >> 31);

    return n;
}


internal u32 nlz_u64(u64 x) {
    if (x == 0) return 32;

    u32 n = 1;
    if ((x >> 32) == 0) { n += 32; x <<= 32; }
    if ((x >> 48) == 0) { n += 16; x <<= 16; }
    if ((x >> 56) == 0) { n +=  8; x <<=  8; }
    if ((x >> 60) == 0) { n +=  4; x <<=  4; }
    if ((x >> 62) == 0) { n +=  2; x <<=  2; }
    n -= (x >> 63);

    return n;
}


internal u64 multiply_u32_to_u64(u32 a, u32 b) {
    u32 a_lo = a & 0xFFFF;
    u32 b_lo = b & 0xFFFF;
    u32 a_hi = (a>>16) & 0xFFFF;
    u32 b_hi = (b>>16) & 0xFFFF;

    u64 x = (u64)(a_lo * b_lo);
    u64 y = (u64)(a_lo * b_hi) << 16;
    u64 z = (u64)(a_hi * b_lo) << 16;
    u64 w = (u64)(a_hi * b_hi) << 32;

    return x + y + z + w;
}


internal u64 multiply_u64(u64 a, u64 b) {
    u32 a_lo = a & 0xFFFFFFFF;
    u32 b_lo = b & 0xFFFFFFFF;
    u32 a_hi = a >> 32;
    u32 b_hi = b >> 32;

    u64 x = multiply_u32_to_u64(a_lo, b_lo);
    u64 y = multiply_u32_to_u64(a_hi, b_lo) << 32;
    u64 z = multiply_u32_to_u64(a_lo, b_hi) << 32;
    // Everything from (a_hi * b_hi) overflows.

    return x + y + z;
}


internal i64 multiply_i32_to_i64(i32 a, i32 b) {
    i32 sign = 0;
    if (a < 0) { sign ^= 1; a = -a; }
    if (b < 0) { sign ^= 1; b = -b; }

    i64 p = (i64)multiply_u32_to_u64((u32)a, (u32)b);

    return (sign) ? -p : p;
}


internal i64 multiply_i64(i64 a, i64 b) {
    i32 sign = 0;
    if (a < 0) { sign ^= 1; a = -a; }
    if (b < 0) { sign ^= 1; b = -b; }

    i64 p = (i64)multiply_u64((u64)a, (u64)b);

    return (sign) ? -p : p;
}


internal u32 divide_u32(u32 a, u32 b, u32 *rem) {
    if (b == 0) {
        return ~(u32)0;
    }
    sio_hw->div_udivisor = b;
    sio_hw->div_udividend = a;
    while(!(sio_hw->div_csr & SIO_DIV_CSR_READY));
    if (rem != 0) {
        *rem = sio_hw->div_remainder;
    }
    return sio_hw->div_quotient;
}


internal i32 divide_i32(i32 a, i32 b, i32 *rem) {
    if (b == 0) {
        return ~(u32)0;
    }
    sio_hw->div_sdivisor = b;
    sio_hw->div_sdividend = a;
    while(!(sio_hw->div_csr & SIO_DIV_CSR_READY));
    if (rem != 0) {
        *rem = sio_hw->div_remainder;
    }
    return sio_hw->div_quotient;
}


internal u64 divide_u64(u64 a, u64 b, u64 *rem) {
    if (b == 0) {
        return ~(u64)0;
    }

    u64 q = 0;
    u64 r = 0;

    u32 leading_zeros = nlz_u64(a);

    for (i32 i = 64 - leading_zeros - 1; i >= 0; i--) {
        r <<= 1;
        r |= (a >> i) & 1;
        if (r >= b) {
            r -= b;
            q |= 1ul << i;
        }
    }

    if (rem != 0) {
        *rem = r;
    }

    return q;
}


internal i64 divide_i64(i64 a, i64 b, i64 *rem) {
    i32 sign = 0;
    if (a < 0) { sign ^= 1; a = -a; }
    if (b < 0) { sign ^= 1; b = -b; }
    i64 r;
    i64 q = (i64)divide_u64((u64)a, (u64)b, (u64 *)&r);

    if (rem != 0) {
        *rem = (a < 0) ? -r : r;
    }
    return (sign) ? -q : q;
}


#endif
