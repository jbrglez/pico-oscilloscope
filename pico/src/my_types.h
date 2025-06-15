#ifndef MY_TYPES_H
#define MY_TYPES_H


#include <stdint.h>

typedef volatile uint32_t io_rw_32;
typedef volatile uint32_t io_wo_32;
typedef const volatile uint32_t io_ro_32;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef int32_t b32;


#define internal static
#define global   static


#define MIN(a,b) (((a)<(b) ? (a) : (b)))
#define MAX(a,b) (((a)>(b) ? (a) : (b)))


#endif
